#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/task_data.hpp"
#include "tyr/formalism/planning/task_index.hpp"
#include "tyr/formalism/planning/task_view.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

using TaskIndex = ygg::Index<fp::Task>;
using TaskData = ygg::Data<fp::Task>;
using TaskView = ygg::View<TaskIndex, fp::Repository>;

static_assert(std::constructible_from<TaskIndex, ygg::uint_t>);
static_assert(std::totally_ordered<TaskIndex>);
static_assert(std::totally_ordered<TaskData>);
static_assert(std::totally_ordered<TaskView>);
static_assert(std::same_as<TaskView, fp::TaskView>);
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
