#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_LITERAL_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_LITERAL_VIEW_HPP_

#include "tyr/formalism/planning/literal_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/atom_view.hpp"
#include "tyr/serialization/formalism/predicate_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<::tyr::TaskKind T, ::tyr::formalism::FactKind F>
struct TypeName<::tyr::formalism::planning::LiteralView<T, F>>
{
    static std::string get() { return std::string(F::name) + (std::same_as<T, ::tyr::GroundTag> ? T::name : "") + "Literal"; }
};

template<class Archive, ::tyr::TaskKind T, ::tyr::formalism::FactKind F>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::LiteralView<T, F>>)
{
    ar.field("atom", [](const auto& value) -> decltype(auto) { return (value.get_atom()); });
    ar.field("polarity", [](const auto& value) -> decltype(auto) { return (value.get_polarity()); });
}

}

#endif
