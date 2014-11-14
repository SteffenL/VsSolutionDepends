#pragma once

#include "FileLocator.h"

#include <boost/filesystem.hpp>

#include <string>
#include <deque>
#include <memory>
#include <map>
#include <list>
#include <functional>

namespace pugi { class xml_document; }

class VsAssembly;
class VsAssemblyReference;
class VsProject;
class VsSolution;

typedef std::shared_ptr<VsProject> VsProjectPtr;
typedef std::shared_ptr<VsSolution> VsSolutionPtr;
typedef std::deque<VsSolutionPtr> VsSolutionDeque;
typedef std::list<VsSolutionPtr> VsSolutionList;
typedef std::deque<std::unique_ptr<VsAssemblyReference>> VsAssemblyReferenceDeque;

/// <summary>   Describes a .NET assembly. </summary>
class VsAssembly
{
public:

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Constructor. </summary>
    ///
    /// <param name="filePath"> Full path to the assembly file. </param>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    VsAssembly(const boost::filesystem::path& filePath);

    /// <summary>   Destructor. </summary>
    virtual ~VsAssembly();

    /// <summary>   Full path to the assembly file. </summary>
    boost::filesystem::path FilePath;
};

/// <summary>   Describes a reference to a .NET assembly in a Visual Studio project. </summary>
class VsAssemblyReference
{
public:

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Constructor. </summary>
    ///
    /// <param name="hintPath">
    ///     The hint path to the assembly file reference. This is merely a hint, not necessarily the
    ///     real path.
    /// </param>
    /// <param name="assembly"> The assembly being referenced. </param>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    VsAssemblyReference(const boost::filesystem::path& hintPath, std::unique_ptr<VsAssembly> assembly);

    /// <summary>   Destructor. </summary>
    virtual ~VsAssemblyReference();

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>
    ///     The hint path to the assembly file reference. This is merely a hint, not necessarily the
    ///     real path .
    /// </summary>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    boost::filesystem::path HintPath;
    /// <summary>   The assembly being referenced. </summary>
    std::unique_ptr<VsAssembly> Assembly;
    /// <summary>   The parent project of this reference. </summary>
    VsProjectPtr ParentProject;
};

/// <summary>   Describes a Visual Studo project. </summary>
class VsProject
{
public:

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Constructor. </summary>
    ///
    /// <param name="filePath">         Full path to the project file. </param>
    /// <param name="parentSolution">   The parent solution of this project. </param>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    VsProject(const boost::filesystem::path& filePath, VsSolutionPtr parentSolution);

    /// <summary>   Destructor. </summary>
    virtual ~VsProject();

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Tests the file contents to attempt recognize a project. </summary>
    ///
    /// <param name="filePath"> Full path to the file. </param>
    ///
    /// <returns>   true if the test passes, false if the test fails. </returns>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static bool TestFile(const boost::filesystem::path& filePath);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Tests the file extension to attempt recognize a project. </summary>
    ///
    /// <param name="filePath"> Full path to the file. </param>
    ///
    /// <returns>   true if the test passes, false if the test fails. </returns>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static bool TestFileExtension(const boost::filesystem::path& filePath);

    /// <summary>   Full path to the project file. </summary>
    boost::filesystem::path FilePath;
    /// <summary>   The assembly references in this project. </summary>
    VsAssemblyReferenceDeque AssemblyReferences;
    /// <summary>   The parent solution of this project. </summary>
    VsSolutionPtr ParentSolution;

private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Loads the assembly references for this project. </summary>
    ///
    /// <param name="doc">  XML document for the project file. </param>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    void loadAssemblyReferences(const pugi::xml_document& doc);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Gets the parent directory of this project file. </summary>
    ///
    /// <returns>   The parent directory of this project. </returns>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    boost::filesystem::path getParentDir() const;
};

/// <summary>   Describes a Visual Studio solution. </summary>
class VsSolution
{
public:

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Constructor. </summary>
    ///
    /// <param name="filePath"> Full path to the solution file. </param>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    VsSolution(const boost::filesystem::path& filePath);

