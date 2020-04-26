#include <iostream>
#include "toml/toml.h"

int main(int argc, char *argv[])
{
    auto root = toml::make_table();
    root->emplace("Integer", 1234L);
    root->emplace("Double", 1.234);
    root->emplace("String", std::string("ABCD"));

    auto table = toml::make_table();
    table->emplace("ElementOne", 1L);
    table->emplace("ElementTwo", 2.0);
    table->emplace("ElementThree", std::string("THREE"));

    auto nested_table = toml::make_table();
    nested_table->emplace("ElementOne", 2L);
    nested_table->emplace("ElementTwo", 3.0);
    nested_table->emplace("ElementThree", std::string("FOUR"));

    table->emplace("Nested", nested_table);

    root->emplace("Table", table);

    auto int_array = toml::make_array();
    int_array->emplace_back(1L);
    int_array->emplace_back(2.0);
    int_array->emplace_back(3L);
    int_array->emplace_back(4);
    int_array->emplace_back(5L);

    root->emplace("IntegerArray", int_array);

    auto double_array = toml::make_array();
    double_array->emplace_back(1.1);
    double_array->emplace_back(2.2);
    double_array->emplace_back(3.3);
    double_array->emplace_back(4.4);
    double_array->emplace_back(5.5);

    root->emplace("DoubleArray", double_array);

    auto string_array = toml::make_array();
    string_array->emplace_back(std::string("A"));
    string_array->emplace_back(std::string("B"));
    string_array->emplace_back(std::string("C"));
    string_array->emplace_back(std::string("D"));
    string_array->emplace_back(std::string("E"));

    root->emplace("StringArray", string_array);

    auto table_array = toml::make_array();
    table_array->emplace_back(table);
    table_array->emplace_back(table);
    table_array->emplace_back(table);

    root->emplace("TableArray", table_array);

    auto array_of_arrays = toml::make_array();
    array_of_arrays->emplace_back(int_array);
    array_of_arrays->emplace_back(double_array);
    array_of_arrays->emplace_back(string_array);

    root->emplace("ArrayOfArrays", array_of_arrays);

    std::cout << (*root);
    return 0;
}
