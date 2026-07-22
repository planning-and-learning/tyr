#include "tyr/planning/state_data.hpp"
#include "tyr/planning/state_index.hpp"
#include "tyr/planning/state_view.hpp"

#include <concepts>

namespace p = tyr::planning;

template<typename Kind>
concept StateIndexContract = std::constructible_from<ygg::Index<p::State<Kind>>, ygg::uint_t> && std::totally_ordered<ygg::Index<p::State<Kind>>>;

using StateKinds = ygg::TypeList<tyr::GroundTag, tyr::LiftedTag>;
static_assert([]<typename... Kinds>(ygg::TypeList<Kinds...>) { return (StateIndexContract<Kinds> && ...); }(StateKinds {}));
