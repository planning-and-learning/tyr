#include "tyr/formalism/planning/axiom_data.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace lifted_tests
{

using AxiomIndex = urd::index<tyr::formalism::planning::Axiom<::tyr::LiftedTag>>;
using AxiomData = urd::data<tyr::formalism::planning::Axiom<::tyr::LiftedTag>>;
using AxiomView = urd::index_view<tyr::formalism::planning::Axiom<::tyr::LiftedTag>, tyr::formalism::planning::Repository>;

static_assert(std::constructible_from<AxiomIndex, ygg::uint_t>);
static_assert(std::totally_ordered<AxiomIndex>);
static_assert(std::totally_ordered<AxiomData>);
static_assert(std::totally_ordered<AxiomView>);
static_assert(std::same_as<AxiomView, tyr::formalism::planning::AxiomView<::tyr::LiftedTag>>);
static_assert(requires(AxiomData& data) {
    data.index;
    data.variables;
    data.body;
    data.head;
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const AxiomView& view) {
    view.index();
    view.get_arity();
    view.get_body();
    view.get_variables();
    view.get_head();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});

}

namespace ground_tests
{

using GroundAxiomIndex = urd::index<tyr::formalism::planning::Axiom<::tyr::GroundTag>>;
using GroundAxiomData = urd::data<tyr::formalism::planning::Axiom<::tyr::GroundTag>>;
using GroundAxiomView = urd::index_view<tyr::formalism::planning::Axiom<::tyr::GroundTag>, tyr::formalism::planning::Repository>;

static_assert(std::constructible_from<GroundAxiomIndex, ygg::uint_t>);
static_assert(std::totally_ordered<GroundAxiomIndex>);
static_assert(std::totally_ordered<GroundAxiomData>);
static_assert(std::totally_ordered<GroundAxiomView>);
static_assert(std::same_as<GroundAxiomView, tyr::formalism::planning::AxiomView<::tyr::GroundTag>>);
static_assert(requires(GroundAxiomData& data) {
    data.index;
    data.row;
    data.body;
    data.head;
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const GroundAxiomView& view) {
    view.index();
    view.get_axiom();
    view.get_row();
    view.get_objects();
    view.get_key();
    view.get_body();
    view.get_head();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});

}
