#include "tyr/planning/ground/match_tree/declarations.hpp"
#include "tyr/planning/ground/match_tree/nodes/variable_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/variable_view.hpp"
#include "tyr/planning/ground/match_tree/repository.hpp"

#include <concepts>

template<typename Tag>
concept VariableSelectorContract =
    std::constructible_from<ygg::Index<tyr::planning::match_tree::VariableSelectorNode<Tag>>, ygg::uint_t>
    && std::totally_ordered<ygg::Index<tyr::planning::match_tree::VariableSelectorNode<Tag>>>
    && std::totally_ordered<ygg::Data<tyr::planning::match_tree::VariableSelectorNode<Tag>>>
    && std::totally_ordered<ygg::View<ygg::Index<tyr::planning::match_tree::VariableSelectorNode<Tag>>, tyr::planning::match_tree::Repository<Tag>>>
    && requires(ygg::Data<tyr::planning::match_tree::VariableSelectorNode<Tag>>& data,
                const ygg::View<ygg::Index<tyr::planning::match_tree::VariableSelectorNode<Tag>>, tyr::planning::match_tree::Repository<Tag>>& view) {
           data.index;
           data.variable;
           data.domain_children;
           data.dontcare_child;
           data.clear();
           view.get_variable();
           view.get_domain_children();
           view.get_dontcare_child();
       };

using MatchTreeTags = ygg::TypeList<tyr::formalism::planning::Action<::tyr::GroundTag>, tyr::formalism::planning::Axiom<::tyr::GroundTag>>;
static_assert([]<typename... Tags>(ygg::TypeList<Tags...>) { return (VariableSelectorContract<Tags> && ...); }(MatchTreeTags {}));
