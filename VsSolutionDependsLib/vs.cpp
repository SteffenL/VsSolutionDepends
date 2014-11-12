#include "vs.h"
#include "filesystem.h"
#include "log.h"

#include <nowide/fstream.hpp>
#include <pugixml.hpp>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <regex>
#include <deque>
#include <string>

VsAssembly::~VsAssembly()
{

}

VsAssembly::VsAssembly(const boost::filesystem::path& filePath) : FilePath(filePath)
{

}

VsAssemblyReference::~VsAssemblyReference()
{

}

VsAssemblyReference::VsAssemblyReference(const boost::filesystem::path& hintPath, std::unique_ptr<VsAssembly> assembly) : HintPath(hintPath), Assembly(std::move(assembly))
{

}

VsProject::~VsProject()
{

}

VsProject::VsProject(const boost::filesystem::path& filePath, VsSolutionPtr parentSolution) : FilePath(filePath), ParentSolution(parentSolution)
{
    create();
}

void VsProject::create()
{
    nowide::ifstream projectFile(FilePath.string().c_str());
    if (!projectFile) {
        throw std::runtime_error("Failed to read from file: " + FilePath.string());
    }

    std::ostringstream projectFileContents;
    projectFileContents << projectFile.rdbuf();

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load(projectFileContents.str().c_str());
    if (!result) {
        throw std::runtime_error("Failed to load project due to XML parsing error: " + FilePath.string() + ". Message: " + std::string(result.description()));
    }

    loadAssemblyReferences(doc);
}

void VsProject::loadAssemblyReferences(const pugi::xml_document& doc)
{
    namespace fs = boost::filesystem;
    const auto& projectDir = getParentDir();

    // Exclude the assemblies that are in the GAC (Global Assembly Cache), such as framework assemblies
    // Exclusion is easy because there's no path specified for them
    for (auto& xpathNode : doc.select_nodes("/Project/ItemGroup/Reference/HintPath")) {
        // Note: The hint-path can contain variables such as $(Configuration) (Debug, Release, etc), so we won't be able to accurately
        // Also, we must not trust that the file (generated output) exists
        fs::path assemblyHintPath = xpathNode.node().first_child().value();
        // Make the hint path absolute
        assemblyHintPath = fs::normalize(fs::complete(assemblyHintPath, projectDir));

        // If this assembly doesn't have a parent project, we have no use for it
        fs::path parentProjectFilePath;
        if (!VsFileLocator::FindParentProjectForAssemblyReference(parentProjectFilePath, assemblyHintPath)) {
            continue;
        }

        // We can't truly know the real assembly file path, so don't blindly make it the actual path to the file
        // TODO: Perhaps figure out the real location of the assembly
        auto assembly = std::make_unique<VsAssembly>("");
        auto assemblyReference = std::make_unique<VsAssemblyReference>(assemblyHintPath.string(), std::move(assembly));
        AssemblyReferences.emplace_back(std::move(assemblyReference));
    }
}

boost::filesystem::path VsProject::getParentDir() const
{
    return FilePath.parent_path();
}

void VsSolution::LoadProjects(VsSolutionPtr selfPtr)
{
    namespace fs = boost::filesystem;
    std::deque<fs::path> files;

    // Find projects in the solution
    const char projectPattern[] = R"!!(^Project\("\{[A-F0-9]{8}-[A-F0-9]{4}-[A-F0-9]{4}-[A-F0-9]{4}-[A-F0-9]{12}\}"\) = "[^"]+", "([^"]+)", "\{[A-F0-9]{8}-[A-F0-9]{4}-[A-F0-9]{4}-[A-F0-9]{4}-[A-F0-9]{12}\}"$)!!";
    std::regex projectRegex(projectPattern, std::regex::icase);
    for (std::sregex_iterator it(m_solutionFileContents.cbegin(), m_solutionFileContents.cend(), projectRegex), end; it != end; ++it) {
        const auto& match(*it);
        const std::string& filePath = match[1].str();
        files.push_back(filePath);
    }

    // Load projects
    for (const auto& relativeFilePath : files) {
        const auto& solutionDir = getParentDir();
        const fs::path filePath(solutionDir / relativeFilePath);

        // Filter by file extension
        std::string extension = filePath.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        if (extension != ".csproj") {
            continue;
        }

        // Skip non-existent files, e.g. virtual solution folders
        if (!fs::exists(filePath)) {
            continue;
        }

        auto project = VsFileRepo::CreateProject(filePath.string(), selfPtr);
        Projects.emplace_back(project);
    }
}

VsSolution::~VsSolution()
{

}

