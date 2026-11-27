#ifndef PYTYR_FORMALISM_CONVERSIONS_HPP_
#define PYTYR_FORMALISM_CONVERSIONS_HPP_

// The conversions from Python of the formalism repository's builders (urd::python::detail::conversions): declared here,
// instantiated once in formalism/conversions.cpp, so that a source that binds a builder compiles its own kind's conversions only,
// not those of every kind its children reach (brokkr's template rule T5).

#include "pytyr/module.hpp"

#include <nanobind/nanobind.h>
#include <tyr/formalism/declarations.hpp>
#include <tyr/formalism/repository.hpp>
#include <urd/python/conversion_instantiations.hpp>
#include <yggdrasil/formalism/builder.hpp>

namespace tyr::python
{
using FormalismKinds = tyr::formalism::Repository::SymbolTypes;

/// Binds repository.insert(data) and repository.insert(builder) -> (view, created), and repository.view(key) -> the
/// view of the record of T under key. Data is canonicalized in place and interned (no copy); a builder is interned with
/// its children, built ones first, through scratch records of a builder storage of its own.
template<typename T>
void bind_insert(nanobind::class_<tyr::formalism::Repository>& repository)
{
    using tyr::formalism::Repository;
    repository.def("insert", [](Repository& self, urd::data<T>& data) { return self.insert(data); }, nanobind::arg("data"));
    repository.def(
        "insert",
        [](Repository& self, const urd::builder<T, Repository>& built)
        {
            auto storage = tyr::formalism::Builder {};
            return self.insert(built, storage);
        },
        nanobind::arg("builder"));
    repository.def("view", [](const Repository& self, urd::key_t<T> key) { return self.view(key); }, nanobind::arg("key"), nanobind::keep_alive<0, 1>());
}
}

URD_PYTHON_CONVERSIONS(extern, tyr::formalism::Repository, tyr::python::FormalismKinds, 70)

#endif
