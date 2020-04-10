#include <iostream>
#include "gtest/gtest.h"
#include "toml/toml.h"

namespace
{
using namespace std;
using namespace toml;

TEST(node_test, base_include)
{
    vector<shared_ptr<node>> data;
    data.push_back(make_value<int64_t>(2));
    data.push_back(make_value<std::string>("haha"));

    cout << bool(data[0]) << endl;
    cout << data[0]->is_value<int32_t>() << endl;
    cout << data[0]->is_value<bool>() << endl;
    cout << data[0]->is_value() << endl;
    cout << data[1]->is_value<std::string>() << endl;
    cout << data[1]->is_value<std::string_view>() << endl;

    auto array = make_array();
    array->emplace_back<double>(0.5);
    cout << (*array)[0]->is_value<float>() << " " << *(*array)[0]->value<float>() << endl;
    data.push_back(array);

    cout << data[2]->is_value<int64_t>() << endl;
    cout << data[2]->is_value<std::string_view>() << endl;
    cout << data[2]->is_array() << endl;
    cout << "hehehehehehehehe" << endl;
    cout << data[2]->as_array()->at(0)->value_or(120) << endl;

    auto view = parse_file("/tmp/123").ok();
    cout << "haha " << *view["haha"].value<int>() << endl;
}
} // namespace