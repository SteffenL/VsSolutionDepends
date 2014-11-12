#include "App.h"

#include <nowide/args.hpp>
#include <nowide/iostream.hpp>

#include <stdexcept>

int main(int argc, char* argv[])
{
    int retCode = 0;
    try {
        nowide::args args(argc, argv);
        retCode = App(argc, argv).Run();
    }
    catch (std::exception& ex) {
        nowide::cerr << "Unhandled exception: " << ex.what() << "\n";
        retCode = 1;
    }

    nowide::cout << std::flush;
    nowide::cerr << std::flush;
    return retCode;
}
