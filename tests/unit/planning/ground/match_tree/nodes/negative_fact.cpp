#include "tyr/planning/ground/match_tree/nodes/negative_fact_data.hpp"
#include "tyr/planning/ground/match_tree/declarations.hpp"
#include "tyr/planning/ground/match_tree/nodes/negative_fact_view.hpp"
#include "tyr/planning/ground/match_tree/repository.hpp"

#include <concepts>

template<typename Tag>
concept NegativeFactSelectorContract =
    std::constructible_from<ygg::Index<tyr::planning::match_tree::NegativeFactSelectorNode<Tag>>, ygg::uint_t>
    && std::totally_ordered<ygg::Index<tyr::planning::match_tree::NegativeFactSelectorNode<Tag>>>
    && std::totally_ordered<ygg::Data<tyr::planning::match_tree::NegativeFactSelectorNode<Tag>>>
    && std::totally_ordered<ygg::View<ygg::Index<tyr::planning::match_tree::NegativeFactSelectorNode<Tag>>, tyr::planning::match_tree::Repository<Tag>>>
    && requires(ygg::Data<tyr::planning::match_tree::NegativeFactSelectorNode<Tag>>& data,
                const ygg::View<ygg::Index<tyr::planning::match_tree::NegativeFactSelectorNode<Tag>>, tyr::planning::match_tree::Repository<Tag>>& view) {
           data.index;
           data.fact;
           data.true_child;
           data.dontcare_child;
           data.clear();
           view.get_fact();
           view.get_true_child();
           view.get_dontcare_child();
       };

using MatchTreeTags = ygg::TypeList<tyr::formalism::planning::Action<::tyr::GroundTag>, tyr::formalism::planning::Axiom<::tyr::GroundTag>>;
static_assert([]<typename... Tags>(ygg::TypeList<Tags...>) { return (NegativeFactSelectorContract<Tags> && ...); }(MatchTreeTags {}));
