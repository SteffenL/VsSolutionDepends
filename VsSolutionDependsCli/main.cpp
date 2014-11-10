#include "..\VsSolutionDepends\vs.h"
#include "..\VsSolutionDepends\filesystem.h"

#include <nowide/args.hpp>

#include <filesystem>
#include <cstdlib>

int main(int argc, char* argv[])
{
    nowide::args(argc, argv);
    namespace fs = std::tr2::sys;

    if (argc < 2) {
        std::printf("Not enough arguments specified.\n");
        return 1;
    }

    std::deque<std::string> searchDirs;
    for (int i = 1; i < argc; ++i) {
        searchDirs.push_back(fs::normalize(fs::complete(fs::path(argv[i]))));
    }

    try
    {
        std::deque<std::tr2::sys::path> solutionFiles;
        for (const auto& searchDir : searchDirs) {
            VsFileLocator locator;
            if (!locator.FindSolutions(searchDir, solutionFiles)) {
                std::printf("Failed to find all solutions.\n");
                return 1;
            }
        }

        if (solutionFiles.size() == 0) {
            std::printf("No solutions found.\n");
            return 1;
        }

        {
            VsSolutionList solutions;
            for (const auto& solutionFilePath : solutionFiles) {
                auto solution = VsFileRepo::CreateSolution(solutionFilePath);
                solutions.emplace_back(solution);
            }

            // Discover dependencies
            VsSolutionDependencyManager::DiscoverDependencies(solutions);

            // Reorder solutions
            VsSolutionList solutionsWithDependencies(solutions.begin(), solutions.end());
            solutionsWithDependencies = VsSolutionDependencyManager::ReorderDependencies(solutionsWithDependencies);

            // Print solution orders
            /*std::printf("Insertion order:\n");
            for (auto solution : solutions) {
                std::wprintf(L"%s\n", nowide::widen(solution->FilePath.string()).c_str());
            }

            std::printf("New order:\n");*/
            for (auto solution : solutionsWithDependencies) {
                std::wprintf(L"%s\n", nowide::widen(solution->FilePath.string()).c_str());
            }
        }

        return 0;
    }
    catch (std::runtime_error& ex) {
        std::printf("Error: %s\n", ex.what());
        return 1;
    }
    catch (std::exception& ex) {
        std::printf("Unhandled exception: %s\n", ex.what());
        return 1;
    }
}
