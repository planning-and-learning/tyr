#include "tyr/planning/ground/match_tree/declarations.hpp"
#include "tyr/planning/ground/match_tree/nodes/constraint_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/node_view.hpp"
#include "tyr/planning/ground/match_tree/repository.hpp"

#include <concepts>

template<typename Tag>
concept NumericConstraintSelectorContract =
    std::constructible_from<ygg::Index<tyr::planning::match_tree::NumericConstraintSelectorNode<Tag>>, ygg::uint_t>
    && std::totally_ordered<ygg::Index<tyr::planning::match_tree::NumericConstraintSelectorNode<Tag>>>
    && std::totally_ordered<ygg::Data<tyr::planning::match_tree::NumericConstraintSelectorNode<Tag>>>
    && std::totally_ordered<ygg::View<ygg::Index<tyr::planning::match_tree::NumericConstraintSelectorNode<Tag>>, tyr::planning::match_tree::Repository<Tag>>>
    && requires(ygg::Data<tyr::planning::match_tree::NumericConstraintSelectorNode<Tag>>& data,
                const ygg::View<ygg::Index<tyr::planning::match_tree::NumericConstraintSelectorNode<Tag>>, tyr::planning::match_tree::Repository<Tag>>& view) {
           data.index;
           data.constraint;
           data.true_child;
           data.dontcare_child;
           data.clear();
           view.get_constraint();
           view.get_true_child();
           view.get_dontcare_child();
       };

using MatchTreeTags = ygg::TypeList<tyr::formalism::planning::Action<::tyr::GroundTag>, tyr::formalism::planning::Axiom<::tyr::GroundTag>>;
static_assert([]<typename... Tags>(ygg::TypeList<Tags...>) { return (NumericConstraintSelectorContract<Tags> && ...); }(MatchTreeTags {}));
