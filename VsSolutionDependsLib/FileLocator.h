#pragma once

#include <boost/filesystem.hpp>
#include <functional>

/// <summary>   Helps searching for files. </summary>
class FileLocator
{
public:
    typedef std::function<bool(const boost::filesystem::path& filePath)> FindFileCallback;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Searches for files. </summary>
    ///
    /// <param name="files">                [out] The files that were found. </param>
    /// <param name="directoryPath">        Full pathname of the directory file. </param>
    /// <param name="recurse">
    ///     true to process recursively, false to process locally only.
    /// </param>
    /// <param name="currentResultCount">   [in,out] Current number of results. </param>
    /// <param name="maxResults">
    ///     (optional) Maximum number of results before stopping the search.
    /// </param>
    /// <param name="callback">
    ///     (optional) A callback function meant for filtering results.
    /// </param>
    ///
    /// <returns>   true if it succeeds, false if it fails. </returns>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static bool FindFiles(std::deque<boost::filesystem::path>& files, const boost::filesystem::path& directoryPath, bool recurse, int& currentResultCount, int maxResults = -1, FindFileCallback callback = nullptr);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Searches for the first file. </summary>
    ///
    /// <param name="foundFilePath">    [out] Full path to the found file. </param>
    /// <param name="directoryPath">    Full path to a directory containing files. </param>
    /// <param name="recurse">
    ///     true to process recursively, false to process locally only.
    /// </param>
    /// <param name="callback">         (optional) specifies the filter. </param>
    ///
    /// <returns>   true if it succeeds, false if it fails. </returns>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static bool FindSingleFile(boost::filesystem::path& foundFilePath, const boost::filesystem::path& directoryPath, bool recurse, FindFileCallback callback = nullptr);
};
