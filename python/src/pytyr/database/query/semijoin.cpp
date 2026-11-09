#include "bindings.hpp"
#include "pytyr/module.hpp"

namespace tyr::database_python
{
void bind_query_semijoin(nb::module_& m, QueryRepositoryBinding& repository)
{
    using namespace nb::literals;
    urd::python::bind_index<urd::index<tyr::datalog::database::Query<tyr::datalog::database::QuerySemijoinTag>>>(m, "QuerySemijoinIndex");

    {
        using V = QueryData<tyr::datalog::database::QuerySemijoinTag>;
        urd::python::bind_data<V>(m, "QuerySemijoinData").def(urd::python::init_from_views<QueryView, QueryView>(), "lhs"_a, "rhs"_a).def_rw("lhs", &V::lhs).def_rw("rhs", &V::rhs);
    }

    {
        using V = ConcreteQueryView<tyr::datalog::database::QuerySemijoinTag>;
        auto cls = nb::class_<V>(m, "QuerySemijoin");
        cls.def("index", &V::index)
            .def("columns", &V::columns, nb::keep_alive<0, 1>())
            .def("get_lhs", &V::get_lhs, nb::keep_alive<0, 1>())
            .def("get_rhs", &V::get_rhs, nb::keep_alive<0, 1>());
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    ygg::bind_insert<tyr::datalog::database::Query<tyr::datalog::database::QuerySemijoinTag>>(repository);
}

}  // namespace tyr::database_python
