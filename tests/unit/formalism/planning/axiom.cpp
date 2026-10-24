#include "tyr/formalism/planning/axiom_data.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace lifted_tests
{

using AxiomIndex = ygg::Index<tyr::formalism::planning::Axiom<::tyr::LiftedTag>>;
using AxiomData = ygg::Data<tyr::formalism::planning::Axiom<::tyr::LiftedTag>>;
using AxiomView = ygg::View<AxiomIndex, tyr::formalism::planning::Repository>;

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
    data.clear();
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const AxiomView& view) {
    view.get_index();
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

using GroundAxiomIndex = ygg::Index<tyr::formalism::planning::Axiom<::tyr::GroundTag>>;
using GroundAxiomData = ygg::Data<tyr::formalism::planning::Axiom<::tyr::GroundTag>>;
using GroundAxiomView = ygg::View<GroundAxiomIndex, tyr::formalism::planning::Repository>;

static_assert(std::constructible_from<GroundAxiomIndex, ygg::uint_t>);
static_assert(std::totally_ordered<GroundAxiomIndex>);
static_assert(std::totally_ordered<GroundAxiomData>);
static_assert(std::totally_ordered<GroundAxiomView>);
static_assert(std::same_as<GroundAxiomView, tyr::formalism::planning::AxiomView<::tyr::GroundTag>>);
static_assert(requires(GroundAxiomData& data) {
    data.index;
    data.binding;
    data.body;
    data.head;
    data.clear();
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const GroundAxiomView& view) {
    view.get_index();
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
