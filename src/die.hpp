#pragma once

#include <exception>
#include <fmt/format.h>

class DieException : public std::runtime_error {
public:
    template <typename Msg>
    explicit DieException(Msg&& msg)
        : std::runtime_error(std::forward<Msg>(msg))
    {
        // I need 3.3.0 for call constructors, but there is a bug in 3.3.0 that prevents C++
        // exceptions from bubbling up so the error message inside them is lost:
        // https://github.com/ThePhD/sol2/issues/1508
        // This is why I do this weird shit here.
        fmt::print("Error: {}\n", msg);
        std::exit(1);
    }
};

template <typename Format, typename... Args>
void die(Format format, Args&&... args)
{
    throw DieException(fmt::format(fmt::runtime(format), std::forward<Args>(args)...));
}

template <typename Format, typename... Args>
void dieAssert(bool expr, Format format, Args&&... args)
{
    if (!expr) {
        die(format, std::forward<Args>(args)...);
    }
}
