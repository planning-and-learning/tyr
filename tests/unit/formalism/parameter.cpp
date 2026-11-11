#include "tyr/formalism/parameter_index.hpp"

#include <concepts>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <yggdrasil/core/config.hpp>

static_assert(std::constructible_from<tyr::formalism::ParameterIndex, ygg::uint_t>);
static_assert(std::totally_ordered<tyr::formalism::ParameterIndex>);
static_assert(std::same_as<tyr::formalism::ParameterList, urd::Store<urd::sequence<tyr::formalism::ParameterIndex>>>);
