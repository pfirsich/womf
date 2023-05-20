#include <fmt/format.h>

#include "glwx/window.hpp"

#include "resource.hpp"

using namespace womf;

int main()
{
    const auto window = glwx::makeWindow("womf", 1024, 768).value();
    glw::State::instance().setViewport(window.getSize().x, window.getSize().y);

    auto res = getResource<Texture>("assets/test.png");
    fmt::print("Created {}\n", fmt::ptr(res));
    auto res2 = getResource<Texture>("assets/test.png");
    fmt::print("Created {}\n", fmt::ptr(res2));
    size_t unready = 0;
    while (!res->ready()) {
        unready++;
    }
    fmt::print("ready after {} checks\n", unready);
    fmt::print("get res\n");
    res->get();
    fmt::print("get res2\n");
    res2->get();
    fmt::print("Hello, World!\n");
    return 0;
}
