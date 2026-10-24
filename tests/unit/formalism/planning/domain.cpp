#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/domain_data.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

using DomainIndex = ygg::Index<tyr::formalism::planning::Domain>;
using DomainData = ygg::Data<tyr::formalism::planning::Domain>;
using DomainView = ygg::View<DomainIndex, tyr::formalism::planning::Repository>;

static_assert(std::constructible_from<DomainIndex, ygg::uint_t>);
static_assert(std::totally_ordered<DomainIndex>);
static_assert(std::totally_ordered<DomainData>);
static_assert(std::totally_ordered<DomainView>);
static_assert(std::same_as<DomainView, tyr::formalism::planning::DomainView>);
static_assert(requires(DomainData& data) {
    data.index;
    data.name;
    data.static_predicates;
    data.fluent_predicates;
    data.derived_predicates;
    data.static_functions;
    data.fluent_functions;
    data.auxiliary_function;
    data.constants;
    data.actions;
    data.axioms;
    data.clear();
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const DomainView& view) {
    view.get_index();
    view.get_name();
    view.template get_predicates<tyr::formalism::StaticTag>();
    view.template get_predicates<tyr::formalism::FluentTag>();
    view.template get_predicates<tyr::formalism::DerivedTag>();
    view.template get_functions<tyr::formalism::StaticTag>();
    view.template get_functions<tyr::formalism::FluentTag>();
    view.get_auxiliary_function();
    view.get_constants();
    view.get_actions();
    view.get_axioms();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});
