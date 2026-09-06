#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_NUMERIC_EFFECT_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_NUMERIC_EFFECT_VIEW_HPP_

#include "tyr/formalism/planning/numeric_effect_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/enums.hpp"
#include "tyr/serialization/formalism/planning/function_expression_view.hpp"
#include "tyr/serialization/formalism/planning/function_term_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<::tyr::TaskKind T, ::tyr::formalism::FactKind F>
struct TypeName<::tyr::formalism::planning::NumericEffectView<T, F>>
{
    static std::string get() { return std::string(F::name) + (std::same_as<T, ::tyr::GroundTag> ? T::name : "") + "NumericEffect"; }
};

template<::tyr::TaskKind T, ::tyr::formalism::FactKind F>
void tag_invoke(boost::json::value_from_tag,
                boost::json::value& result,
                const ::tyr::formalism::planning::NumericEffectView<T, F>& value,
                Dictionaries* dictionaries)
{
    dictionaries->object(result,
                         value,
                         [&](auto& ar)
                         {
                             ar.field("operator", value.get_operator());
                             ar.field("fterm", value.get_fterm());
                             ar.field("fexpr", value.get_fexpr());
                         });
}

}

#endif
