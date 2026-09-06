#include "tyr/planning/ground/match_tree/nodes/atom_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/atom_index.hpp"
#include "tyr/planning/ground/match_tree/nodes/atom_view.hpp"
#include "tyr/planning/ground/match_tree/repository.hpp"

#include <concepts>

namespace fp = tyr::formalism::planning;
namespace mt = tyr::planning::match_tree;

template<typename Tag>
concept AtomSelectorContract =
    std::constructible_from<ygg::Index<mt::AtomSelectorNode<Tag>>, ygg::uint_t> && std::totally_ordered<ygg::Index<mt::AtomSelectorNode<Tag>>>
    && std::totally_ordered<ygg::Data<mt::AtomSelectorNode<Tag>>> && std::totally_ordered<ygg::View<ygg::Index<mt::AtomSelectorNode<Tag>>, mt::Repository<Tag>>>
    && requires(ygg::Data<mt::AtomSelectorNode<Tag>>& data, const ygg::View<ygg::Index<mt::AtomSelectorNode<Tag>>, mt::Repository<Tag>>& view) {
           data.index;
           data.atom;
           data.true_child;
           data.false_child;
           data.dontcare_child;
           data.clear();
           view.get_atom();
           view.get_true_child();
           view.get_false_child();
           view.get_dontcare_child();
       };

using MatchTreeTags = ygg::TypeList<fp::Action<::tyr::GroundTag>, fp::Axiom<::tyr::GroundTag>>;
static_assert([]<typename... Tags>(ygg::TypeList<Tags...>) { return (AtomSelectorContract<Tags> && ...); }(MatchTreeTags {}));
