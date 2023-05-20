#pragma once

#include <memory>

namespace womf {
template <typename T>
class Function;

template <typename Ret, typename... Args>
class Function<Ret(Args...)> {
public:
    Function() = default;

    Function(std::nullptr_t) { }

    template <typename Func>
    Function(Func&& func)
    {
        callable_.reset(new Callable<Func> { std::forward<Func>(func) });
    }

    Function(const Function&) = delete;

    Function(Function&& other)
        : callable_(std::move(other.callable_))
    {
    }

    template <typename Func>
    Function& operator=(Func&& func)
    {
        callable_.reset(new Callable<Func> { std::forward<Func>(func) });
        return *this;
    }

    Function& operator=(Function&& other)
    {
        callable_ = std::move(other.callable_);
        return *this;
    }

    Function& operator=(std::nullptr_t)
    {
        callable_.reset();
        return *this;
    }

    Function& operator=(const Function&) = delete;

    explicit operator bool() const { return callable_ != nullptr; }

    Ret operator()(Args... args) const
    {
        return callable_->operator()(std::forward<Args>(args)...);
    }

private:
    struct CallableBase {
        virtual Ret operator()(Args...) const = 0;
        virtual ~CallableBase() = default;
    };

    template <typename Func>
    struct Callable final : public CallableBase {
        // Afaik std::function employs the same "trick" of making the stored function mutable, so we
        // can a define single const operator(). The problem is that even const-overloading
        // operator() would fail to compile for mutable functors, because the definition of
        // operator() const has to be valid, which it would not be.
        // std::move_only_function instead allows to inject the cv qualifier through the template
        // parameter, but I think I would have to specialize for each combination, which is a pain,
        // so I don't want to do that.
        mutable std::decay_t<Func> func;

        Callable(Func&& func)
            : func(std::forward<Func>(func))
        {
        }

        Ret operator()(Args... args) const override { return func(std::forward<Args>(args)...); }
    };

    std::unique_ptr<CallableBase> callable_;
};
}
