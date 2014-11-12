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

/*
template < >
path& path::append< typename path::iterator >(typename path::iterator begin, typename path::iterator end, const codecvt_type& cvt)
{
    for (; begin != end; ++begin)
        *this /= *begin;
    return *this;
}*/
// Return path when appended to a_From will resolve to same as a_To
//path make_relative(path a_From, path a_To);

}}
