#include <iostream>
#include "toml/toml.h"

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " filename" << std::endl;
        return 1;
    }

    if (auto result = toml::parse_file(argv[1]); result.is_ok())
    {
        auto view = result.ok();
        std::cout << view << std::endl;
    }
    else
    {
        std::cerr << "Failed to parse " << argv[1] << ": " << result.err().what() << std::endl;
        return 1;
    }

    return 0;
}
