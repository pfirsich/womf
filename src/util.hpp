#pragma once

#include <memory>

template <typename Derived>
struct SharedPtrOnly {
    using Ptr = std::shared_ptr<Derived>;

    template <typename... Args>
    [[nodiscard]] static std::shared_ptr<Derived> create(Args&&... args)
    {
        return std::make_shared<Derived>(std::forward<Args>(args)...);
    }

protected:
    SharedPtrOnly() = default;
};

template <typename Derived>
struct UniquePtrOnly {
    using Ptr = std::unique_ptr<Derived>;

    template <typename... Args>
    [[nodiscard]] static std::unique_ptr<Derived> create(Args&&... args)
    {
        return std::make_unique<Derived>(std::forward<Args>(args)...);
    }

protected:
    UniquePtrOnly() = default;
};
