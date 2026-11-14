#include "bindings.hpp"
#include "pytyr/module.hpp"

#include <urd/python/sequences/sequence.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::database_python
{
void bind_query_generic_join(nb::module_& m, QueryRepositoryBinding& repository)
{
    using namespace nb::literals;
    urd::python::bind_index<urd::index<tyr::datalog::database::Query<tyr::datalog::database::QueryGenericJoinTag>>>(m, "QueryGenericJoinIndex");

    {
        using V = QueryData<tyr::datalog::database::QueryGenericJoinTag>;
        urd::python::bind_data<V>(m, "QueryGenericJoinData")
            .def(urd::python::init_from_views<urd::Store<urd::sequence<QueryView>>, urd::Store<urd::sequence<Column>>, urd::Store<urd::sequence<Column>>>(),
                 "inputs"_a,
                 "variable_order"_a,
                 "output_order"_a = urd::Store<urd::sequence<Column>> {})
            .def_rw("inputs", &V::inputs)
            .def_rw("variable_order", &V::variable_order)
            .def_rw("output_order", &V::output_order);
    }

    {
        using V = ConcreteQueryView<tyr::datalog::database::QueryGenericJoinTag>;
        auto cls = nb::class_<V>(m, "QueryGenericJoin");
        cls.def("index", &V::index).def("columns", &V::columns, nb::keep_alive<0, 1>());
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    ygg::bind_insert<tyr::datalog::database::Query<tyr::datalog::database::QueryGenericJoinTag>>(repository);
}

}  // namespace tyr::database_python
