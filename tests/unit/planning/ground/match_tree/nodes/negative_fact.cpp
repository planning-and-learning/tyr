#include "tyr/planning/ground/match_tree/nodes/negative_fact_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/negative_fact_index.hpp"
#include "tyr/planning/ground/match_tree/nodes/negative_fact_view.hpp"
#include "tyr/planning/ground/match_tree/repository.hpp"

#include <concepts>

namespace fp = tyr::formalism::planning;
namespace mt = tyr::planning::match_tree;

template<typename Tag>
concept NegativeFactSelectorContract =
    std::constructible_from<ygg::Index<mt::NegativeFactSelectorNode<Tag>>, ygg::uint_t> && std::totally_ordered<ygg::Index<mt::NegativeFactSelectorNode<Tag>>>
    && std::totally_ordered<ygg::Data<mt::NegativeFactSelectorNode<Tag>>>
    && std::totally_ordered<ygg::View<ygg::Index<mt::NegativeFactSelectorNode<Tag>>, mt::Repository<Tag>>>
    && requires(ygg::Data<mt::NegativeFactSelectorNode<Tag>>& data, const ygg::View<ygg::Index<mt::NegativeFactSelectorNode<Tag>>, mt::Repository<Tag>>& view) {
           data.index;
           data.fact;
           data.true_child;
           data.dontcare_child;
           data.clear();
           view.get_fact();
           view.get_true_child();
           view.get_dontcare_child();
       };

using MatchTreeTags = ygg::TypeList<fp::GroundAction, fp::GroundAxiom>;
static_assert([]<typename... Tags>(ygg::TypeList<Tags...>) { return (NegativeFactSelectorContract<Tags> && ...); }(MatchTreeTags {}));
