#include "tyr/planning/ground/match_tree/nodes/generator_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/generator_index.hpp"
#include "tyr/planning/ground/match_tree/nodes/generator_view.hpp"
#include "tyr/planning/ground/match_tree/repository.hpp"

#include <concepts>

namespace fp = tyr::formalism::planning;
namespace mt = tyr::planning::match_tree;

template<typename Tag>
concept ElementGeneratorContract =
    std::constructible_from<ygg::Index<mt::ElementGeneratorNode<Tag>>, ygg::uint_t> && std::totally_ordered<ygg::Index<mt::ElementGeneratorNode<Tag>>>
    && std::totally_ordered<ygg::Data<mt::ElementGeneratorNode<Tag>>>
    && std::totally_ordered<ygg::View<ygg::Index<mt::ElementGeneratorNode<Tag>>, mt::Repository<Tag>>>
    && requires(ygg::Data<mt::ElementGeneratorNode<Tag>>& data, const ygg::View<ygg::Index<mt::ElementGeneratorNode<Tag>>, mt::Repository<Tag>>& view) {
           data.index;
           data.elements;
           data.clear();
           view.get_elements();
       };

using MatchTreeTags = ygg::TypeList<fp::Action<::tyr::GroundTag>, fp::Axiom<::tyr::GroundTag>>;
static_assert([]<typename... Tags>(ygg::TypeList<Tags...>) { return (ElementGeneratorContract<Tags> && ...); }(MatchTreeTags {}));
