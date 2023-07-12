#pragma once

#include <cstring>
#include <span>
#include <variant>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "buffer.hpp"

enum class Interpolation {
    Step,
    Linear,
    // Cubic,
};

namespace detail {
template <typename T>
T interpolate(
    Interpolation interp, float time, std::span<const float> times, std::span<const T> values);

extern template float interpolate<float>(
    Interpolation interp, float time, std::span<const float> times, std::span<const float> values);

extern template glm::vec3 interpolate<glm::vec3>(Interpolation interp, float time,
    std::span<const float> times, std::span<const glm::vec3> values);

extern template glm::quat interpolate<glm::quat>(Interpolation interp, float time,
    std::span<const float> times, std::span<const glm::quat> values);
}

template <typename T>
class SamplerT {
public:
    SamplerT(Interpolation interp, std::span<const float> times, std::span<const T> values)
        : times_(times.begin(), times.end())
        , values_(values.begin(), values_.end())
        , interp_(interp)
    {
        checkValues();
    }

    // quat needs to be xyzw (like glm by default and glTF)
    SamplerT(Interpolation interp, BufferBase::Ptr times, BufferBase::Ptr values)
        : times_(times->size() / sizeof(float))
        , values_(values->size() / sizeof(T))
        , interp_(interp)
    {
        assert(times->data().size() % sizeof(float) == 0);
        assert(values->data().size() % sizeof(T) == 0);
        std::memcpy(&times_[0], times->data().data(), times->data().size());
        std::memcpy(&values_[0], values->data().data(), values->data().size());
        checkValues();
    }

    Interpolation getInterpolation() const { return interp_; }

    float getDuration() const { return times_.back(); }

    T sample(float time) const
    {
        return detail::interpolate(
            interp_, time, std::span<const float>(times_), std::span<const T>(values_));
    }

private:
    void checkValues()
    {
        assert(times_.size() > 0);
        assert(times_.size() == values_.size()); // not true for cubic
        for (size_t i = 1; i < times_.size(); ++i) {
            assert(times_[i] > 0.0f);
            assert(times_[i] > times_[i - 1]);
        }
    }

    // TODO: Later keep a pointer to the buffers and check for updates
    // We have vectors here instead of just buffer pointers, because we need to copy to avoid strict
    // aliasing violations :(
    std::vector<float> times_;
    std::vector<T> values_;
    Interpolation interp_;
    float duration_;
};

class Sampler {
public:
    enum class Type {
        Scalar,
        Vec3,
        Quat,
    };

    Sampler(Type type, Interpolation interp, BufferBase::Ptr times, BufferBase::Ptr values)
        : sampler_(makeSampler(type, interp, std::move(times), std::move(values)))
        , type_(type)
    {
    }

    Sampler(Type type, Interpolation interp, Buffer::Ptr times, Buffer::Ptr values)
        : Sampler(type, interp, std::static_pointer_cast<BufferBase>(std::move(times)),
            std::static_pointer_cast<BufferBase>(std::move(values)))
    {
    }

    Sampler(Type type, Interpolation interp, BufferView::Ptr times, BufferView::Ptr values)
        : Sampler(type, interp, std::static_pointer_cast<BufferBase>(std::move(times)),
            std::static_pointer_cast<BufferBase>(std::move(values)))
    {
    }

    Type getType() const { return type_; }

    float getDuration() const
    {
        return std::visit([](const auto& sampler) { return sampler.getDuration(); }, sampler_);
    }

    Interpolation getInterpolation() const
    {
        return std::visit([](const auto& sampler) { return sampler.getInterpolation(); }, sampler_);
    }

    // Returns variant, so I don't have to return sol::object and include sol in a header file :(
    std::variant<float, glm::vec3, glm::quat> sample(float time) const
    {
        return std::visit(
            [time](const auto& sampler) -> std::variant<float, glm::vec3, glm::quat> {
                return sampler.sample(time);
            },
            sampler_);
    }

private:
    template <typename... Args>
    std::variant<SamplerT<float>, SamplerT<glm::vec3>, SamplerT<glm::quat>> makeSampler(
        Type type, Args&&... args)
    {
        switch (type) {
        case Type::Scalar:
            return SamplerT<float>(std::forward<Args>(args)...);
        case Type::Vec3:
            return SamplerT<glm::vec3>(std::forward<Args>(args)...);
        case Type::Quat:
            return SamplerT<glm::quat>(std::forward<Args>(args)...);
        default:
            std::abort();
        }
    }

    std::variant<SamplerT<float>, SamplerT<glm::vec3>, SamplerT<glm::quat>> sampler_;
    Type type_;
};
