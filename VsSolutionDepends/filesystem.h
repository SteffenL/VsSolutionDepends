#pragma once

#include <filesystem>

namespace std { namespace tr2 { namespace sys {

// Ref.: http://stackoverflow.com/a/1750710 (for Boost)
std::tr2::sys::path resolve(
    const std::tr2::sys::path& p,
    const std::tr2::sys::path& base = std::tr2::sys::current_path<std::tr2::sys::path>());

std::tr2::sys::path normalize(
    const std::tr2::sys::path& p);

}}}
