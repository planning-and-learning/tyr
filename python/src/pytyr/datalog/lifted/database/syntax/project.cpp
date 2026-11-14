#include "bindings.hpp"
#include "pytyr/module.hpp"

#include <urd/python/sequences/sequence.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::database_python
{
void bind_query_project(nb::module_& m, QueryRepositoryBinding& repository)
{
    using namespace nb::literals;
    urd::python::bind_index<urd::index<tyr::datalog::database::Query<tyr::datalog::database::QueryProjectTag>>>(m, "QueryProjectIndex");

    {
        using V = QueryData<tyr::datalog::database::QueryProjectTag>;
        urd::python::bind_data<V>(m, "QueryProjectData")
            .def(urd::python::init_from_views<QueryView, urd::Store<urd::sequence<Column>>>(), "arg"_a, "labels"_a)
            .def_rw("arg", &V::arg)
            .def_rw("labels", &V::labels);
    }

    {
        using V = ConcreteQueryView<tyr::datalog::database::QueryProjectTag>;
        auto cls = nb::class_<V>(m, "QueryProject");
        cls.def("index", &V::index).def("columns", &V::columns, nb::keep_alive<0, 1>()).def("get_arg", &V::get_arg);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    ygg::bind_insert<tyr::datalog::database::Query<tyr::datalog::database::QueryProjectTag>>(repository);
}

}  // namespace tyr::database_python
