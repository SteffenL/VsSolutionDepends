#include "DependencyGraphFormatter.h"
#include "../VsSolutionDependsLib/log.h"
#include "../VsSolutionDependsLib/filesystem.h"

#include <iostream>
#include <boost/filesystem.hpp>

DependencyGraphFormatter::DependencyGraphFormatter(const VsSolutionList& solutions)
    : m_solutions(solutions)
{
}

bool DependencyGraphFormatter::AsFlatList(std::ostream& stream, const FlatListOptions& options) const
{
    namespace fs = boost::filesystem;

    VsSolutionList sortedSolutions;
    if (!VsSolutionDependencyHelper::TopologicalSortSolutions(sortedSolutions, m_solutions)) {
        LOG_ERROR() << "Failed to sort the solutions.\n";
        return false;
    }

    for (const auto solution : sortedSolutions) {
        const fs::path& filePath = (options.Flags & FlatListOptions::Flag::UseBaseDir)
            ? fs::make_relative(options.BaseDir, solution->FilePath)
            : solution->FilePath;
        stream << filePath.string() << std::endl;
    }

    return true;
}
