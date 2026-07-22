#include "tyr/formalism/planning/ground_axiom_data.hpp"
#include "tyr/formalism/planning/ground_axiom_index.hpp"
#include "tyr/formalism/planning/ground_axiom_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

using GroundAxiomIndex = ygg::Index<fp::GroundAxiom>;
using GroundAxiomData = ygg::Data<fp::GroundAxiom>;
using GroundAxiomView = ygg::View<GroundAxiomIndex, fp::Repository>;

static_assert(std::constructible_from<GroundAxiomIndex, ygg::uint_t>);
static_assert(std::totally_ordered<GroundAxiomIndex>);
static_assert(std::totally_ordered<GroundAxiomData>);
static_assert(std::totally_ordered<GroundAxiomView>);
static_assert(std::same_as<GroundAxiomView, fp::GroundAxiomView>);
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
