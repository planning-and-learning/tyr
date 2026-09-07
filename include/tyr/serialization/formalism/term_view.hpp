#ifndef TYR_SERIALIZATION_FORMALISM_TERM_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_TERM_VIEW_HPP_

#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/term_view.hpp"
#include "tyr/serialization/formalism/object_view.hpp"
#include "tyr/serialization/formalism/parameter_index.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<>
struct TypeName<::tyr::formalism::planning::TermView>
{
    static std::string get() { return "Term"; }
};

template<class Archive>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::TermView>)
{
    ar.variant([](const auto& value) -> decltype(auto) { return (value.get_variant()); });
}

}

#endif
