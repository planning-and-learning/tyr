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

template<::tyr::TaskKind T, ::tyr::formalism::FactKind F>
void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const ::tyr::formalism::planning::AtomView<T, F>& value, Dictionaries* dictionaries)
{
    dictionaries->object(result,
                         value,
                         [&](auto& ar)
                         {
                             if constexpr (std::same_as<T, ::tyr::LiftedTag>)
                             {
                                 ar.field("predicate", value.get_predicate());
                                 ar.field("terms", value.get_terms());
                             }
                             else
                             {
                                 ar.field("binding", value.get_row());
                             }
                         });
}

}

#endif
