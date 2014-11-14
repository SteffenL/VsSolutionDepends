#pragma once

#include <boost/filesystem.hpp>
#include <boost/version.hpp>

namespace boost { namespace filesystem {

// Ref.: http://stackoverflow.com/a/1750710 (for Boost)
path resolve(
    const path& p,
    const path& base = current_path());

path make_relative(const path& p, const path& base);

// Warnings: Also removes preceding double dots (..)
path normalize(const path& p);

}}
