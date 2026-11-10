#include "bindings.hpp"
#include "pytyr/module.hpp"

namespace tyr::database_python
{
void bind_query_select_value(nb::module_& m, QueryRepositoryBinding& repository)
{
    using namespace nb::literals;
    urd::python::bind_index<urd::index<tyr::datalog::database::Query<tyr::datalog::database::QuerySelectValueTag>>>(m, "QuerySelectValueIndex");

    {
        using V = QueryData<tyr::datalog::database::QuerySelectValueTag>;
        // Python values are cast to the given column type; the record encodes them.
        urd::python::bind_data<V>(m, "QuerySelectValueData")
            .def(
                "__init__",
                [](V* self, QueryView arg, Column column, ygg::TypeId type, nb::handle value)
                {
                    std::visit([&](auto converted) { new (self) V(arg.index(), column, converted); }, ygg::cast_value(value, type));
                },
                "arg"_a,
                "column"_a,
                "type"_a,
                "value"_a)
            .def_rw("arg", &V::arg)
            .def_rw("column", &V::column);
    }

    {
        using V = ConcreteQueryView<tyr::datalog::database::QuerySelectValueTag>;
        auto cls = nb::class_<V>(m, "QuerySelectValue");
        cls.def("index", &V::index).def("columns", &V::columns, nb::keep_alive<0, 1>()).def("get_arg", &V::get_arg).def("get_column", &V::get_column);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    ygg::bind_insert<tyr::datalog::database::Query<tyr::datalog::database::QuerySelectValueTag>>(repository);
}

}  // namespace tyr::database_python
