#pragma once

#include <boost/filesystem.hpp>

#include <string>
#include <deque>
#include <memory>
#include <map>
#include <list>

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
    VsAssembly(const boost::filesystem::path& filePath/*, VsProjectPtr parentProject*/);

    virtual ~VsAssembly();

    boost::filesystem::path FilePath;
    //VsProjectPtr ParentProject;
};

class VsAssemblyReference
{
public:
    VsAssemblyReference(const boost::filesystem::path& hintPath, std::unique_ptr<VsAssembly> assembly);

    virtual ~VsAssemblyReference();

    boost::filesystem::path HintPath;
    std::unique_ptr<VsAssembly> Assembly;
    //VsProjectPtr ReferencedFromProject;
    VsProjectPtr ParentProject;
};

class VsProject
{
public:
    VsProject(const boost::filesystem::path& filePath, VsSolutionPtr parentSolution);
    //VsProject(const std::string& filePath, VsSolution* parentSolution);

    virtual ~VsProject();

    boost::filesystem::path FilePath;
    std::deque<std::unique_ptr<VsAssemblyReference>> AssemblyReferences;
    VsSolutionPtr ParentSolution;

private:
    void create();
    void loadAssemblyReferences(const pugi::xml_document& doc);
    boost::filesystem::path getParentDir() const;
};

class VsSolution
{
public:
    VsSolution(const boost::filesystem::path& filePath);
    virtual ~VsSolution();
    void LoadProjects(VsSolutionPtr selfPtr);

    boost::filesystem::path FilePath;
    std::deque<VsProjectPtr> Projects;

private:
    boost::filesystem::path getParentDir() const;

    std::string m_solutionFileContents;
};

class VsSolutionDependencyManager
{
public:
    static bool TopologicalSortSolutions(VsSolutionList& sortedSolutions, const VsSolutionList& solutions);
    static bool ResolveAssemblyReferences(VsSolutionList& solutions, bool loadDependencies);
};

class VsFileLocator
{
public:
    VsFileLocator();
    ~VsFileLocator();

    static bool FindSolutions(const boost::filesystem::path directoryPath, std::deque<boost::filesystem::path>& files);
    static bool FindParentProjectForAssemblyReference(boost::filesystem::path& projectFilePath, const boost::filesystem::path& assemblyHintPath);
    static bool FindParentSolutionForProject(boost::filesystem::path& solutionFilePath, const boost::filesystem::path& projectFilePath);
};

class VsFileRepo
{
public:
    static VsSolutionPtr CreateSolution(const boost::filesystem::path& filePath);
    static VsSolutionPtr GetSolution(const boost::filesystem::path& filePath);
    static VsProjectPtr CreateProject(const boost::filesystem::path& filePath, VsSolutionPtr parentSolution);
    static VsProjectPtr GetProject(const boost::filesystem::path& filePath);

    static bool HasLoadedSolution(const boost::filesystem::path& filePath);
    static bool HasLoadedProject(const boost::filesystem::path& filePath);
    static void RegisterSolution(const boost::filesystem::path& filePath, VsSolutionPtr solution);
    static void RegisterProject(const boost::filesystem::path& filePath, VsProjectPtr project);

private:
    static std::map<std::string, VsSolutionPtr> m_solutionCache;
    static std::map<std::string, VsProjectPtr> m_projectCache;
};
