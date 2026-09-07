#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_ATOM_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_ATOM_VIEW_HPP_

#include "tyr/formalism/planning/atom_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/binding_view.hpp"
#include "tyr/serialization/formalism/predicate_view.hpp"
#include "tyr/serialization/formalism/term_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<::tyr::TaskKind T, ::tyr::formalism::FactKind F>
struct TypeName<::tyr::formalism::planning::AtomView<T, F>>
{
    static std::string get() { return std::string(F::name) + (std::same_as<T, ::tyr::GroundTag> ? T::name : "") + "Atom"; }
};

template<class Archive, ::tyr::TaskKind T, ::tyr::formalism::FactKind F>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::AtomView<T, F>>)
{
    if constexpr (std::same_as<T, ::tyr::LiftedTag>)
    {
        ar.field("predicate", [](const auto& value) -> decltype(auto) { return (value.get_predicate()); });
        ar.field("terms", [](const auto& value) -> decltype(auto) { return (value.get_terms()); });
    }
    else
    {
        ar.field("binding", [](const auto& value) -> decltype(auto) { return (value.get_row()); });
    }
}

}

#endif
