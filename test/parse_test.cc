#include <iostream>
#include "gtest/gtest.h"
#include "toml/toml.h"

namespace
{
using namespace std;
using namespace toml;

TEST(toml_test, parse_test)
{
    auto view = parse_file("../examples/example.toml").ok();

    EXPECT_TRUE(bool(view));
    EXPECT_EQ(view["title"].value_or(""sv), "TOML Example");
    EXPECT_EQ(view["owner"]["name"].value_or(""sv), "Tom Preston-Werner");

}
} // namespace