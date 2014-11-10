#include "filesystem.h"

std::tr2::sys::path std::tr2::sys::resolve(const path& p, const path& base /*= current_path<path>()*/)
{
    std::tr2::sys::path abs_p = std::tr2::sys::complete(p, base);
    std::tr2::sys::path result;
    for (std::tr2::sys::path::iterator it = abs_p.begin();
        it != abs_p.end();
        ++it)
    {
        if (*it == "..")
        {
            // /a/b/.. is not necessarily /a if b is a symbolic link
            if (std::tr2::sys::is_symlink(result))
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

std::tr2::sys::path std::tr2::sys::normalize(const path& p)
{
    std::tr2::sys::path result;
    for (std::tr2::sys::path::iterator it = p.begin();
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
