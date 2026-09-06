#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_LITERAL_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_LITERAL_VIEW_HPP_

#include "tyr/formalism/planning/literal_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/dictionaries.hpp"
#include "tyr/serialization/formalism/planning/atom_view.hpp"
#include "tyr/serialization/formalism/predicate_view.hpp"

namespace tyr::serialization
{

template<TaskKind T, formalism::FactKind F>
struct TypeName<formalism::planning::LiteralView<T, F>>
{
    static std::string get() { return std::string(F::name) + (std::same_as<T, GroundTag> ? T::name : "") + "Literal"; }
};

template<TaskKind T, formalism::FactKind F>
void tag_invoke(boost::json::value_from_tag,
                boost::json::value& result,
                const formalism::planning::LiteralView<T, F>& value,
                Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar)
    {
        ar.field("atom", value.get_atom());
        ar.field("polarity", value.get_polarity());
    });
}

}

#endif
