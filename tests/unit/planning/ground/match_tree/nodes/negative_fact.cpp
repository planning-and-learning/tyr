#include "tyr/planning/ground/match_tree/declarations.hpp"
#include "tyr/planning/ground/match_tree/nodes/negative_fact_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/node_view.hpp"
#include "tyr/planning/ground/match_tree/repository.hpp"

#include <concepts>

template<typename Tag>
concept NegativeFactSelectorContract =
    std::constructible_from<urd::index<tyr::planning::match_tree::NegativeFactSelectorNode<Tag>>, ygg::uint_t>
    && std::totally_ordered<urd::index<tyr::planning::match_tree::NegativeFactSelectorNode<Tag>>>
    && std::totally_ordered<urd::data<tyr::planning::match_tree::NegativeFactSelectorNode<Tag>>>
    && std::totally_ordered<urd::index_view<tyr::planning::match_tree::NegativeFactSelectorNode<Tag>, tyr::planning::match_tree::Repository<Tag>>>
    && requires(urd::data<tyr::planning::match_tree::NegativeFactSelectorNode<Tag>>& data,
                const urd::index_view<tyr::planning::match_tree::NegativeFactSelectorNode<Tag>, tyr::planning::match_tree::Repository<Tag>>& view) {
           data.index;
           data.fact;
           data.true_child;
           data.dontcare_child;
           view.get_fact();
           view.get_true_child();
           view.get_dontcare_child();
       };

using MatchTreeTags = boost::mp11::mp_list<tyr::formalism::planning::Action<::tyr::GroundTag>, tyr::formalism::planning::Axiom<::tyr::GroundTag>>;
static_assert([]<typename... Tags>(boost::mp11::mp_list<Tags...>) { return (NegativeFactSelectorContract<Tags> && ...); }(MatchTreeTags {}));
