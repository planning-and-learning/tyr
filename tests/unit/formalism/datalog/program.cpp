#include "tyr/formalism/datalog/program_data.hpp"
#include "tyr/formalism/datalog/program_index.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

using Entity = fd::Program;
using Index = ygg::Index<Entity>;
using Data = ygg::Data<Entity>;
using View = ygg::View<Index, fd::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, fd::ProgramView<tyr::LiftedTag>>);
static_assert(std::constructible_from<Data,
                                      fd::PredicateViewList<f::StaticTag>,
                                      fd::PredicateViewList<f::FluentTag>,
                                      fd::FunctionViewList<f::StaticTag>,
                                      fd::FunctionViewList<f::FluentTag>,
                                      fd::ObjectViewList,
                                      fd::GroundAtomViewList<f::StaticTag>,
                                      fd::GroundAtomViewList<f::FluentTag>,
                                      fd::GroundFunctionTermValueViewList<f::StaticTag>,
                                      fd::GroundFunctionTermValueViewList<f::FluentTag>,
                                      std::optional<fd::GroundConjunctiveConditionView>,
                                      std::optional<fd::MetricView>,
                                      fd::RuleViewList<f::PredicateTag>,
                                      fd::RuleViewList<f::FunctionTag>>);
static_assert(requires(Data& data, const View& view) {
    data.index;
    data.static_predicates;
    data.fluent_predicates;
    data.static_functions;
    data.fluent_functions;
    data.objects;
    data.static_atoms;
    data.fluent_atoms;
    data.static_fterm_values;
    data.fluent_fterm_values;
    data.goal;
    data.metric;
    data.predicate_rules;
    data.function_rules;
    data.clear();
    data.template get_predicates<f::StaticTag>();
    data.template get_predicates<f::FluentTag>();
    data.template get_functions<f::StaticTag>();
    data.template get_functions<f::FluentTag>();
    data.template get_atoms<f::StaticTag>();
    data.template get_atoms<f::FluentTag>();
    data.template get_fterm_values<f::StaticTag>();
    data.template get_fterm_values<f::FluentTag>();
    view.get_index();
    view.template get_predicates<f::StaticTag>();
    view.template get_predicates<f::FluentTag>();
    view.template get_functions<f::StaticTag>();
    view.template get_functions<f::FluentTag>();
    view.get_objects();
    view.template get_atoms<f::StaticTag>();
    view.template get_atoms<f::FluentTag>();
    view.template get_fterm_values<f::StaticTag>();
    view.template get_fterm_values<f::FluentTag>();
    view.get_goal();
    view.get_metric();
    view.template get_rules<f::PredicateTag>();
    view.template get_rules<f::FunctionTag>();
});
