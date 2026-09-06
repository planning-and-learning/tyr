#include "tyr/formalism/datalog/conjunctive_condition_data.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_index.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include <concepts>

namespace lifted_tests
{

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

using Entity = fd::ConjunctiveCondition<::tyr::LiftedTag>;
using Index = ygg::Index<Entity>;
using Data = ygg::Data<Entity>;
using View = ygg::View<Index, fd::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, fd::ConjunctiveConditionView<::tyr::LiftedTag>>);
static_assert(std::constructible_from<Data,
                                      fd::VariableViewList,
                                      fd::LiteralViewList<::tyr::LiftedTag, f::StaticTag>,
                                      fd::LiteralViewList<::tyr::LiftedTag, f::FluentTag>,
                                      fd::BooleanOperatorViewList<::tyr::LiftedTag>>);
static_assert(requires(Data& data, const View& view) {
    data.index;
    data.variables;
    data.static_literals;
    data.fluent_literals;
    data.numeric_constraints;
    data.clear();
    data.template get_literals<f::StaticTag>();
    data.template get_literals<f::FluentTag>();
    view.get_index();
    view.get_variables();
    view.template get_literals<f::StaticTag>();
    view.template get_literals<f::FluentTag>();
    view.get_numeric_constraints();
    view.get_arity();
});

}

namespace ground_tests
{

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

using Entity = fd::ConjunctiveCondition<::tyr::GroundTag>;
using Index = ygg::Index<Entity>;
using Data = ygg::Data<Entity>;
using View = ygg::View<Index, fd::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, fd::ConjunctiveConditionView<::tyr::GroundTag>>);
static_assert(
    std::constructible_from<Data, fd::LiteralViewList<::tyr::GroundTag, f::StaticTag>, fd::LiteralViewList<::tyr::GroundTag, f::FluentTag>, fd::BooleanOperatorViewList<::tyr::GroundTag>>);
static_assert(requires(Data& data, const View& view) {
    data.index;
    data.static_literals;
    data.fluent_literals;
    data.numeric_constraints;
    data.clear();
    data.template get_literals<f::StaticTag>();
    data.template get_literals<f::FluentTag>();
    view.get_index();
    view.template get_literals<f::StaticTag>();
    view.template get_literals<f::FluentTag>();
    view.get_numeric_constraints();
});

}
