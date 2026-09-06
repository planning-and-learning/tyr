#include "tyr/planning/ground/match_tree/nodes/node_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/node_view.hpp"
#include "tyr/planning/ground/match_tree/repository.hpp"

#include <concepts>

namespace fp = tyr::formalism::planning;
namespace mt = tyr::planning::match_tree;

template<typename Tag>
concept NodeContract = std::totally_ordered<ygg::Data<mt::Node<Tag>>> && std::totally_ordered<ygg::View<ygg::Data<mt::Node<Tag>>, mt::Repository<Tag>>>
                       && requires(ygg::Data<mt::Node<Tag>>& data, const ygg::View<ygg::Data<mt::Node<Tag>>, mt::Repository<Tag>>& view) {
                              data.value;
                              data.clear();
                              view.get_variant();
                          };

using MatchTreeTags = ygg::TypeList<fp::Action<::tyr::GroundTag>, fp::Axiom<::tyr::GroundTag>>;
static_assert([]<typename... Tags>(ygg::TypeList<Tags...>) { return (NodeContract<Tags> && ...); }(MatchTreeTags {}));
