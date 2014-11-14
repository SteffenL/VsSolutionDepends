#include "filesystem.h"
#include "log.h"

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
        LOG_ERROR() << "Failed to make a relative path for \"" << nativePath.string() << "\" using \"" << nativeBasePath.string() << "\" as the base directory.\n";
        throw std::runtime_error("Failed to make a relative path");
    }

    path relativePath(relativePath_c);
    return relativePath;
}

}}
