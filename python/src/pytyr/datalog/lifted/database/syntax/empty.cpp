#include "bindings.hpp"
#include "pytyr/module.hpp"

namespace tyr::database_python
{
void bind_query_empty(nb::module_& m, QueryRepositoryBinding& repository)
{
    using namespace nb::literals;
    urd::python::bind_index<urd::index<tyr::datalog::database::Query<tyr::datalog::database::QueryEmptyTag>>>(m, "QueryEmptyIndex");

    {
        using V = QueryData<tyr::datalog::database::QueryEmptyTag>;
        urd::python::bind_data<V>(m, "QueryEmptyData").def(nb::init<ColumnIndices>(), "columns"_a);
    }

    {
        using V = ConcreteQueryView<tyr::datalog::database::QueryEmptyTag>;
        auto cls = nb::class_<V>(m, "QueryEmpty");
        cls.def("index", &V::index).def("columns", &V::columns, nb::keep_alive<0, 1>());
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    ygg::bind_insert<tyr::datalog::database::Query<tyr::datalog::database::QueryEmptyTag>>(repository);
}

}  // namespace tyr::database_python