    /// <summary>   Destructor. </summary>
    virtual ~VsSolution();

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Loads the projects in this solution. </summary>
    ///
    /// <param name="self">
    ///     This solution should be passed to each project so that they also know which solution is
    ///     their parent.
    /// </param>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    void LoadProjects(VsSolutionPtr self);

    /// <summary>   Full path to the solution file. </summary>
    boost::filesystem::path FilePath;
    /// <summary>   The projects in this solution. </summary>
    std::deque<VsProjectPtr> Projects;

private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Gets the parent directory of this solution file. </summary>
    ///
    /// <returns>   The parent directory of this solution. </returns>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    boost::filesystem::path getParentDir() const;

    /// <summary>   The raw contents of the solution file. </summary>
    std::string m_solutionFileContents;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// <summary>
///     A helper class for things regarding Visual Studio solutions.
/// </summary>
////////////////////////////////////////////////////////////////////////////////////////////////////

class VsSolutionHelper
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Does a topologically sort a solution dependency map. </summary>
    ///
    /// <param name="sortedSolutions">  [out] The sorted list of solutions. </param>
    /// <param name="solutions">        The non-sorted list of solutions. </param>
    ///
    /// <returns>   true if it succeeds, false if it fails. </returns>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static bool TopologicallySortSolutions(VsSolutionList& sortedSolutions, const VsSolutionList& solutions);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Resolves assembly references in each project of each solution. </summary>
    ///
    /// <remarks>
    ///     Actually, not the assemblies themselves are resolved, but rather the projects that
    ///     produce them, and those projects' parent solution. It's as simple as letting them know
    ///     who owns it, to make the dependency map complete. Dependencies are the solutions and
    ///     projects that own the produced assemblies that are being referenced. Loading dependencies
    ///     is useful when they aren't already loaded, and aren't in the search path. In this case,
    ///     loading dependencies will attempt to locate the project that produces the assembly, then
    ///     the solution that owns the project, then load the solution and tie things together.
    /// </remarks>
    ///
    /// <param name="solutions">        [in] The solutions. </param>
    /// <param name="loadDependencies"> Load dependencies. </param>
    ///
    /// <returns>   true if it succeeds, false if it fails. </returns>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static bool ResolveAssemblyReferences(VsSolutionList& solutions, bool loadDependencies);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>
    ///     Removes the unresolvable assembly references in projects for each solution. Unresolvable
    ///     assembly references are ones that can't be resolved to a specific Visual Studio project
    ///     that produce the assemblies being referenced.
    /// </summary>
    ///
    /// <param name="solutions">    [in,out] The solutions. </param>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static void RemoveUnresolvableAssemblyReferences(VsSolutionList& solutions);
};

