#include "bindings.hpp"
#include "pytyr/module.hpp"

namespace tyr::database_python
{
void bind_query_join(nb::module_& m, QueryRepositoryBinding& repository)
{
    using namespace nb::literals;
    urd::python::bind_index<urd::index<tyr::datalog::database::Query<tyr::datalog::database::QueryJoinTag>>>(m, "QueryJoinIndex");

    {
        using V = QueryData<tyr::datalog::database::QueryJoinTag>;
        urd::python::bind_data<V>(m, "QueryJoinData")
            .def(urd::python::init_from_views<QueryView, QueryView>(), "lhs"_a, "rhs"_a)
            .def_rw("lhs", &V::lhs)
            .def_rw("rhs", &V::rhs);
    }

    {
        using V = ConcreteQueryView<tyr::datalog::database::QueryJoinTag>;
        auto cls = nb::class_<V>(m, "QueryJoin");
        cls.def("index", &V::index).def("columns", &V::columns, nb::keep_alive<0, 1>()).def("get_lhs", &V::get_lhs).def("get_rhs", &V::get_rhs);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    ygg::bind_insert<tyr::datalog::database::Query<tyr::datalog::database::QueryJoinTag>>(repository);
}

}  // namespace tyr::database_python
