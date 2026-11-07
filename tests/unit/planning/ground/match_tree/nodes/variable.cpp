#include "tyr/planning/ground/match_tree/declarations.hpp"
#include "tyr/planning/ground/match_tree/nodes/node_view.hpp"
#include "tyr/planning/ground/match_tree/nodes/variable_data.hpp"
#include "tyr/planning/ground/match_tree/repository.hpp"

#include <concepts>

template<typename Tag>
concept VariableSelectorContract =
    std::constructible_from<urd::index<tyr::planning::match_tree::VariableSelectorNode<Tag>>, ygg::uint_t>
    && std::totally_ordered<urd::index<tyr::planning::match_tree::VariableSelectorNode<Tag>>>
    && std::totally_ordered<urd::data<tyr::planning::match_tree::VariableSelectorNode<Tag>>>
    && std::totally_ordered<urd::index_view<tyr::planning::match_tree::VariableSelectorNode<Tag>, tyr::planning::match_tree::Repository<Tag>>>
    && requires(urd::data<tyr::planning::match_tree::VariableSelectorNode<Tag>>& data,
                const urd::index_view<tyr::planning::match_tree::VariableSelectorNode<Tag>, tyr::planning::match_tree::Repository<Tag>>& view) {
           data.index;
           data.variable;
           data.domain_children;
           data.dontcare_child;
           view.get_variable();
           view.get_domain_children();
           view.get_dontcare_child();
       };

using MatchTreeTags = boost::mp11::mp_list<tyr::formalism::planning::Action<::tyr::GroundTag>, tyr::formalism::planning::Axiom<::tyr::GroundTag>>;
static_assert([]<typename... Tags>(boost::mp11::mp_list<Tags...>) { return (VariableSelectorContract<Tags> && ...); }(MatchTreeTags {}));
