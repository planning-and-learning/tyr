#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_data.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"

#include <concepts>

namespace tyr::tests::datalog_rule
{

namespace lifted_tests
{

using Entity = tyr::formalism::datalog::Rule<::tyr::LiftedTag>;
using Index = urd::index<Entity>;
using Data = urd::data<Entity>;
using View = urd::index_view<Entity, tyr::formalism::datalog::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, tyr::formalism::datalog::RuleView<::tyr::LiftedTag>>);
static_assert(requires(Data& data, const View& view) {
    data.index;
    data.variables;
    data.body;
    data.heads;
    data.cost;
    view.index();
    view.get_arity();
    view.get_variables();
    view.get_body();
    view.get_heads();
    view.get_cost();
});

}

namespace ground_tests
{

using Entity = tyr::formalism::datalog::Rule<::tyr::GroundTag>;
using Index = urd::index<Entity>;
using Data = urd::data<Entity>;
using View = urd::index_view<Entity, tyr::formalism::datalog::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, tyr::formalism::datalog::RuleView<::tyr::GroundTag>>);
static_assert(requires(Data& data, const View& view) {
    data.index;
    data.row;
    data.body;
    data.heads;
    data.cost;
    view.index();
    view.get_rule();
    view.get_row();
    view.get_objects();
    view.get_key();
    view.get_body();
    view.get_heads();
    view.get_cost();
});

}

}  // namespace tyr::tests::datalog_rule
