#include <cstdint>
#include <iostream>
#include "toml/toml.h"

int main()
{
    auto root = toml::make_table();

    root->emplace("small-integer", int64_t{12});

    auto si = root->at("small-integer")->as<int16_t>().value();
    std::cout << "small-integer " << si << " type converted " << typeid(si).name() << std::endl;
    root->emplace("small-integer2", si);

    try
    {
        root->emplace("too-big", std::numeric_limits<uint64_t>::max());
        auto tb = root->at("too-big")->as<int64_t>().value();
        std::cout << "too-big " << tb << std::endl;
    }
    catch (std::overflow_error &e)
    {
        std::cerr << "too-big overflow catched " << e.what() << std::endl;
    }

    root->emplace("medium-integer", std::numeric_limits<int32_t>::max());
    try
    {
        auto mi = root->at("medium-integer")->as<int16_t>().value();
        std::cout << "medium-integer " << mi << std::endl;
    }
    catch (std::overflow_error &e)
    {
        std::cerr << "medium-integer overflow catched " << e.what() << std::endl;
    }

    auto mi = root->at("medium-integer")->as<uint32_t>().value(); // signed as unsigned, checked
    std::cout << "medium-integer unsigned " << mi << std::endl;

    root->emplace("medium-negative", std::numeric_limits<int32_t>::min());

    try
    {
        root->at("medium-negative")->as<int16_t>();
    }
    catch (std::underflow_error & e)
    {
        std::cerr << "medium-negative underflow catched " << e.what() << std::endl;
    }

    try
    {
        root->at("medium-negative")->as<uint64_t>();
    }
    catch (std::underflow_error &e)
    {
        std::cerr << "medium-negative underflow catched " << e.what() << std::endl;
    }

    root->emplace("float", 0.1f);
    std::cout << "float as double " << root->at("float")->as<double>().value() << std::endl;

    return 0;
}
