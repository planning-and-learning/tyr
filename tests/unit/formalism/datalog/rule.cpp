#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_data.hpp"
#include "tyr/formalism/datalog/rule_index.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include <concepts>

namespace lifted_tests
{

namespace fd = tyr::formalism::datalog;

using Entity = fd::Rule<::tyr::LiftedTag, tyr::formalism::PredicateTag>;
using Index = ygg::Index<Entity>;
using Data = ygg::Data<Entity>;
using View = ygg::View<Index, fd::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, fd::RuleView<::tyr::LiftedTag, tyr::formalism::PredicateTag>>);
static_assert(std::constructible_from<Data,
                                      fd::VariableViewList,
                                      fd::ConjunctiveConditionView<::tyr::LiftedTag>,
                                      Data::HeadView<fd::Repository>,
                                      fd::NumericEffectOperatorViewList<::tyr::LiftedTag, tyr::formalism::FluentTag>>);
static_assert(requires(Data& data, const View& view) {
    data.index;
    data.variables;
    data.body;
    data.head;
    data.metric_effects;
    data.clear();
    view.get_index();
    view.get_arity();
    view.get_variables();
    view.get_body();
    view.get_head();
    view.get_metric_effects();
});

}

namespace ground_tests
{

namespace fd = tyr::formalism::datalog;

using Entity = fd::Rule<::tyr::GroundTag, tyr::formalism::PredicateTag>;
using Index = ygg::Index<Entity>;
using Data = ygg::Data<Entity>;
using View = ygg::View<Index, fd::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, fd::RuleView<::tyr::GroundTag, tyr::formalism::PredicateTag>>);
static_assert(std::constructible_from<Data,
                                      fd::RuleBindingView<tyr::formalism::PredicateTag>,
                                      fd::ConjunctiveConditionView<::tyr::GroundTag>,
                                      Data::HeadView<fd::Repository>,
                                      fd::NumericEffectOperatorViewList<::tyr::GroundTag, tyr::formalism::FluentTag>>);
static_assert(requires(Data& data, const View& view) {
    data.index;
    data.binding;
    data.body;
    data.head;
    data.metric_effects;
    data.clear();
    view.get_index();
    view.get_rule();
    view.get_row();
    view.get_objects();
    view.get_key();
    view.get_body();
    view.get_head();
    view.get_metric_effects();
});

}
