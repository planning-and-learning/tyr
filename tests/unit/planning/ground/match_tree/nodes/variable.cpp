#include "tyr/planning/ground/match_tree/nodes/variable_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/variable_index.hpp"
#include "tyr/planning/ground/match_tree/nodes/variable_view.hpp"
#include "tyr/planning/ground/match_tree/repository.hpp"

#include <concepts>

namespace fp = tyr::formalism::planning;
namespace mt = tyr::planning::match_tree;

template<typename Tag>
concept VariableSelectorContract =
    std::constructible_from<ygg::Index<mt::VariableSelectorNode<Tag>>, ygg::uint_t> && std::totally_ordered<ygg::Index<mt::VariableSelectorNode<Tag>>>
    && std::totally_ordered<ygg::Data<mt::VariableSelectorNode<Tag>>>
    && std::totally_ordered<ygg::View<ygg::Index<mt::VariableSelectorNode<Tag>>, mt::Repository<Tag>>>
    && requires(ygg::Data<mt::VariableSelectorNode<Tag>>& data, const ygg::View<ygg::Index<mt::VariableSelectorNode<Tag>>, mt::Repository<Tag>>& view) {
           data.index;
           data.variable;
           data.domain_children;
           data.dontcare_child;
           data.clear();
           view.get_variable();
           view.get_domain_children();
           view.get_dontcare_child();
       };

using MatchTreeTags = ygg::TypeList<fp::GroundAction, fp::GroundAxiom>;
static_assert([]<typename... Tags>(ygg::TypeList<Tags...>) { return (VariableSelectorContract<Tags> && ...); }(MatchTreeTags {}));
