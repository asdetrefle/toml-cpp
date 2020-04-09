#pragma once

#include <vector>

#include "toml_base.h"
#include "toml_node.h"

namespace toml
{
TOML_NAMESPACE_BEGIN

template <class T>
struct array_of_trait
{
    using return_type = std::optional<std::vector<T>>;
};

template <>
struct array_of_trait<array>
{
    using return_type = std::optional<std::vector<std::shared_ptr<array>>>;
};
TOML_NAMESPACE_END
}