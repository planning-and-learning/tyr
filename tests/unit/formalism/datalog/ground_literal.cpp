#include "tyr/formalism/datalog/ground_literal_data.hpp"
#include "tyr/formalism/datalog/ground_literal_index.hpp"
#include "tyr/formalism/datalog/ground_literal_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>

namespace fd = tyr::formalism::datalog;

template<typename Entity>
struct GroundLiteralPublicView;

template<tyr::formalism::FactKind T>
struct GroundLiteralPublicView<fd::GroundLiteral<T>>
{
    using type = fd::GroundLiteralView<T>;
};

template<typename Entity>
concept GroundLiteralContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                                && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fd::Repository>>
                                && std::same_as<ygg::View<ygg::Index<Entity>, fd::Repository>, typename GroundLiteralPublicView<Entity>::type>
                                && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fd::Repository>& view) {
                                       data.index;
                                       data.atom;
                                       data.polarity;
                                       data.clear();
                                       view.get_index();
                                       view.get_atom();
                                       view.get_polarity();
                                   };

static_assert([]<typename... Entities>(ygg::TypeList<Entities...>) { return (GroundLiteralContract<Entities> && ...); }(fd::GroundLiteralTypes {}));
