#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_FUNCTION_TERM_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_FUNCTION_TERM_VIEW_HPP_

#include "tyr/formalism/planning/function_term_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/binding_view.hpp"
#include "tyr/serialization/formalism/function_view.hpp"
#include "tyr/serialization/formalism/term_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<::tyr::TaskKind T, ::tyr::formalism::FactKind F>
struct TypeName<::tyr::formalism::planning::FunctionTermView<T, F>>
{
    static std::string get() { return std::string(F::name) + (std::same_as<T, ::tyr::GroundTag> ? T::name : "") + "FunctionTerm"; }
};

template<class Archive, ::tyr::TaskKind T, ::tyr::formalism::FactKind F>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::FunctionTermView<T, F>>)
{
    if constexpr (std::same_as<T, ::tyr::LiftedTag>)
    {
        ar.field("function", [](const auto& value) -> decltype(auto) { return (value.get_function()); });
        ar.field("terms", [](const auto& value) -> decltype(auto) { return (value.get_terms()); });
    }
    else
    {
        ar.field("binding", [](const auto& value) -> decltype(auto) { return (value.get_row()); });
    }
}

}

#endif
