#ifndef TYR_SERIALIZATION_FORMALISM_PARAMETER_INDEX_HPP_
#define TYR_SERIALIZATION_FORMALISM_PARAMETER_INDEX_HPP_

#include "tyr/formalism/parameter_index.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<>
struct PrimitiveSerializer<formalism::ParameterIndex>
{
    static std::string name() { return "ParameterIndex"; }
    static ygg::uint_t value(formalism::ParameterIndex index) { return ygg::uint_t(index); }
};

}

#endif
