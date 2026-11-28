#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/planning/task_data.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/repository.hpp"

#include <concepts>
#include <cstddef>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <urd/elements/view.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::tests::planning_task
{

namespace lifted_tests
{

using TaskIndex = urd::index<tyr::formalism::Task<::tyr::LiftedTag>>;
using TaskData = urd::data<tyr::formalism::Task<::tyr::LiftedTag>>;
using TaskView = urd::index_view<tyr::formalism::Task<::tyr::LiftedTag>, tyr::formalism::Repository>;

static_assert(std::constructible_from<TaskIndex, ygg::uint_t>);
static_assert(std::totally_ordered<TaskIndex>);
static_assert(std::totally_ordered<TaskData>);
static_assert(std::totally_ordered<TaskView>);
static_assert(std::same_as<TaskView, tyr::formalism::TaskView<::tyr::LiftedTag>>);
static_assert(std::same_as<tyr::formalism::TaskListView<::tyr::LiftedTag>,
                           urd::resolved_range<urd::flat_vector<urd::index<tyr::formalism::Task<::tyr::LiftedTag>>>, tyr::formalism::Repository>>);
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
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const TaskView& view) {
    view.index();
    view.name();
    view.domain();
    view.derived_predicates();
    view.objects();
    { view.num_objects() } -> std::same_as<size_t>;
    view.template atoms<tyr::formalism::StaticTag>();
    view.template atoms<tyr::formalism::FluentTag>();
    view.template fterm_values<tyr::formalism::StaticTag>();
    view.template fterm_values<tyr::formalism::FluentTag>();
    view.auxiliary_fterm_value();
    view.goal();
    view.metric();
    view.axioms();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});

}

namespace ground_tests
{

using TaskIndex = urd::index<tyr::formalism::Task<::tyr::GroundTag>>;
using TaskData = urd::data<tyr::formalism::Task<::tyr::GroundTag>>;
using TaskView = urd::index_view<tyr::formalism::Task<::tyr::GroundTag>, tyr::formalism::Repository>;

static_assert(std::constructible_from<TaskIndex, ygg::uint_t>);
static_assert(std::totally_ordered<TaskIndex>);
static_assert(std::totally_ordered<TaskData>);
static_assert(std::totally_ordered<TaskView>);
static_assert(std::same_as<TaskView, tyr::formalism::TaskView<::tyr::GroundTag>>);
static_assert(std::same_as<tyr::formalism::TaskListView<::tyr::GroundTag>,
                           urd::resolved_range<urd::flat_vector<urd::index<tyr::formalism::Task<::tyr::GroundTag>>>, tyr::formalism::Repository>>);
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
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const TaskView& view) {
    view.index();
    view.name();
    view.domain();
    view.derived_predicates();
    view.objects();
    { view.num_objects() } -> std::same_as<size_t>;
    view.template atoms<tyr::formalism::StaticTag>();
    view.template atoms<tyr::formalism::FluentTag>();
    view.template atoms<tyr::formalism::DerivedTag>();
    view.template fterm_values<tyr::formalism::StaticTag>();
    view.template fterm_values<tyr::formalism::FluentTag>();
    view.auxiliary_fterm_value();
    view.goal();
    view.metric();
    view.axioms();
    view.fluent_variables();
    view.fluent_facts();
    view.ground_actions();
    view.ground_axioms();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});

}

static_assert(!std::same_as<lifted_tests::TaskIndex, ground_tests::TaskIndex>);
static_assert(!std::constructible_from<lifted_tests::TaskIndex, ground_tests::TaskIndex>);
static_assert(!std::constructible_from<ground_tests::TaskIndex, lifted_tests::TaskIndex>);
static_assert(!std::same_as<lifted_tests::TaskData, ground_tests::TaskData>);
static_assert(!std::same_as<lifted_tests::TaskView, ground_tests::TaskView>);
static_assert(!std::same_as<::tyr::formalism::PlanningTask<::tyr::LiftedTag>, ::tyr::formalism::PlanningTask<::tyr::GroundTag>>);

}  // namespace tyr::tests::planning_task
