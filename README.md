# tominal - Minimal TOML parser and writer
A header-only C++17 library for parsing and writing [TOML][toml] configuration files.

Targets: [TOML v1.0.0-rc.1][currver] as of April 2020. [![Build Status](https://travis-ci.com/asdetrefle/tominal.svg?branch=master)](https://travis-ci.com/github/asdetrefle/tominal)

## Motivation:

This project was previously a fork of [cpptoml][cpptoml], but rewritten (partially) according to the new released standard [v1.0.0-rc.1][currver]. This includes support for features like mixed type arrays.

Many of the concepts and implementation are brought from [toml++][tomlplusplus]. Being an awesome
C++ library for TOML, it seems a bit heavy and complicated for my use case. [cpptoml][cpptoml] was perfect but I really want to add the idea of `node_view` from [toml++][tomlplusplus] so the user can chain `[]` operator to query nested tables and arrays. Any contributions or suggestions are very welcome!



C++ Alternatives:
- [toml++][tomlplusplus] is a C++17/20 implementation of a TOML parser, which also supports v1.0.0-rc.1 as of writing.
- [toml11][toml11] is a C++11/14/17 implementation of a TOML parser which supports v1.0.0-rc.1 as well.
- [cpptoml][cpptoml] is the former version of this project. It supports v0.5.0 currently.

## Build Status

## Example Usage
To parse a TOML document from a file, you can do the following:

```cpp
#include "tominal/toml.h"

// ## Parsing
// `parse_file()` returns a `node_view` of a `toml::table`, which owns
// a copy of `std::shared_ptr<table>` so there is no lifetime issue.
// It will return an empty view if it catches an `toml::parse_error`.
auto config = toml::parse_file("examples/example.toml").ok();

// ## Obtaining Basic Values
auto title = config["title"].value_or("TOML"sv); // "TOML Example"sv
// or to have the default `std::string_view` if node_view is empty
auto default_title = config["title"].value_or_default<std::string_view>();

// ## Nested Tables
// Nested tables can be queried directly by chaining `[]` operator through
// the `node_view` and you can use a `map` function to convert to your data type
auto author = config["owner"]["name"].value_or("TOML");
auto enabled = config["database.enabled"].value_or_default<bool>();

// map a node_view in place if it exists:
auto dob = config["owner.dob"].map<offset_date_time>([](const auto &val) {
    return val.year * 10000 + val.month * 100 + val.day;
});
config["owner.dob"].map<local_date>([](const auto &val) {}); // void callback


// ## Arrays of Values / Tables
// Similarly to `toml::table`, you can access the `node` of a table by `[]`
// operator. The are also `collect` and `map_collect` provided for convenience:
auto gamma = config["clients"][0]["data"][1][1].value_or_default<int>();
auto data0 = config["clients"][1]["host"].collect<std::string_view>(); // std::vector{"omega"sv}
auto ports = config["database"]["ports"].map_collect<int>([](const auto &val) {
    return val - 8000; // std::vector{0, 0, 1}
});
```

A problem I had with `cpptoml` was that I was not able to dereference an `rvalue` table or array:

```cpp
// This will fail with cpptoml
for (auto& i : *config->get_qualified_array_of<array>("clients.data"))
{
    auto v = *(i->at(1)->get_array_of<int64_t>());
}
```

and now it is much easier to get the same vector:
```cpp
auto v = config["clients"][0]["data"][1].collect<int>();
```

tominal has extended support for dates and times beyond the TOML v0.4.0
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
116 passed, 12 failed // toml-test suite is only compliant with TOML v0.4.0
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
entire `toml::table` for serialization.

`build_toml.cpp` shows how to construct a TOML representation in-memory and
then serialize it to a stream.

[currver]: https://github.com/toml-lang/toml/blob/master/versions/en/toml-v1.0.0-rc.1.md
[cpptoml]: https://github.com/skystrife/cpptoml
[toml]: https://github.com/toml-lang/toml
[toml-test]: https://github.com/BurntSushi/toml-test
[toml-test-fork]: https://github.com/skystrife/toml-test
[toml11]: https://github.com/ToruNiina/toml11
[tinytoml]: https://github.com/mayah/tinytoml
[boost.toml]: https://github.com/ToruNiina/Boost.toml
[tomlplusplus]: https://github.com/marzer/tomlplusplus
