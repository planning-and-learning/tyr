#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_FDR_FACT_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_FDR_FACT_VIEW_HPP_

#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/fdr_variable_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<::tyr::formalism::FactKind T>
struct TypeName<::tyr::formalism::planning::FDRFactView<T>>
{
    static std::string get() { return std::string(T::name) + "FDRFact"; }
};

template<::tyr::formalism::FactKind T>
void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const ::tyr::formalism::planning::FDRFactView<T>& value, Dictionaries* dictionaries)
{
    dictionaries->object(result,
                         value,
                         [&](auto& ar)
                         {
                             ar.field("variable", value.get_variable());
                             ar.field("value", ygg::uint_t(value.get_value()));
                         });
}

}

#endif
