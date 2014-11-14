#include "DependencyMapFormatter.h"
#include "../VsSolutionDependsLib/log.h"
#include "../VsSolutionDependsLib/filesystem.h"

#include <iostream>
#include <boost/filesystem.hpp>

DependencyMapFormatter::DependencyMapFormatter(const VsSolutionList& solutions)
    : m_solutions(solutions)
{
}

bool DependencyMapFormatter::AsFlatList(std::ostream& stream, const FlatListOptions& options) const
{
    namespace fs = boost::filesystem;

    VsSolutionList sortedSolutions;
    if (!VsSolutionHelper::TopologicallySortSolutions(sortedSolutions, m_solutions)) {
        LOG_ERROR() << "Failed to sort the solutions.\n";
        return false;
    }

    for (const auto solution : sortedSolutions) {
        fs::path filePath = (options.Flags & FlatListOptions::Flag::UseBaseDir)
            ? fs::make_relative(options.BaseDir, solution->FilePath)
            : solution->FilePath;
        filePath.make_preferred();
        stream << filePath.string() << std::endl;
    }

    return true;
}
