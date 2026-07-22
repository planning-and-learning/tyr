#include "tyr/formalism/parameter_index.hpp"

#include <concepts>

static_assert(std::constructible_from<tyr::formalism::ParameterIndex, ygg::uint_t>);
static_assert(std::totally_ordered<tyr::formalism::ParameterIndex>);
static_assert(std::same_as<tyr::formalism::ParameterList, std::vector<tyr::formalism::ParameterIndex>>);
