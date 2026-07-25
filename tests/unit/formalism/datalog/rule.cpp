#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_data.hpp"
#include "tyr/formalism/datalog/rule_index.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"

#include <concepts>

namespace fd = tyr::formalism::datalog;

using Entity = fd::Rule<tyr::formalism::PredicateTag>;
using Index = ygg::Index<Entity>;
using Data = ygg::Data<Entity>;
using View = ygg::View<Index, fd::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, fd::RuleView<tyr::formalism::PredicateTag>>);
static_assert(std::constructible_from<Data,
                                      fd::VariableViewList,
                                      fd::ConjunctiveConditionView,
                                      Data::HeadView<fd::Repository>,
                                      fd::NumericEffectOperatorViewList<tyr::formalism::FluentTag>>);
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
