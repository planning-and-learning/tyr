#include "tyr/formalism/planning/fdr_task_data.hpp"
#include "tyr/formalism/planning/fdr_task_index.hpp"
#include "tyr/formalism/planning/fdr_task_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

using FdrTaskIndex = ygg::Index<fp::FDRTask>;
using FdrTaskData = ygg::Data<fp::FDRTask>;
using FdrTaskView = ygg::View<FdrTaskIndex, fp::Repository>;

static_assert(std::constructible_from<FdrTaskIndex, ygg::uint_t>);
static_assert(std::totally_ordered<FdrTaskIndex>);
static_assert(std::totally_ordered<FdrTaskData>);
static_assert(std::totally_ordered<FdrTaskView>);
static_assert(std::same_as<FdrTaskView, fp::FDRTaskView>);
static_assert(requires(FdrTaskData& data) {
    data.index;
    data.name;
    data.domain;
    data.derived_predicates;
    data.objects;
    data.static_atoms;
    data.fluent_atoms;
    data.derived_atoms;
    data.static_fterm_values;
    data.fluent_fterm_values;
    data.auxiliary_fterm_value;
    data.metric;
    data.axioms;
    data.fluent_variables;
    data.fluent_facts;
    data.goal;
    data.ground_actions;
    data.ground_axioms;
    data.clear();
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const FdrTaskView& view) {
    view.get_index();
    view.get_name();
    view.get_domain();
    view.get_derived_predicates();
    view.get_objects();
    view.template get_atoms<f::StaticTag>();
    view.template get_atoms<f::FluentTag>();
    view.template get_atoms<f::DerivedTag>();
    view.template get_fterm_values<f::StaticTag>();
    view.template get_fterm_values<f::FluentTag>();
    view.get_auxiliary_fterm_value();
    view.get_goal();
    view.get_metric();
    view.get_axioms();
    view.get_fluent_variables();
    view.get_fluent_facts();
    view.get_ground_actions();
    view.get_ground_axioms();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});
