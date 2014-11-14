#include "FileLocator.h"
#include "log.h"
#include <deque>

bool FileLocator::FindFiles(std::deque<boost::filesystem::path>& files, const boost::filesystem::path& directoryPath, bool recurse, int& currentResultCount, int maxResults, FindFileCallback callback)
{
    namespace fs = boost::filesystem;

    if (!fs::exists(directoryPath)) {
        LOG_ERROR() << "Unable to search in the following directory because it doesn't exist: " << directoryPath.string() << "\n";
        return false;
    }

    if (!fs::is_directory(directoryPath)) {
        LOG_ERROR() << "Unable to search because the path isn't a directory: " << directoryPath.string() << "\n";
        return false;
    }

    for (fs::directory_iterator it(directoryPath), end; (it != end) && ((maxResults == -1) || (currentResultCount < maxResults)); ++it) {
        if (fs::is_directory(it->status())) {
            if (recurse) {
                if (!FindFiles(files, it->path(), recurse, currentResultCount, maxResults, callback)) {
                    return false;
                }
            }

            continue;
        }

        if (callback && callback(it->path())) {
            files.push_back(it->path());
            ++currentResultCount;
        }
    }

    return true;
}

bool FileLocator::FindSingleFile(boost::filesystem::path& foundFilePath, const boost::filesystem::path& directoryPath, bool recurse, FindFileCallback callback)
{
    std::deque<boost::filesystem::path> files;
    int currentResultCount = 0;
    if (!FindFiles(files, directoryPath, recurse, currentResultCount, 1, callback)) {
        return false;
    }

    if (files.empty()) {
        return false;
    }

    foundFilePath = files.front();
    return true;
}
