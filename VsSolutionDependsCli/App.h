#pragma once

#include "../VsSolutionDependsLib/vs.h"
#include "DependencyMapFormatter.h"

#include <tclap/CmdLine.h>

#include <string>
#include <vector>
#include <map>
#include <memory>

class App
{
    enum class DependencyMapOutputFormat
    {
        FlatList
    };

    struct DependencyMapOutputOptions
    {
        DependencyMapFormatter::FlatListOptions FlatList;
    };

    struct CmdLineArgs_t
    {
        struct OutputFormatSpecific_t
        {
            struct FlatList_t
            {
                FlatList_t(TCLAP::CmdLine& cmd);
                std::string GetBaseDir() const;

            private:
                std::unique_ptr<TCLAP::ValueArg<std::string>> m_baseDir;
            };

            OutputFormatSpecific_t(TCLAP::CmdLine& cmd);
            const FlatList_t& GetFlatList() const;

        private:
            FlatList_t m_flatList;
        };

        CmdLineArgs_t(TCLAP::CmdLine& cmd);

        bool GetWithoutDependencies() const;
        const std::vector<std::string>& GetSearchDirs() const;
        DependencyMapOutputFormat GetOutputFormat() const;
        std::string GetOutputFilePath() const;
        bool GetVerbose() const;
        const OutputFormatSpecific_t& GetOutputFormatSpecific() const;

    private:
        std::unique_ptr<TCLAP::SwitchArg> m_withoutDependencies;
        std::unique_ptr<TCLAP::MultiArg<std::string>> m_searchDirs;

        std::unique_ptr<TCLAP::ValueArg<std::string>> m_outputFormat;
        std::unique_ptr<TCLAP::ValuesConstraint<std::string>> m_outputFormatConstraint;
        std::vector<std::string> m_outputFormatAllowedValues;
        std::map<std::string, DependencyMapOutputFormat> m_outputFormatAllowedMap;
        std::unique_ptr<TCLAP::SwitchArg> m_verbose;

        std::unique_ptr<TCLAP::ValueArg<std::string>> m_outputFilePath;

        OutputFormatSpecific_t  m_outputFormatSpecific;
    };

public:
    App(int argc, char* argv[]);
    int Run();

private:
    void parseCmdLine();
    int m_argc;
    char** m_argv;

    bool formatDependencyMap(std::ostream& outStream, DependencyMapOutputFormat format, const VsSolutionList& solutions, const DependencyMapOutputOptions& options);
    bool generateOutput(const VsSolutionList& solutions);
    void setupFlatListOptions(DependencyMapFormatter::FlatListOptions& options, const VsSolutionList& solutions);
    std::unique_ptr<CmdLineArgs_t> m_cmdLineArgs;
};
