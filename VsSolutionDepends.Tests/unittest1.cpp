#include "stdafx.h"
#include "../VsSolutionDependsLib/vs.h"
#include "testmagic.h"

#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace VsSolutionDependsTests
{
TEST_CLASS(UnitTest1)
{
public:

    TEST_METHOD(SolutionOrders)
    {
        /*
        Dependencies:

        S1
            P1
                S2->P3
                S3->P5
            P2

        S2
            P3
                S3->P5
            P4

        S3
            P5
            P6
                S4->P7

        S4
            P7
            P8

        Correct order: S4, S3, S2, S1
        */

        VsSolutionList solutions;

        auto s1 = std::make_shared<VsSolution>("s1.sln");
        auto s2 = std::make_shared<VsSolution>("s2.sln");
        auto s3 = std::make_shared<VsSolution>("s3.sln");
        auto s4 = std::make_shared<VsSolution>("s4.sln");

        auto p1 = std::make_shared<VsProject>("p1\\p1.csproj", s1);
        auto p2 = std::make_shared<VsProject>("p2\\p2.csproj", s1);
        auto p3 = std::make_shared<VsProject>("p3\\p3.csproj", s2);
        auto p4 = std::make_shared<VsProject>("p4\\p4.csproj", s2);
        auto p5 = std::make_shared<VsProject>("p5\\p5.csproj", s3);
        auto p6 = std::make_shared<VsProject>("p6\\p6.csproj", s3);
        auto p7 = std::make_shared<VsProject>("p7\\p7.csproj", s4);
        auto p8 = std::make_shared<VsProject>("p8\\p8.csproj", s4);

        p1->AssemblyReferences.emplace_back(std::make_unique<VsAssemblyReference>(
            "..\\..\\s2\\p3\\bin\\p3.dll",
            std::make_unique<VsAssembly>(""/*, p3*/)));
        p1->AssemblyReferences.emplace_back(std::make_unique<VsAssemblyReference>(
            "..\\..\\s3\\p5\\bin\\p5.dll",
            std::make_unique<VsAssembly>(""/*, p5*/)));

        p3->AssemblyReferences.emplace_back(std::make_unique<VsAssemblyReference>(
            "..\\..\\s3\\p5\\bin\\p5.dll",
            std::make_unique<VsAssembly>(""/*, p5*/)));

        p6->AssemblyReferences.emplace_back(std::make_unique<VsAssemblyReference>(
            "..\\..\\s4\\p7\\bin\\p7.dll",
            std::make_unique<VsAssembly>(""/*, p7*/)));

        s1->Projects.emplace_back(p1);
        s1->Projects.emplace_back(p2);
        s2->Projects.emplace_back(p3);
        s2->Projects.emplace_back(p4);
        s3->Projects.emplace_back(p5);
        s3->Projects.emplace_back(p6);
        s4->Projects.emplace_back(p7);
        s4->Projects.emplace_back(p8);

        solutions.emplace_back(s1);
        solutions.emplace_back(s2);
        solutions.emplace_back(s3);
        solutions.emplace_back(s4);

        // Discover dependencies
        VsSolutionHelper::ResolveAssemblyReferences(solutions, false);

        // Reorder solutions
        VsSolutionList solutionOrders;
        Assert::IsTrue(VsSolutionHelper::TopologicallySortSolutions(solutionOrders, solutions));

        // Assert the correct order
        auto solutionOrderIt = solutionOrders.cbegin();
        Assert::AreEqual<const VsSolutionPtr>(s4, *(solutionOrderIt++));
        Assert::AreEqual<const VsSolutionPtr>(s3, *(solutionOrderIt++));
        Assert::AreEqual<const VsSolutionPtr>(s2, *(solutionOrderIt++));
        Assert::AreEqual<const VsSolutionPtr>(s1, *(solutionOrderIt++));
    }

    TEST_METHOD(FindSolutionsInFileSystem)
    {
        std::deque<std::string> searchDirs;
        searchDirs.push_back("C:\\Users\\Steffen\\Desktop\\RA_vs2013\\COMMON");
        searchDirs.push_back("C:\\Users\\Steffen\\Desktop\\RA_vs2013\\SafeMedicalApplications");

        std::deque<boost::filesystem::path> files;
        for (const auto& searchDir : searchDirs) {
             Assert::IsTrue(VsFileLocator::FindSolutions(files, searchDir, true));
        }

        Assert::IsTrue(files.size() > 0);
    }

    TEST_METHOD(Everything)
    {
        std::deque<std::string> searchDirs;
        searchDirs.push_back("..\\TestData\\Solutions\\Root1");
        searchDirs.push_back("..\\TestData\\Solutions\\Root2");

        {
            std::deque<boost::filesystem::path> solutionFiles;
            for (const auto& searchDir : searchDirs) {
                Assert::IsTrue(VsFileLocator::FindSolutions(solutionFiles, searchDir, true));
            }

            Assert::IsTrue(!solutionFiles.empty());

            {
                VsSolutionList solutions;
                for (const auto& solutionFilePath : solutionFiles) {
                    auto solution = VsFileRepository::CreateSolution(solutionFilePath);
                    solutions.emplace_back(solution);
                }

                VsSolutionHelper::RemoveUnresolvableAssemblyReferences(solutions);
                // Discover dependencies
                VsSolutionHelper::ResolveAssemblyReferences(solutions, false);
                VsSolutionHelper::RemoveUnresolvableAssemblyReferences(solutions);

                // Reorder solutions
                VsSolutionList solutionOrders;
                Assert::IsTrue(VsSolutionHelper::TopologicallySortSolutions(solutionOrders, solutions));

                // Print solution orders
                Logger::WriteMessage("Insertion order:");
                for (auto solution : solutions) {
                    Logger::WriteMessage(solution->FilePath.string().c_str());
                }

                Logger::WriteMessage("New order:");
                for (auto solution : solutionOrders) {
                    Logger::WriteMessage(solution->FilePath.string().c_str());
                }
            }
        }
    }
};
}