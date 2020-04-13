#include <iostream>
#include "gtest/gtest.h"
#include "toml/toml.h"

namespace
{
using namespace std;
using namespace toml;

TEST(toml_test, parse_example)
{
    auto view = parse_file("../examples/example.toml").ok();

    EXPECT_TRUE(bool(view));
    EXPECT_EQ(view["title"].value_or(""sv), "TOML Example"sv);
    EXPECT_EQ(view["owner"]["name"].value_or(""sv), "Tom Preston-Werner"sv);

    EXPECT_TRUE(view["owner"]["dob"].map<offset_date_time>([](const auto &val) {
                                        return val.year == 1979 &&
                                               val.day == 27 &&
                                               val.minute == 32 &&
                                               val.minute_offset == -480;
                                    })
                    .value());

    EXPECT_FALSE(view["database"]["enabled"].map<bool>([](const auto &val) {
                                                return !val;
                                            })
                     .value());

    EXPECT_EQ(view["clients"][0]["data"][0][0].value_or(""sv), "gamma"sv);
    EXPECT_EQ(view["clients"][0]["data"][0].collect<std::string_view>(),
              (std::vector{"gamma"sv, "delta"sv}));
    EXPECT_EQ(view["database"]["ports"].map_collect<int>(
                  [](const auto &val) {
                      return val - 1;
                  }),
              (std::vector{8000, 8000, 8001}));
}

TEST(toml_test, parse_array)
{
    static constexpr auto source = R"(
        numbers = [ 1, 2, 3, "four", 5.0 ]

        [animals]
        cats = [ "tiger", "lion", "puma" ]
    )";

    auto view = toml::parse(source).ok();

    EXPECT_EQ(view["numbers"].collect<double>(),
              (std::vector{1.0, 2.0, 3.0, 5.0}));

    EXPECT_EQ(view["numbers"].collect<int>(),
              (std::vector{1, 2, 3}));

    EXPECT_EQ(view["animals"]["cats"].collect<std::string_view>(),
              (std::vector{"tiger"sv, "lion"sv, "puma"sv}));

    EXPECT_EQ(view["animals"]["cats"].map_collect<std::string_view>(
                  [](const auto &val) {
                      return val.substr(0, 2);
                  }),
              (std::vector{"ti"sv, "li"sv, "pu"sv}));
}
} // namespace