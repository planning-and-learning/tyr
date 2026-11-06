#include "tyr/planning/ground/match_tree/declarations.hpp"
#include "tyr/planning/ground/match_tree/nodes/constraint_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/node_view.hpp"
#include "tyr/planning/ground/match_tree/repository.hpp"

#include <concepts>

template<typename Tag>
concept NumericConstraintSelectorContract =
    std::constructible_from<urd::index<tyr::planning::match_tree::NumericConstraintSelectorNode<Tag>>, ygg::uint_t>
    && std::totally_ordered<urd::index<tyr::planning::match_tree::NumericConstraintSelectorNode<Tag>>>
    && std::totally_ordered<urd::data<tyr::planning::match_tree::NumericConstraintSelectorNode<Tag>>>
    && std::totally_ordered<urd::view<tyr::planning::match_tree::NumericConstraintSelectorNode<Tag>, tyr::planning::match_tree::Repository<Tag>>>
    && requires(urd::data<tyr::planning::match_tree::NumericConstraintSelectorNode<Tag>>& data,
                const urd::view<tyr::planning::match_tree::NumericConstraintSelectorNode<Tag>, tyr::planning::match_tree::Repository<Tag>>& view) {
           data.index;
           data.constraint;
           data.true_child;
           data.dontcare_child;
           view.get_constraint();
           view.get_true_child();
           view.get_dontcare_child();
       };

using MatchTreeTags = boost::mp11::mp_list<tyr::formalism::planning::Action<::tyr::GroundTag>, tyr::formalism::planning::Axiom<::tyr::GroundTag>>;
static_assert([]<typename... Tags>(boost::mp11::mp_list<Tags...>) { return (NumericConstraintSelectorContract<Tags> && ...); }(MatchTreeTags {}));
