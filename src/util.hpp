#pragma once

#include <cstdio>
#include <memory>
#include <string>

#include "die.hpp"

template <typename Output>
Output readFile(const std::string& filename)
{
    auto file = std::unique_ptr<FILE, decltype(&std::fclose)>(
        std::fopen(filename.c_str(), "rb"), &std::fclose);
    if (!file) {
        throw DieException(fmt::format("Could not open file '{}'", filename));
    }
    std::fseek(file.get(), 0, SEEK_END);
    Output data;
    data.resize(std::ftell(file.get()));
    std::fseek(file.get(), 0, SEEK_SET);
    std::fread(data.data(), 1, data.size(), file.get());
    return data;
}
