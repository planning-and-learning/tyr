#include "tyr/formalism/datalog/ground_rule_data.hpp"
#include "tyr/formalism/datalog/ground_rule_index.hpp"
#include "tyr/formalism/datalog/ground_rule_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>

namespace fd = tyr::formalism::datalog;

using Entity = fd::GroundRule;
using Index = ygg::Index<Entity>;
using Data = ygg::Data<Entity>;
using View = ygg::View<Index, fd::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, fd::GroundRuleView>);
static_assert(requires(Data& data, const View& view) {
    data.index;
    data.binding;
    data.body;
    data.head;
    data.metric_effects;
    data.clear();
    view.get_index();
    view.get_rule();
    view.get_row();
    view.get_objects();
    view.get_key();
    view.get_body();
    view.get_head();
    view.get_metric_effects();
});
