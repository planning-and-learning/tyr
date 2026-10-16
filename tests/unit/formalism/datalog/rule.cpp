#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_data.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include <concepts>

namespace lifted_tests
{

using Entity = tyr::formalism::datalog::Rule<::tyr::LiftedTag, tyr::formalism::PredicateTag>;
using Index = ygg::Index<Entity>;
using Data = ygg::Data<Entity>;
using View = ygg::View<Index, tyr::formalism::datalog::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, tyr::formalism::datalog::RuleView<::tyr::LiftedTag, tyr::formalism::PredicateTag>>);
static_assert(std::constructible_from<Data,
                                      tyr::formalism::datalog::VariableViewList,
                                      tyr::formalism::datalog::ConjunctiveConditionView<::tyr::LiftedTag>,
                                      Data::HeadView<tyr::formalism::datalog::Repository>,
                                      ygg::float_t>);
static_assert(requires(Data& data, const View& view) {
    data.index;
    data.variables;
    data.body;
    data.heads;
    data.cost;
    data.clear();
    view.get_index();
    view.get_arity();
    view.get_variables();
    view.get_body();
    view.get_heads();
    view.get_cost();
});

}

namespace ground_tests
{

using Entity = tyr::formalism::datalog::Rule<::tyr::GroundTag, tyr::formalism::PredicateTag>;
using Index = ygg::Index<Entity>;
using Data = ygg::Data<Entity>;
using View = ygg::View<Index, tyr::formalism::datalog::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, tyr::formalism::datalog::RuleView<::tyr::GroundTag, tyr::formalism::PredicateTag>>);
static_assert(std::constructible_from<Data,
                                      tyr::formalism::datalog::RuleBindingView<tyr::formalism::PredicateTag>,
                                      tyr::formalism::datalog::ConjunctiveConditionView<::tyr::GroundTag>,
                                      Data::HeadView<tyr::formalism::datalog::Repository>,
                                      ygg::float_t>);
static_assert(requires(Data& data, const View& view) {
    data.index;
    data.binding;
    data.body;
    data.heads;
    data.cost;
    data.clear();
    view.get_index();
    view.get_rule();
    view.get_row();
    view.get_objects();
    view.get_key();
    view.get_body();
    view.get_heads();
    view.get_cost();
});

}