/// <summary>   Helps searching for files related to Visual Studio. </summary>
class VsFileLocator : public FileLocator
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Searches for Visual Studio solution files. </summary>
    ///
    /// <param name="files">            [out] The files that were found. </param>
    /// <param name="directoryPath">    Full path to the root directory containing solutions. </param>
    /// <param name="recurse">
    ///     true to process recursively, false to process locally only.
    /// </param>
    /// <param name="maxResults">       (Optional) The maximum number of solutions to search for. </param>
    ///
    /// <returns>   true if it succeeds, false if it fails. </returns>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static bool FindSolutions(std::deque<boost::filesystem::path>& files, const boost::filesystem::path& directoryPath, bool recurse, int maxResults = -1);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Searches for the first Visual Studio solution file. </summary>
    ///
    /// <param name="foundFilePath">    [out] Full path to the found solution file. </param>
    /// <param name="directoryPath">    Full path to a directory containing solutions. </param>
    /// <param name="recurse">
    ///     true to process recursively, false to process locally only.
    /// </param>
    ///
    /// <returns>   true if it succeeds, false if it fails. </returns>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static bool FindSingleSolution(boost::filesystem::path& foundFilePath, const boost::filesystem::path& directoryPath, bool recurse);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Searches for Visual Studio project files. </summary>
    ///
    /// <param name="files">            [out] The files that were found. </param>
    /// <param name="directoryPath">    Full path to the root directory containing projects. </param>
    /// <param name="recurse">
    ///     true to process recursively, false to process locally only.
    /// </param>
    /// <param name="maxResults">       (Optional) The maximum number of projects to search for. </param>
    ///
    /// <returns>   true if it succeeds, false if it fails. </returns>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static bool FindProjects(std::deque<boost::filesystem::path>& files, const boost::filesystem::path& directoryPath, bool recurse, int maxResults = -1);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Searches for the first Visual Studio project file. </summary>
    ///
    /// <param name="foundFilePath">    [out] Full path to the found project file. </param>
    /// <param name="directoryPath">    Full path to a directory containing projects. </param>
    /// <param name="recurse">
    ///     true to process recursively, false to process locally only.
    /// </param>
    ///
    /// <returns>   true if it succeeds, false if it fails. </returns>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static bool FindSingleProject(boost::filesystem::path& foundFilePath, const boost::filesystem::path& directoryPath, bool recurse);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>
    ///     Searches for the Visual Studio project that produces the .NET assembly being referenced.
    /// </summary>
    ///
    /// <param name="foundFilePath">    [out] Full path to the project file. </param>
    /// <param name="assemblyHintPath"> Hint path for the assembly reference. </param>
    ///
    /// <returns>   true if it succeeds, false if it fails. </returns>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static bool FindParentProjectForAssemblyReference(boost::filesystem::path& foundFilePath, const boost::filesystem::path& assemblyHintPath);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>
    ///     Searches for the Visual Studio solution that owns the specified Visual Studio project
    ///     file.
    /// </summary>
    ///
    /// <param name="foundFilePath">    [out] Full path of the solution file. </param>
    /// <param name="projectFilePath">  Full path of the project file. </param>
    ///
    /// <returns>   true if it succeeds, false if it fails. </returns>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static bool FindParentSolutionForProject(boost::filesystem::path& foundFilePath, const boost::filesystem::path& projectFilePath);
};

/// <summary>   Repository of the loaded Visual Studio solutions, projects, etc. </summary>
class VsFileRepository
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Loads a solution file. </summary>
    ///
    /// <param name="filePath"> Full path to the file. </param>
    ///
    /// <returns>   The solution. </returns>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static VsSolutionPtr CreateSolution(const boost::filesystem::path& filePath);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Gets an already-loaded solution. </summary>
    ///
    /// <param name="filePath"> Full path to the file. </param>
    ///
    /// <returns>   The solution if it is already loaded; otherwise, and empty pointer. </returns>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static VsSolutionPtr GetSolution(const boost::filesystem::path& filePath);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Loads a project file. </summary>
    ///
    /// <param name="filePath">         Full path to the file. </param>
    /// <param name="parentSolution">   The solution that owns the project. </param>
    ///
    /// <returns>   The project. </returns>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static VsProjectPtr CreateProject(const boost::filesystem::path& filePath, VsSolutionPtr parentSolution);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Gets an already-loaded project. </summary>
    ///
    /// <param name="filePath"> Full path to the file. </param>
    ///
    /// <returns>   The project. </returns>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static VsProjectPtr GetProject(const boost::filesystem::path& filePath);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Checks whether a solution is already loaded or not. </summary>
    ///
    /// <param name="filePath"> Full path to the file. </param>
    ///
    /// <returns>   true if loaded, false if not. </returns>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static bool HasLoadedSolution(const boost::filesystem::path& filePath);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Checks whether a project is already loaded or not. </summary>
    ///
    /// <param name="filePath"> Full path to the file. </param>
    ///
    /// <returns>   true if loaded, false if not. </returns>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static bool HasLoadedProject(const boost::filesystem::path& filePath);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Registers a solution as being loaded. </summary>
    ///
    /// <param name="filePath"> Full path to the file. </param>
    /// <param name="solution"> The solution. </param>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static void RegisterSolution(const boost::filesystem::path& filePath, VsSolutionPtr solution);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>   Registers a project as being loaded. </summary>
    ///
    /// <param name="filePath"> Full path to the file. </param>
    /// <param name="project">  The project. </param>
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static void RegisterProject(const boost::filesystem::path& filePath, VsProjectPtr project);

private:
    /// <summary>   The cache of loaded solutions. </summary>
    static std::map<std::string, VsSolutionPtr> m_solutionCache;
    /// <summary>   The cache of loaded projects. </summary>
    static std::map<std::string, VsProjectPtr> m_projectCache;
};
