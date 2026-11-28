#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/domain_data.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/repository.hpp"

#include <concepts>
#include <urd/elements/index.hpp>
#include <urd/elements/view.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::tests::planning_domain
{

using DomainIndex = urd::index<tyr::formalism::Domain>;
using DomainData = urd::data<tyr::formalism::Domain>;
using DomainView = urd::index_view<tyr::formalism::Domain, tyr::formalism::Repository>;

static_assert(std::constructible_from<DomainIndex, ygg::uint_t>);
static_assert(std::totally_ordered<DomainIndex>);
static_assert(std::totally_ordered<DomainData>);
static_assert(std::totally_ordered<DomainView>);
static_assert(std::same_as<DomainView, tyr::formalism::DomainView>);
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
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const DomainView& view) {
    view.index();
    view.name();
    view.template predicates<tyr::formalism::StaticTag>();
    view.template predicates<tyr::formalism::FluentTag>();
    view.template predicates<tyr::formalism::DerivedTag>();
    view.template functions<tyr::formalism::StaticTag>();
    view.template functions<tyr::formalism::FluentTag>();
    view.auxiliary_function();
    view.constants();
    view.actions();
    view.axioms();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});

}  // namespace tyr::tests::planning_domain
