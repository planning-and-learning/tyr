#include "tyr/planning/ground/match_tree/nodes/node_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/node_view.hpp"
#include "tyr/planning/ground/match_tree/repository.hpp"

#include <concepts>

template<typename Tag>
concept NodeContract = std::totally_ordered<ygg::Data<tyr::planning::match_tree::Node<Tag>>>
                       && std::totally_ordered<ygg::View<ygg::Data<tyr::planning::match_tree::Node<Tag>>, tyr::planning::match_tree::Repository<Tag>>>
                       && requires(ygg::Data<tyr::planning::match_tree::Node<Tag>>& data,
                                   const ygg::View<ygg::Data<tyr::planning::match_tree::Node<Tag>>, tyr::planning::match_tree::Repository<Tag>>& view) {
                              data.variant;
                              data.clear();
                              view.get_variant();
                          };

using MatchTreeTags = ygg::TypeList<tyr::formalism::planning::Action<::tyr::GroundTag>, tyr::formalism::planning::Axiom<::tyr::GroundTag>>;
static_assert([]<typename... Tags>(ygg::TypeList<Tags...>) { return (NodeContract<Tags> && ...); }(MatchTreeTags {}));
