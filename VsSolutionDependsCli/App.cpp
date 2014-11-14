#include "App.h"
#include "DependencyMapFormatter.h"
#include "../VsSolutionDependsLib/vs.h"
#include "../VsSolutionDependsLib/filesystem.h"
#include "../VsSolutionDependsLib/log.h"

#include <nowide/convert.hpp>
#include <nowide/iostream.hpp>
#include <nowide/fstream.hpp>

#include <boost/filesystem.hpp>
#include <cstdlib>
#include <deque>
#include <map>
#include <iomanip>

App::CmdLineArgs_t::CmdLineArgs_t(TCLAP::CmdLine& cmd)
    : m_outputFormatSpecific(cmd)
{
    m_outputFormatAllowedValues = std::vector < std::string > { "flat" };
    m_outputFormatAllowedMap = std::map<std::string, DependencyMapOutputFormat>{
            { "flat", DependencyMapOutputFormat::FlatList }
    };

    m_outputFormatConstraint = std::make_unique<TCLAP::ValuesConstraint<std::string>>(m_outputFormatAllowedValues);

    m_withoutDependencies = std::make_unique<TCLAP::SwitchArg>("", "without-dependencies", "Don't discover and load dependencies resolved from assembly references.", cmd, false);
    m_searchDirs = std::make_unique<TCLAP::MultiArg<std::string>>("d", "search-dir", "A root directory in which to search for solutions.", true, "string", cmd);
    m_outputFormat = std::make_unique<TCLAP::ValueArg<std::string>>("f", "output-format", "Dependency map output format.", false, "flat", m_outputFormatConstraint.get(), cmd);
    m_outputFilePath = std::make_unique<TCLAP::ValueArg<std::string>>("o", "output-file", "Output file path.", true, "", "string", cmd);
    m_verbose = std::make_unique<TCLAP::SwitchArg>("v", "verbose", "Be verbose in the output (useful info, warnings, etc).", cmd, false);
    m_logFilePath = std::make_unique<TCLAP::ValueArg<std::string>>("", "log", "Logs to a file; useful for troubleshooting.", false, "", "string", cmd);
}

bool App::CmdLineArgs_t::GetWithoutDependencies() const
{
    return m_withoutDependencies->getValue();
}

const std::vector<std::string>& App::CmdLineArgs_t::GetSearchDirs() const
{
    return m_searchDirs->getValue();
}

App::DependencyMapOutputFormat App::CmdLineArgs_t::GetOutputFormat() const
{
    return m_outputFormatAllowedMap.at(m_outputFormat->getValue());
}

std::string App::CmdLineArgs_t::GetOutputFilePath() const
{
    return m_outputFilePath->getValue();
}

bool App::CmdLineArgs_t::GetVerbose() const
{
    return m_verbose->getValue();
}

const App::CmdLineArgs_t::OutputFormatSpecific_t& App::CmdLineArgs_t::GetOutputFormatSpecific() const
{
    return m_outputFormatSpecific;
}

std::string App::CmdLineArgs_t::GetLogFilePath() const
{
    return m_logFilePath->getValue();
}


App::CmdLineArgs_t::OutputFormatSpecific_t::OutputFormatSpecific_t(TCLAP::CmdLine& cmd)
    : m_flatList(cmd)
{
}

const App::CmdLineArgs_t::OutputFormatSpecific_t::FlatList_t& App::CmdLineArgs_t::OutputFormatSpecific_t::GetFlatList() const
{
    return m_flatList;
}

App::CmdLineArgs_t::OutputFormatSpecific_t::FlatList_t::FlatList_t(TCLAP::CmdLine& cmd)
{
    m_baseDir = std::make_unique<TCLAP::ValueArg<std::string>>("", "output-format-flat-base-dir", "Make relative file paths using this directory as the base.", false, "", "string", cmd);
}

std::string App::CmdLineArgs_t::OutputFormatSpecific_t::FlatList_t::GetBaseDir() const
{
    return m_baseDir->getValue();
}


App::App(int argc, char* argv[]) : m_argc(argc), m_argv(argv) {}

