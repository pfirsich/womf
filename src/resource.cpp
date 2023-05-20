#include "resource.hpp"

#include <array>

#define STBI_WINDOWS_UTF8 // Windows, I hate you
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <spdlog/spdlog.h>

#include "threadpool.hpp"

namespace womf {
template <typename T>
class Loader;

std::future<Loader<Texture>::Result> Loader<Texture>::load(const std::string& url)
{
    return getDefaultThreadPool().submit([url]() {
        int width = 0, height = 0, channels = 0;
        auto image = stbiImagePtr(stbi_load(url.c_str(), &width, &height, &channels, 0));
        if (!image) {
            spdlog::critical("Could not load image: {}", url);
            std::exit(1);
        }
        return Result {
            std::move(image),
            static_cast<size_t>(width),
            static_cast<size_t>(height),
            static_cast<size_t>(channels),
        };
    });
}

static constexpr std::array<glw::ImageFormat, 4> channelsToFormat {
    glw::ImageFormat::Red,
    glw::ImageFormat::Rg,
    glw::ImageFormat::Rgb,
    glw::ImageFormat::Rgba,
};

Texture Loader<Texture>::finalize(const Result& res)
{
    assert(res.channels >= 1 && res.channels <= 4);
    const auto format = channelsToFormat[res.channels - 1];
    // This works because the underlying values are the same
    const auto dataFormat = static_cast<Texture::DataFormat>(format);
    Texture texture(Texture::Target::Texture2D);
    const auto mipmaps = true;
    texture.storage(mipmaps ? 0 : 1, format, res.width, res.height);
    texture.subImage(dataFormat, Texture::DataType::U8, res.data.get());
    if (mipmaps) {
        texture.generateMipmaps();
        texture.setFilter(Texture::MinFilter::LinearMipmapNearest, Texture::MagFilter::Linear);
    } else {
        texture.setFilter(Texture::MinFilter::Linear, Texture::MagFilter::Linear);
    }
    return texture;
}
}
