#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_FDR_FACT_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_FDR_FACT_VIEW_HPP_

#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/fdr_variable_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<::tyr::formalism::FactKind T>
struct Serializer<::tyr::formalism::planning::FDRFactView<T>>
{
    static std::string name() { return std::string(T::name) + "FDRFact"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::FDRFactView<T>& value)
    {
        ar.field("variable", value.get_variable());
        ar.field("value", ygg::uint_t(value.get_value()));
    }
};

}

#endif
