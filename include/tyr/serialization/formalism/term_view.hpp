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

inline void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const ::tyr::formalism::planning::TermView& value, Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar) { ar.variant(value.get_variant()); });
}

}

#endif
