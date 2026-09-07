#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_ARITHMETIC_OPERATOR_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_ARITHMETIC_OPERATOR_VIEW_HPP_

#include "tyr/formalism/planning/arithmetic_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/binary_operator_view.hpp"
#include "tyr/serialization/formalism/planning/multi_operator_view.hpp"
#include "tyr/serialization/formalism/planning/unary_operator_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::ArithmeticOperatorView<T>>)
{
    ar.variant([](const auto& value) -> decltype(auto) { return (value.get_variant()); });
}

}

#endif
