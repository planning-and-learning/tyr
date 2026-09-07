#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/task_data.hpp"
#include "tyr/formalism/planning/task_index.hpp"
#include "tyr/formalism/planning/task_view.hpp"

#include <concepts>

namespace lifted_tests
{

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

using TaskIndex = ygg::Index<fp::Task<::tyr::LiftedTag>>;
using TaskData = ygg::Data<fp::Task<::tyr::LiftedTag>>;
using TaskView = ygg::View<TaskIndex, fp::Repository>;

static_assert(std::constructible_from<TaskIndex, ygg::uint_t>);
static_assert(std::totally_ordered<TaskIndex>);
static_assert(std::totally_ordered<TaskData>);
static_assert(std::totally_ordered<TaskView>);
static_assert(std::same_as<TaskView, fp::TaskView<::tyr::LiftedTag>>);
static_assert(std::same_as<fp::TaskListView<::tyr::LiftedTag>, ygg::View<ygg::IndexList<fp::Task<::tyr::LiftedTag>>, fp::Repository>>);
static_assert(requires(TaskData& data) {
    data.index;
    data.name;
    data.domain;
    data.derived_predicates;
    data.objects;
    data.static_atoms;
    data.fluent_atoms;
    data.static_fterm_values;
    data.fluent_fterm_values;
    data.auxiliary_fterm_value;
    data.goal;
    data.metric;
    data.axioms;
    data.clear();
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const TaskView& view) {
    view.get_index();
    view.get_name();
    view.get_domain();
    view.get_derived_predicates();
    view.get_objects();
    view.template get_atoms<f::StaticTag>();
    view.template get_atoms<f::FluentTag>();
    view.template get_fterm_values<f::StaticTag>();
    view.template get_fterm_values<f::FluentTag>();
    view.get_auxiliary_fterm_value();
    view.get_goal();
    view.get_metric();
    view.get_axioms();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});

}

namespace ground_tests
{

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

using TaskIndex = ygg::Index<fp::Task<::tyr::GroundTag>>;
using TaskData = ygg::Data<fp::Task<::tyr::GroundTag>>;
using TaskView = ygg::View<TaskIndex, fp::Repository>;

static_assert(std::constructible_from<TaskIndex, ygg::uint_t>);
static_assert(std::totally_ordered<TaskIndex>);
static_assert(std::totally_ordered<TaskData>);
static_assert(std::totally_ordered<TaskView>);
static_assert(std::same_as<TaskView, fp::TaskView<::tyr::GroundTag>>);
static_assert(std::same_as<fp::TaskListView<::tyr::GroundTag>, ygg::View<ygg::IndexList<fp::Task<::tyr::GroundTag>>, fp::Repository>>);
static_assert(requires(TaskData& data) {
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
static_assert(requires(const TaskView& view) {
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

}

static_assert(!std::same_as<lifted_tests::TaskIndex, ground_tests::TaskIndex>);
static_assert(!std::constructible_from<lifted_tests::TaskIndex, ground_tests::TaskIndex>);
static_assert(!std::constructible_from<ground_tests::TaskIndex, lifted_tests::TaskIndex>);
static_assert(!std::same_as<lifted_tests::TaskData, ground_tests::TaskData>);
static_assert(!std::same_as<lifted_tests::TaskView, ground_tests::TaskView>);
static_assert(!std::same_as<::tyr::formalism::planning::PlanningTask<::tyr::LiftedTag>, ::tyr::formalism::planning::PlanningTask<::tyr::GroundTag>>);
