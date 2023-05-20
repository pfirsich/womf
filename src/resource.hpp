#include <future>
#include <string>
#include <unordered_map>
#include <variant>

#include "stb_image.h"

#include "glw/texture.hpp"
#include "util.hpp"

namespace womf {
using Texture = glw::Texture;

template <typename T>
class Loader;

template <>
struct Loader<Texture> {
    using StbiImagePtr = std::unique_ptr<uint8_t, decltype(&stbi_image_free)>;
    static StbiImagePtr stbiImagePtr(uint8_t* buffer)
    {
        return StbiImagePtr(buffer, &stbi_image_free);
    }

    struct Result {
        StbiImagePtr data;
        size_t width;
        size_t height;
        size_t channels;
    };

    static std::future<Result> load(const std::string& url);
    static Texture finalize(const Result& res);
};

template <typename T>
bool isFutureReady(const std::future<T>& f)
{
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

template <typename T>
class Resource
    : public std::enable_shared_from_this<Resource<T>>
    , public SharedPtrOnly<Resource<T>> {
public:
    using LoaderFuture = std::future<typename Loader<T>::Result>;

    Resource(std::string id)
        : id_(std::move(id))
        , data_(Loader<T>::load(id_))
    {
    }

    Resource(std::string id, LoaderFuture fut)
        : id_(std::move(id))
        , data_(std::move(fut))
    {
    }

    Resource(std::string id, T data)
        : id_(std::move(id))
        , data_(std::move(data))
        , finalized_(true)
    {
    }

    std::shared_ptr<Resource> getPtr() { return this->shared_from_this(); }

    const std::string& id() const { return id_; }

    bool ready() const { return finalized_ || isFutureReady(std::get<LoaderFuture>(data_)); }

    T& get()
    {
        if (!finalized_) {
            data_ = Loader<T>::finalize(std::get<LoaderFuture>(data_).get());
            finalized_ = true;
        }
        return std::get<T>(data_);
    }

private:
    std::string id_;
    std::variant<LoaderFuture, T> data_;
    bool finalized_ = false;
};

// The registry is non-owning
template <typename T>
class Registry {
public:
    static Registry& instance()
    {
        static Registry reg;
        return reg;
    }

    void add(const std::string& id, std::weak_ptr<T> ptr)
    {
        assert(!resources_.contains(id));
        return resources_.emplace(id, ptr);
    }

    std::weak_ptr<T> getWeak(const std::string& id)
    {
        const auto it = resources_.find(id);
        if (it != resources_.end()) {
            return it->second;
        }
        return {};
    }

    std::shared_ptr<T> get(const std::string& id)
    {
        const auto it = resources_.find(id);
        if (it != resources_.end()) {
            auto ptr = it->second.lock();
            if (ptr) {
                return ptr;
            } else {
                resources_.erase(it);
            }
        }
        auto ptr = T::create(id);
        resources_.emplace(id, ptr);
        return ptr;
    }

private:
    Registry() = default;

    std::unordered_map<std::string, std::weak_ptr<T>> resources_;
};

template <typename T>
auto getResource(const std::string& id)
{
    return Registry<Resource<T>>::instance().get(id);
}

template <typename T>
void addResource(const std::string& id, std::weak_ptr<T> ptr)
{
    Registry<Resource<T>>::instance().add(id, std::move(ptr));
}
}
