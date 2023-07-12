#include "animation.hpp"

#include <glm/gtc/quaternion.hpp>

#include <optional>

namespace {
size_t findBeforeIndex(float time, std::span<const float> times)
{
    assert(time >= times.front());
    assert(time <= times.back());
    for (size_t i = 1; i < times.size(); ++i) {
        // >=, so time == times.back() returns times.size() - 2
        if (times[i] >= time) {
            return i - 1;
        }
    }
    return times.size() - 1; // The asserts above should prevent this case
}

template <typename T>
T interpolateStep(const T& a, const T& b, float alpha)
{
    return alpha >= 0.5f ? b : a;
}

template <typename T>
T interpolateLinear(const T& a, const T& b, float alpha)
{
    // Already does slerp for quats!
    return glm::mix(a, b, alpha);
}

template <typename T, typename Func>
T interpolateFunc(Func func, float time, std::span<const float> times, std::span<const T> values)
{
    time = glm::clamp(time, times.front(), times.back());
    const auto beforeIdx = findBeforeIndex(time, times);
    const auto afterIdx = beforeIdx + 1;
    const auto alpha = (time - times[beforeIdx]) / (times[afterIdx] - times[beforeIdx]);
    return func(values[beforeIdx], values[afterIdx], alpha);
}
}

namespace detail {
template <typename T>
T interpolate(
    Interpolation interp, float time, std::span<const float> times, std::span<const T> values)
{
    switch (interp) {
    case Interpolation::Step:
        return interpolateFunc(interpolateStep<T>, time, times, values);
    case Interpolation::Linear:
        return interpolateFunc(interpolateLinear<T>, time, times, values);
    default:
        assert(false && "Invalid interpolation type");
    }
}

template float interpolate<float>(
    Interpolation interp, float time, std::span<const float> times, std::span<const float> values);
template glm::vec3 interpolate<glm::vec3>(Interpolation interp, float time,
    std::span<const float> times, std::span<const glm::vec3> values);
template glm::quat interpolate<glm::quat>(Interpolation interp, float time,
    std::span<const float> times, std::span<const glm::quat> values);
}
