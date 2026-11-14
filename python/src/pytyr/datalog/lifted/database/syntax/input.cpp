#include "bindings.hpp"
#include "pytyr/module.hpp"

namespace tyr::database_python
{
void bind_query_input(nb::module_& m, QueryRepositoryBinding& repository)
{
    using namespace nb::literals;
    urd::python::bind_index<urd::index<tyr::datalog::database::Query<tyr::datalog::database::QueryInputTag>>>(m, "QueryInputIndex");

    {
        using V = QueryData<tyr::datalog::database::QueryInputTag>;
        urd::python::bind_data<V>(m, "QueryInputData").def(nb::init<size_t, ColumnIndices>(), "slot"_a, "columns"_a).def_rw("input_slot", &V::input_slot);
    }

    {
        using V = ConcreteQueryView<tyr::datalog::database::QueryInputTag>;
        auto cls = nb::class_<V>(m, "QueryInput");
        cls.def("index", &V::index).def("columns", &V::columns, nb::keep_alive<0, 1>()).def("get_input_slot", &V::get_input_slot);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    ygg::bind_insert<tyr::datalog::database::Query<tyr::datalog::database::QueryInputTag>>(repository);
}

}  // namespace tyr::database_python
