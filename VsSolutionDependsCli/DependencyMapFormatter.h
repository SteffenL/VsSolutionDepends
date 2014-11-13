#pragma once

#include "../VsSolutionDependsLib/vs.h"

#include <boost/filesystem.hpp>

#include <string>
#include <iosfwd>

class DependencyMapFormatter
{
public:
    struct FlatListOptions
    {
        struct Flag
        {
            enum Type
            {
                None = 0,
                UseBaseDir = 0x01
            };
        };

        FlatListOptions(unsigned int flags = Flag::None) : Flags(flags) {}

        unsigned int Flags;
        boost::filesystem::path BaseDir;
    };

    DependencyMapFormatter(const VsSolutionList& solutions);
    bool AsFlatList(std::ostream& stream, const FlatListOptions& options) const;

private:
    const VsSolutionList& m_solutions;
};
