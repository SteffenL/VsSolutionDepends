#pragma once

#include "..\VsSolutionDepends\vs.h"
#include "CppUnitTest.h"

namespace Microsoft { namespace VisualStudio { namespace CppUnitTestFramework {

template<> static std::wstring ToString<VsSolutionPtr>(const VsSolutionPtr& t) { RETURN_WIDE_STRING(t); }

}}}
