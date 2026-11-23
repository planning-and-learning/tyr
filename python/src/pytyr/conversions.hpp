#ifndef PYTYR_CONVERSIONS_HPP_
#define PYTYR_CONVERSIONS_HPP_

// The conversions from Python of the formalism repository's builders (urd::python::detail::conversions): declared here,
// instantiated once in conversions.cpp, so that a source that binds a builder compiles its own kind's conversions only,
// not those of every kind its children reach (brokkr's template rule T5).

#include "pytyr/module.hpp"

#include <tyr/formalism/repository.hpp>
#include <urd/python/conversion_instantiations.hpp>

namespace tyr::python
{
using FormalismKinds = tyr::formalism::Repository::SymbolTypes;
}

URD_PYTHON_CONVERSIONS(extern, tyr::formalism::Repository, tyr::python::FormalismKinds, 63)

#endif
