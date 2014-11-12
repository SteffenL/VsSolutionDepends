#include "filesystem.h"

#include <nowide/convert.hpp>

#include <algorithm>
#include <string>

#include <Shlwapi.h>

namespace boost { namespace filesystem {

path resolve(const path& p, const path& base /*= current_path<path>()*/)
{
    path abs_p = complete(p, base);
    path result;
    for (path::iterator it = abs_p.begin();
        it != abs_p.end();
        ++it)
    {
        if (*it == "..")
        {
            // /a/b/.. is not necessarily /a if b is a symbolic link
            if (is_symlink(result))
                result /= *it;
            // /a/b/../.. is not /a/b/.. under most circumstances
            // We can end up with ..s in our result because of symbolic links
            else if (result.filename() == "..")
                result /= *it;
            // Otherwise it should be safe to resolve the parent
            else
                result = result.parent_path();
        }
        else if (*it == ".")
        {
            // Ignore
        }
        else
        {
            // Just cat other path entries
            result /= *it;
        }
    }
    return result;
}

path normalize(const path& p)
{
    path result;
    for (path::iterator it = p.begin();
        it != p.end();
        ++it)
    {
        if (*it == "..")
        {
            // /a/b/../.. is not /a/b/.. under most circumstances
            // We can end up with ..s in our result because of symbolic links
            if (result.filename() == "..")
                result /= *it;
            // Otherwise it should be safe to resolve the parent
            else
                result = result.parent_path();
        }
        else if (*it == ".")
        {
            // Ignore
        }
        else
        {
            // Just cat other path entries
            result /= *it;
        }
    }
    return result;
}

path make_relative(const path& base, const path& p)
{
    path nativeBasePath(base);
    nativeBasePath.make_preferred();

    path nativePath(p);
    nativePath.make_preferred();

    wchar_t relativePath_c[MAX_PATH] = { 0 };
    if (!::PathRelativePathToW(relativePath_c, nativeBasePath.native().c_str(), FILE_ATTRIBUTE_DIRECTORY, nativePath.native().c_str(), FILE_ATTRIBUTE_NORMAL)) {
        throw std::runtime_error("Failed to make a relative path");
    }

    path relativePath(nowide::narrow(relativePath_c));
    return relativePath;
}

/*
path make_relative(path a_From, path a_To)
{
    a_From = absolute(a_From); a_To = absolute(a_To);
    path ret;
    path::const_iterator itrFrom(a_From.begin()), itrTo(a_To.begin());
    // Find common base
    for (path::const_iterator toEnd(a_To.end()), fromEnd(a_From.end()); itrFrom != fromEnd && itrTo != toEnd && equivalent(*itrFrom, *itrTo); ++itrFrom, ++itrTo);
    // Navigate backwards in directory to reach previously found base
    for (path::const_iterator fromEnd(a_From.end()); itrFrom != fromEnd; ++itrFrom)
    {
        if ((*itrFrom) != ".")
            ret /= "..";
    }
    // Now navigate down the directory branch
    //ret.append(itrTo, a_To.end());
    for (auto it = itrTo; it != a_To.end(); ++it) {
        ret.append(it->string());
    }
    return ret;
}*/

}}
