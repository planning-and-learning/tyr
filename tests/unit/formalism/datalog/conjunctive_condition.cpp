#include "tyr/formalism/datalog/conjunctive_condition_data.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>

namespace lifted_tests
{

using Entity = tyr::formalism::datalog::ConjunctiveCondition<::tyr::LiftedTag>;
using Index = urd::index<Entity>;
using Data = urd::data<Entity>;
using View = urd::view<Entity, tyr::formalism::datalog::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, tyr::formalism::datalog::ConjunctiveConditionView<::tyr::LiftedTag>>);
static_assert(requires(Data& data, const View& view) {
    data.index;
    data.variables;
    data.static_literals;
    data.fluent_literals;
    data.numeric_constraints;
    data.template get_literals<tyr::formalism::StaticTag>();
    data.template get_literals<tyr::formalism::FluentTag>();
    view.index();
    view.get_variables();
    view.template get_literals<tyr::formalism::StaticTag>();
    view.template get_literals<tyr::formalism::FluentTag>();
    view.get_numeric_constraints();
    view.get_arity();
});

}

namespace ground_tests
{

using Entity = tyr::formalism::datalog::ConjunctiveCondition<::tyr::GroundTag>;
using Index = urd::index<Entity>;
using Data = urd::data<Entity>;
using View = urd::view<Entity, tyr::formalism::datalog::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, tyr::formalism::datalog::ConjunctiveConditionView<::tyr::GroundTag>>);
static_assert(requires(Data& data, const View& view) {
    data.index;
    data.static_literals;
    data.fluent_literals;
    data.numeric_constraints;
    data.template get_literals<tyr::formalism::StaticTag>();
    data.template get_literals<tyr::formalism::FluentTag>();
    view.index();
    view.template get_literals<tyr::formalism::StaticTag>();
    view.template get_literals<tyr::formalism::FluentTag>();
    view.get_numeric_constraints();
});

}
