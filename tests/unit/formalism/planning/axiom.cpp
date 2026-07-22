#include "tyr/formalism/planning/axiom_data.hpp"
#include "tyr/formalism/planning/axiom_index.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

using AxiomIndex = ygg::Index<fp::Axiom>;
using AxiomData = ygg::Data<fp::Axiom>;
using AxiomView = ygg::View<AxiomIndex, fp::Repository>;

static_assert(std::constructible_from<AxiomIndex, ygg::uint_t>);
static_assert(std::totally_ordered<AxiomIndex>);
static_assert(std::totally_ordered<AxiomData>);
static_assert(std::totally_ordered<AxiomView>);
static_assert(std::same_as<AxiomView, fp::AxiomView>);
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
