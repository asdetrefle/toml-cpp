# toml
A header-only C++17 library for parsing [TOML][toml] configuration files.

Targets: [TOML v1.0.0-rc.1][currver] as of April 2020.

## Motivation:

This projected was previously a fork of [cpptoml][cpptoml], but rewritten
according to the new released standard [v1.0.0-rc.1][currver]. This 
includes support for features like mixed type arrays.

Many of the concepts and implementation are brought from [toml++][tomlplusplus]
which seems a bit heavy and complicated for my use case. [cpptoml][cpptoml] was
perfect while some features from [toml++][tomlplusplus] are really attractive, 
for example the idea of `node_view` for chaining query of nested tables. So I 
decide to change a bit [cpptoml][cpptoml] and make it what I think is best. 
But any contributions or suggestions are very welcome!

C++ Alternatives:
- [toml++][tomlplusplus] is a C++17/20 implementation of a TOML parser,
  which also supports v1.0.0-rc.1 as of writing.
- [cpptoml][cpptoml] is the former version of this project. It supports v0.5.0 currently.
- [Boost.toml][boost.toml] is an implementation of a TOML parser using
  the Boost library. As of December 2019, it supports v0.5.0 as well.
- [ctoml][ctoml] is a C++11 implementation of a TOML parser, but only
  supports v0.2.0.
- [tinytoml][tinytoml] is a C++11 implementation of a TOML parser, which
  also supports v0.4.0 as of November 2015.

## Build Status

## Example Usage
To parse a configuration file from a file, you can do the following:

```cpp
#include "toml/toml.h"

/**
 * `parse_file()` returns a `node_view` of a `toml::table`, which owns
 * a copy of `std::shared_ptr<table>` so there is no lifetime issue.
 * It will return an empty view if it catches an `toml::parse_error`.
 * */
auto config = cpptoml::parse_file("examples/example.toml").ok();

// ## Obtaining Basic Values
auto title = config["title"].value_or(""sv); // "TOML Example"sv
// or you want to have a `std::optional<std::string_view>`
auto optional_title = config["title"].value<std::string_view>().value();

/** ## Nested Tables
 * Nested tables can be queried directly by chaining `[]` operator through
 * the `node_view` and you can use a `map` function to convert to your data type
 * */
auto author = config["owner"]["name"].value_or(""sv); // "Tom Preston-Werner"

// empty `std::optional<T>` will be mapped if node does not exist
auto dob = config["owner"]["dob"].map<offset_date_time>([](const auto &val) {
    return val.year * 10000 + val.month * 100 + val.day;
}); // std::optional<19790527>

/** ## Arrays of Values
 * Similarly to `toml::table`, you can access the `node` of a table by `[]`
 * operator. The are also `collect` and `map_collect` provided for convenience:
 * */

auto gamma = view["clients"]["data"][0][0].value_or(""sv);
// std::vector{"gamma"sv, "delta"sv}
auto data0 = view["clients"]["data"][0].collect<std::string_view>();
// std::vector{8000, 8000, 8001}
auto ports = view["database"]["ports"].map_collect<int>([](const auto &val) {
                                                            return val - 1;
                                                        });

/**
 * ## Arrays of Tables is the same as arrays
 * /*
```

toml has extended support for dates and times beyond the TOML v0.4.0
spec. Specifically, it supports

- Local Date (`local_date`), which simply represents a date and lacks any time
  information, e.g. `1980-08-02`;
- Local Time (`local_time`), which simply represents a time and lacks any
  date or zone information, e.g. `12:10:03.001`;
- Local Date-time (`local_date_time`), which represents a date and a time,
  but lacks zone information, e.g. `1980-08-02T12:10:03.001`;
- and Offset Date-time (`offset_date_time`), which represents a date, a
  time, and timezone information, e.g. `1980-08-02T12:10:03.001-07:00`

Here are the fields of the date/time objects in cpptoml:

- year (`local_date`, `local_date_time`, `offset_date_time`)
- month (`local_date`, `local_date_time`, `offset_date_time`)
- day (`local_date`, `local_date_time`, `offset_date_time`)
- hour (`local_time`, `local_date_time`, `offset_date_time`)
- minute (`local_time`, `local_date_time`, `offset_date_time`)
- second (`local_time`, `local_date_time`, `offset_date_time`)
- nanosecond (`local_time`, `local_date_time`, `offset_date_time`)
- minute\_offset (`offset_date_time`)


## Test Results

From [the toml-test suite][toml-test]:

```
126 passed, 0 failed // TODO with TOML v1.0.0-rc.1
```

## Compilation
Requires a well conforming C++17 compiler. This project is not going to seek backward
compatibility for g++ < 9.x. Currently only tested on Linux.

Compiling the examples can be done with cmake:

```
mkdir build
cd build
cmake ../
make
```

## More Examples
You can look at the files files `parse.cpp`, `parse_stdin.cpp`, and
`build_toml.cpp` in the root directory for some more examples.

`parse_stdin.cpp` shows how to use the visitor pattern to traverse an
entire `cpptoml::table` for serialization.

`build_toml.cpp` shows how to construct a TOML representation in-memory and
then serialize it to a stream.

[currver]: https://github.com/toml-lang/toml/blob/master/versions/en/toml-v1.0.0-rc.1.md
[cpptoml]: https://github.com/skystrife/cpptoml
[toml]: https://github.com/toml-lang/toml
[toml-test]: https://github.com/BurntSushi/toml-test
[toml-test-fork]: https://github.com/skystrife/toml-test
[ctoml]: https://github.com/evilncrazy/ctoml
[libtoml]: https://github.com/ajwans/libtoml
[tinytoml]: https://github.com/mayah/tinytoml
[boost.toml]: https://github.com/ToruNiina/Boost.toml
[tomlplusplus]: https://github.com/marzer/tomlplusplus