int App::Run()
{
    try {
        parseCmdLine();

        Logging::SetLogFilePath(m_cmdLineArgs->GetLogFilePath());
        Logging::EnableLogging(!m_cmdLineArgs->GetLogFilePath().empty());

        namespace fs = boost::filesystem;

        std::deque<fs::path> searchDirs;
        for (const auto& arg : m_cmdLineArgs->GetSearchDirs()) {
            searchDirs.push_back(fs::normalize(fs::complete(fs::path(arg))));
        }

        std::deque<fs::path> solutionFiles;
        for (const auto& searchDir : searchDirs) {
             if (!VsFileLocator::FindSolutions(solutionFiles, searchDir, true)) {
                nowide::cerr << "Failed to find all solutions.\n";
                return 1;
            }
        }

        if (solutionFiles.size() == 0) {
            nowide::cout << "No solutions found.\n";
            return 1;
        }

        VsSolutionList solutions;
        for (const auto& solutionFilePath : solutionFiles) {
            auto solution = VsFileRepository::CreateSolution(solutionFilePath);
            solutions.emplace_back(solution);
        }

        // Remove unresolvable assembly references so that we don't spend any extra time trying to resolve them
        VsSolutionHelper::RemoveUnresolvableAssemblyReferences(solutions);
        // Resolve assembly references (and dependencies if needed)
        if (VsSolutionHelper::ResolveAssemblyReferences(solutions, !m_cmdLineArgs->GetWithoutDependencies())) {
            // Remove unresolvable assembly references again
            VsSolutionHelper::RemoveUnresolvableAssemblyReferences(solutions);
        }

        if (m_cmdLineArgs->GetVerbose()) {
            // Display gathered information
            nowide::cout << "Information gathered so far:\n";
            for (auto solution : solutions) {
                nowide::cout << std::setw(2) << "" << "Solution: " << solution->FilePath.filename().string() << "\n";

                for (const auto project : solution->Projects) {
                    if (project->ParentSolution) {
                        nowide::cout << std::setw(4) << "" << "Project: " << project->FilePath.filename().string() << "\n";
                    }
                    else {
                        nowide::cout << std::setw(4) << "" << "Project's parent solution couldn't be resolved: " << solution->FilePath.filename().string() << "\n";
                    }

                    for (const auto& assemblyReference : project->AssemblyReferences) {
                        if (assemblyReference->ParentProject) {
                            nowide::cout << std::setw(6) << "" << "Assembly reference: " << assemblyReference->HintPath.filename().string() << "\n";
                        }
                        else {
                            nowide::cout << std::setw(6) << "" << "Assembly reference's parent project couldn't be resolved: " << assemblyReference->HintPath.filename().string() << "\n";
                        }
                    }
                }
            }
        }

        if (!generateOutput(solutions)) {
            return 1;
        }
    }
    catch (std::runtime_error& ex) {
        nowide::cerr << ex.what();
        return 1;
    }

    return 0;
}

void App::parseCmdLine()
{
    try {
        // Parse command line
        TCLAP::CmdLine cmd("Scans directories for Visual Studio solutions, then attempts to resolve their dependency solutions.");
        m_cmdLineArgs = std::make_unique<CmdLineArgs_t>(cmd);
        cmd.parse(m_argc, m_argv);
    }
    catch (TCLAP::ArgException&) {
        throw;
    }
}

bool App::formatDependencyMap(std::ostream& outStream, DependencyMapOutputFormat format, const VsSolutionList& solutions, const DependencyMapOutputOptions& options)
{
    DependencyMapFormatter formatter(solutions);

    switch (format) {
        case DependencyMapOutputFormat::FlatList:
            return formatter.AsFlatList(outStream, options.FlatList);

        default:
            throw std::logic_error("Invalid output format");
    }

    return false;
}

bool App::generateOutput(const VsSolutionList& solutions)
{
    namespace fs = boost::filesystem;
    DependencyMapOutputOptions options;

    switch (m_cmdLineArgs->GetOutputFormat()) {
    case DependencyMapOutputFormat::FlatList:
        setupFlatListOptions(options.FlatList, solutions);
        break;

    default:
        throw std::logic_error("Invalid output format");
    }

    // Generate desired output
    nowide::ofstream outStream(m_cmdLineArgs->GetOutputFilePath().c_str(), std::ios::binary);
    if (!formatDependencyMap(outStream, m_cmdLineArgs->GetOutputFormat(), solutions, options)) {
        nowide::cerr << "Failed to generate a formatted dependency map." << std::endl;
        // Don't leave failed results
        outStream.close();
        fs::remove(fs::path(m_cmdLineArgs->GetOutputFilePath()));
        return false;
    }

    return true;
}

void App::setupFlatListOptions(DependencyMapFormatter::FlatListOptions& options, const VsSolutionList& solutions)
{
    const auto& flatListArgs = m_cmdLineArgs->GetOutputFormatSpecific().GetFlatList();
    if (!flatListArgs.GetBaseDir().empty()) {
        options.Flags |= DependencyMapFormatter::FlatListOptions::Flag::UseBaseDir;
        options.BaseDir = flatListArgs.GetBaseDir();
    }
}
