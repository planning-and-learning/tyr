#include "tyr/planning/successors/ground/match_tree/declarations.hpp"
#include "tyr/planning/successors/ground/match_tree/nodes/atom_data.hpp"
#include "tyr/planning/successors/ground/match_tree/nodes/node_view.hpp"
#include "tyr/planning/successors/ground/match_tree/repository.hpp"

#include <concepts>

template<typename Tag>
concept AtomSelectorContract =
    std::constructible_from<urd::index<tyr::planning::match_tree::AtomSelectorNode<Tag>>, ygg::uint_t>
    && std::totally_ordered<urd::index<tyr::planning::match_tree::AtomSelectorNode<Tag>>>
    && std::totally_ordered<urd::data<tyr::planning::match_tree::AtomSelectorNode<Tag>>>
    && std::totally_ordered<urd::index_view<tyr::planning::match_tree::AtomSelectorNode<Tag>, tyr::planning::match_tree::Repository<Tag>>>
    && requires(urd::data<tyr::planning::match_tree::AtomSelectorNode<Tag>>& data,
                const urd::index_view<tyr::planning::match_tree::AtomSelectorNode<Tag>, tyr::planning::match_tree::Repository<Tag>>& view) {
           data.index;
           data.atom;
           data.true_child;
           data.false_child;
           data.dontcare_child;
           view.atom();
           view.true_child();
           view.false_child();
           view.dontcare_child();
       };

using MatchTreeTags = boost::mp11::mp_list<tyr::formalism::Action<::tyr::GroundTag>, tyr::formalism::Axiom<::tyr::GroundTag>>;
static_assert([]<typename... Tags>(boost::mp11::mp_list<Tags...>) { return (AtomSelectorContract<Tags> && ...); }(MatchTreeTags {}));
