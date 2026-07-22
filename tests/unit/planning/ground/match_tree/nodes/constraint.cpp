#include "tyr/planning/ground/match_tree/nodes/constraint_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/constraint_index.hpp"
#include "tyr/planning/ground/match_tree/nodes/constraint_view.hpp"
#include "tyr/planning/ground/match_tree/repository.hpp"

#include <concepts>

namespace fp = tyr::formalism::planning;
namespace mt = tyr::planning::match_tree;

template<typename Tag>
concept NumericConstraintSelectorContract =
    std::constructible_from<ygg::Index<mt::NumericConstraintSelectorNode<Tag>>, ygg::uint_t>
    && std::totally_ordered<ygg::Index<mt::NumericConstraintSelectorNode<Tag>>> && std::totally_ordered<ygg::Data<mt::NumericConstraintSelectorNode<Tag>>>
    && std::totally_ordered<ygg::View<ygg::Index<mt::NumericConstraintSelectorNode<Tag>>, mt::Repository<Tag>>>
    && requires(ygg::Data<mt::NumericConstraintSelectorNode<Tag>>& data,
                const ygg::View<ygg::Index<mt::NumericConstraintSelectorNode<Tag>>, mt::Repository<Tag>>& view) {
           data.index;
           data.constraint;
           data.true_child;
           data.dontcare_child;
           data.clear();
           view.get_constraint();
           view.get_true_child();
           view.get_dontcare_child();
       };

using MatchTreeTags = ygg::TypeList<fp::GroundAction, fp::GroundAxiom>;
static_assert([]<typename... Tags>(ygg::TypeList<Tags...>) { return (NumericConstraintSelectorContract<Tags> && ...); }(MatchTreeTags {}));
