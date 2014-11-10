#pragma once

#include <string>
#include <deque>
#include <memory>
#include <filesystem>
#include <map>

namespace pugi { class xml_document; }

class VsAssembly;
class VsAssemblyReference;
class VsProject;
class VsSolution;

typedef std::shared_ptr<VsProject> VsProjectPtr;
typedef std::shared_ptr<VsSolution> VsSolutionPtr;
typedef std::deque<VsSolutionPtr> VsSolutionDeque;
typedef std::list<VsSolutionPtr> VsSolutionList;

class VsAssembly
{
public:
    VsAssembly(const std::string& filePath/*, VsProjectPtr parentProject*/);

    virtual ~VsAssembly();

    std::tr2::sys::path FilePath;
    //VsProjectPtr ParentProject;
};

class VsAssemblyReference
{
public:
    VsAssemblyReference(const std::string& hintPath, std::unique_ptr<VsAssembly> assembly);

    virtual ~VsAssemblyReference();

    std::tr2::sys::path HintPath;
    std::unique_ptr<VsAssembly> Assembly;
    //VsProjectPtr ReferencedFromProject;
    VsProjectPtr ParentProject;
};

class VsProject
{
public:
    VsProject(const std::string& filePath, VsSolutionPtr parentSolution);
    //VsProject(const std::string& filePath, VsSolution* parentSolution);

    virtual ~VsProject();

    std::tr2::sys::path FilePath;
    std::deque<std::unique_ptr<VsAssemblyReference>> AssemblyReferences;
    VsSolutionPtr ParentSolution;

private:
    void create();
    void loadAssemblyReferences(const pugi::xml_document& doc);
    std::tr2::sys::path getParentDir() const;
};

class VsSolution
{
public:
    VsSolution(const std::string& filePath);
    virtual ~VsSolution();
    void LoadProjects(VsSolutionPtr selfPtr);

    std::tr2::sys::path FilePath;
    std::deque<VsProjectPtr> Projects;

private:
    std::tr2::sys::path getParentDir() const;

    std::string m_solutionFileContents;
};

class VsSolutionDependencyManager
{
public:
    static VsSolutionList ReorderDependencies(const VsSolutionList& solutions);
    static bool DiscoverDependencies(VsSolutionList& solutions);
};

class VsFileLocator
{
public:
    VsFileLocator();
    ~VsFileLocator();

    static bool FindSolutions(const std::tr2::sys::path directoryPath, std::deque<std::tr2::sys::path>& files);
    static bool FindParentProjectForAssemblyReference(std::tr2::sys::path& projectFilePath, const std::tr2::sys::path& assemblyHintPath);
    static bool FindParentSolutionForProject(std::tr2::sys::path& solutionFilePath, const std::tr2::sys::path& projectFilePath);
};

class VsFileRepo
{
public:
    /*static VsSolutionPtr GetSolution(const std::string& path);
    static VsProjectPtr GetProject(const std::string& path);*/
    /*
    static bool GetSolution(VsSolutionPtr& solution, const std::string& path);
    static bool GetProject(VsProjectPtr& project, const std::string& path);*/

    static VsSolutionPtr CreateSolution(const std::string& filePath);
    static VsSolutionPtr GetSolution(const std::string& filePath);
    static VsProjectPtr CreateProject(const std::string& filePath, VsSolutionPtr parentSolution);
    //static VsProjectPtr CreateProject(const std::string& filePath, VsSolution* parentSolution);
    static VsProjectPtr GetProject(const std::string& filePath);

    static bool HasLoadedSolution(const std::string& filePath);
    static bool HasLoadedProject(const std::string& filePath);
    static void RegisterSolution(const std::string& filePath, VsSolutionPtr solution);
    static void RegisterProject(const std::string& filePath, VsProjectPtr project);

private:
    /*static std::map<std::string, VsSolutionPtr> m_solutionCache;
    static std::map<std::string, VsProjectPtr> m_projectCache;*/
    static std::map<std::string, VsSolutionPtr> m_solutionCache;
    static std::map<std::string, VsProjectPtr> m_projectCache;
};