VsSolution::VsSolution(const boost::filesystem::path& filePath) : FilePath(filePath)
{
    nowide::ifstream solutionFile(filePath.string().c_str());
    if (!solutionFile) {
        throw std::runtime_error("Failed to read from file: " + filePath.string());
    }

    std::ostringstream solutionFileContents;
    solutionFileContents << solutionFile.rdbuf();
    m_solutionFileContents = solutionFileContents.str();
}

boost::filesystem::path VsSolution::getParentDir() const
{
    return FilePath.parent_path();
}

bool VsSolutionDependencyManager::TopologicalSortSolutions(VsSolutionList& sortedSolutions, const VsSolutionList& solutions)
{
    namespace fs = boost::filesystem;

    VsSolutionList newSolutionList(solutions.begin(), solutions.end());
    for (auto solution : solutions) {
        for (const auto project : solution->Projects) {
            for (const auto& assemblyReference : project->AssemblyReferences) {
                if (!assemblyReference->ParentProject) {
                    LOG_ERROR() << "Can't sort this solution because a project's assembly reference's parent project is missing. Solution: " << solution->FilePath.filename()
                        << "; Project: " << project->FilePath.filename()
                        << "; Assembly reference's hint-path: " << assemblyReference->HintPath.filename() << "\n";
                    return false;
                }

                auto refProject = assemblyReference->ParentProject;
                auto refSolution = refProject->ParentSolution;
                auto refSolutionIt = std::find(newSolutionList.begin(), newSolutionList.end(), refSolution);
                auto refSolutionOrder = std::distance(newSolutionList.begin(), refSolutionIt);

                auto solutionIt = std::find(newSolutionList.begin(), newSolutionList.end(), solution);
                auto solutionOrder = std::distance(newSolutionList.begin(), solutionIt);

                if (refSolutionOrder > solutionOrder) {
                    newSolutionList.erase(refSolutionIt);
                    newSolutionList.emplace(solutionIt, refSolution);
                }
            }
        }
    }

    sortedSolutions = newSolutionList;
    return true;
}

bool VsSolutionDependencyManager::ResolveAssemblyReferences(VsSolutionList& solutions, bool loadDependencies)
{
    namespace fs = boost::filesystem;

    // We'll append the discovered solutions to the initial ones

    for (auto solution : solutions) {
        for (const auto project : solution->Projects) {
            for (const auto& assemblyReference : project->AssemblyReferences) {
                // We need to find the assembly's parent project
                fs::path parentProjectFilePath;
                if (!VsFileLocator::FindParentProjectForAssemblyReference(parentProjectFilePath, assemblyReference->HintPath)) {
                    continue;
                }

                // We need to find the parent project's parent solution
                fs::path parentSolutionFilePath;
                if (!VsFileLocator::FindParentSolutionForProject(parentSolutionFilePath, parentProjectFilePath)) {
                    continue;
                }

                // We should already have loaded all of the solutions and projects
                auto parentSolutionIt = std::find_if(solutions.begin(), solutions.end(), [&parentSolutionFilePath](const VsSolutionPtr& comparedSolution) -> bool {
                    return fs::equivalent(comparedSolution->FilePath, parentSolutionFilePath);
                });

                VsSolutionPtr parentSolution;
                if (parentSolutionIt != solutions.end()) {
                    // We do know about this solution already
                    parentSolution = *parentSolutionIt;
                }
                else if (loadDependencies) {
                    // Not found in any of the search paths, but since we're discovering dependencies, we'll load them now
                    parentSolution = VsFileRepo::CreateSolution(parentSolutionFilePath);
                    solutions.emplace_back(parentSolution);
                }
                else {
                    continue;
                }

                auto parentProjectIt = std::find_if(parentSolution->Projects.begin(), parentSolution->Projects.end(), [&parentProjectFilePath](const VsProjectPtr& comparedProject) -> bool {
                    return fs::equivalent(comparedProject->FilePath, parentProjectFilePath);
                });

                if (parentProjectIt == parentSolution->Projects.end()) {
                    // Not found for some reason
                    continue;
                }

                auto parentProject = *parentProjectIt;
                assemblyReference->ParentProject = parentProject;
            }
        }
    }

    return true;
}

bool VsFileLocator::FindSolutions(const boost::filesystem::path directoryPath, std::deque<boost::filesystem::path>& files)
{
    namespace fs = boost::filesystem;

    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
        return false;
    }

    for (fs::directory_iterator it(directoryPath), end; it != end; ++it) {
        if (fs::is_directory(it->status())) {
            if (!FindSolutions(it->path(), files)) {
                return false;
            }

            continue;
        }

        // Filter by file extension
        std::string extension = it->path().extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        if (extension == ".sln") {
            files.push_back(it->path());
        }
    }

    return true;
}

VsFileLocator::~VsFileLocator()
{

}

VsFileLocator::VsFileLocator()
{

}

