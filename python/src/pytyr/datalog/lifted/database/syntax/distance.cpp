#include "bindings.hpp"
#include "pytyr/module.hpp"

namespace tyr::database_python
{
void bind_query_distance(nb::module_& m, QueryRepositoryBinding& repository)
{
    using namespace nb::literals;
    urd::python::bind_index<urd::index<tyr::datalog::database::Query<tyr::datalog::database::QueryDistanceTag>>>(m, "QueryDistanceIndex");

    {
        using V = QueryData<tyr::datalog::database::QueryDistanceTag>;
        urd::python::bind_data<V>(m, "QueryDistanceData")
            .def(urd::python::init_from_views<QueryView, QueryView, QueryView, Column>(), "sources"_a, "edges"_a, "targets"_a, "distance_column"_a)
            .def_rw("sources", &V::sources)
            .def_rw("edges", &V::edges)
            .def_rw("targets", &V::targets)
            .def_rw("distance_column", &V::distance_column);
    }

    {
        using V = ConcreteQueryView<tyr::datalog::database::QueryDistanceTag>;
        auto cls = nb::class_<V>(m, "QueryDistance");
        cls.def("index", &V::index)
            .def("columns", &V::columns, nb::keep_alive<0, 1>())
            .def("get_sources", &V::get_sources)
            .def("get_edges", &V::get_edges)
            .def("get_targets", &V::get_targets)
            .def("get_distance_column", &V::get_distance_column);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    ygg::bind_insert<tyr::datalog::database::Query<tyr::datalog::database::QueryDistanceTag>>(repository);
}

}  // namespace tyr::database_python