bool VsFileLocator::FindParentProjectForAssemblyReference(boost::filesystem::path& projectFilePath, const boost::filesystem::path& assemblyHintPath)
{
    namespace fs = boost::filesystem;
    fs::path dirtyHintPath(assemblyHintPath);

    // We need to find the referenced assembly's parent project
    // Project files should in normal cases reside in a folder above the the one that contains the assembly itself;
    // for this reason, we can go up one level until we find the project file
    fs::path dirtyTestDir = dirtyHintPath.parent_path();
    fs::path foundRefProjectFilePath;
    bool isDone = false;
    while (!isDone) {
        dirtyTestDir = dirtyTestDir.parent_path();
        if (dirtyTestDir.empty()) {
            // Oops, went up too far in the tree
            // TODO: Improve this so that we never go farther than necessary
            isDone = true;
            continue;
        }

        if (!fs::exists(dirtyTestDir)) {
            // The directory doesn't exist, so it's definitely not the project directory
            continue;
        }

        // Try to find project files
        for (fs::directory_iterator it(dirtyTestDir), end; it != end && !isDone; ++it) {
            if (!fs::is_regular_file(it->status())) {
                continue;
            }

            // Filter by file extension
            std::string extension = it->path().extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            if (extension != ".csproj") {
                continue;
            }

            // This should be the correct project directory
            foundRefProjectFilePath = it->path();
            isDone = true;
        }
    }

    if (foundRefProjectFilePath.empty()) {
        // The assembly's project path wasn't found
        return false;
    }

    projectFilePath = foundRefProjectFilePath;
    return true;
}

bool VsFileLocator::FindParentSolutionForProject(boost::filesystem::path& solutionFilePath, const boost::filesystem::path& projectFilePath)
{
    namespace fs = boost::filesystem;
    // We need to find the project's solution path
    // Solution files should in normal cases either reside alongside the project or reside in a folder above
    fs::path dirtyTestDir = projectFilePath;
    fs::path foundSolutionFilePath;
    bool isDone = false;
    while (!isDone) {
        dirtyTestDir = dirtyTestDir.parent_path();
        if (dirtyTestDir.empty()) {
            // Oops, went up too far in the tree
            // TODO: Improve this so that we never go farther than necessary
            isDone = true;
            continue;
        }

        if (!fs::exists(dirtyTestDir)) {
            // The directory doesn't exist, so it's definitely not the project directory
            continue;
        }

        // Try to find project files
        for (fs::directory_iterator it(dirtyTestDir), end; it != end && !isDone; ++it) {
            if (!fs::is_regular_file(it->status())) {
                continue;
            }

            // Filter by file extension
            std::string extension = it->path().extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            if (extension != ".sln") {
                continue;
            }

            // This should be the correct project directory
            foundSolutionFilePath = it->path();
            isDone = true;
        }
    }

    if (foundSolutionFilePath.empty()) {
        // The project's solution path wasn't found
        return false;
    }

    solutionFilePath = foundSolutionFilePath;
    return true;
}

bool VsFileRepo::HasLoadedSolution(const boost::filesystem::path& filePath)
{
    auto it = m_solutionCache.find(filePath.string());
    return it != m_solutionCache.end();
}

bool VsFileRepo::HasLoadedProject(const boost::filesystem::path& filePath)
{
    auto it = m_projectCache.find(filePath.string());
    return it != m_projectCache.end();
}

void VsFileRepo::RegisterSolution(const boost::filesystem::path& filePath, VsSolutionPtr solution)
{
    m_solutionCache[filePath.string()] = solution;
}

void VsFileRepo::RegisterProject(const boost::filesystem::path& filePath, VsProjectPtr project)
{
    m_projectCache[filePath.string()] = project;
}

VsSolutionPtr VsFileRepo::CreateSolution(const boost::filesystem::path& filePath)
{
    auto solution = std::make_shared<VsSolution>(filePath);
    RegisterSolution(filePath, solution);
    solution->LoadProjects(solution);
    return solution;
}

VsProjectPtr VsFileRepo::CreateProject(const boost::filesystem::path& filePath, VsSolutionPtr parentSolution)
{
    auto project = std::make_shared<VsProject>(filePath, parentSolution);
    RegisterProject(filePath, project);
    return project;
}

VsSolutionPtr VsFileRepo::GetSolution(const boost::filesystem::path& filePath)
{
    return HasLoadedSolution(filePath)
        ? m_solutionCache[filePath.string()]
        : VsSolutionPtr();
}

VsProjectPtr VsFileRepo::GetProject(const boost::filesystem::path& filePath)
{
    return HasLoadedProject(filePath)
        ? m_projectCache[filePath.string()]
        : VsProjectPtr();
}

std::map<std::string, VsProjectPtr> VsFileRepo::m_projectCache;

std::map<std::string, VsSolutionPtr> VsFileRepo::m_solutionCache;
