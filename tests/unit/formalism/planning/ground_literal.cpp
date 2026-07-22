#include "tyr/formalism/planning/ground_literal_data.hpp"
#include "tyr/formalism/planning/ground_literal_index.hpp"
#include "tyr/formalism/planning/ground_literal_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept GroundLiteralContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                                && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fp::Repository>>
                                && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fp::Repository>& view) {
                                       data.index;
                                       data.atom;
                                       data.polarity;
                                       data.clear();
                                       { data == data } -> std::same_as<bool>;
                                       view.get_index();
                                       view.get_atom();
                                       view.get_polarity();
                                       { view == view } -> std::same_as<bool>;
                                       { view < view } -> std::same_as<bool>;
                                   };

static_assert(GroundLiteralContract<fp::GroundLiteral<f::StaticTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::GroundLiteral<f::StaticTag>>, fp::Repository>, fp::GroundLiteralView<f::StaticTag>>);
static_assert(GroundLiteralContract<fp::GroundLiteral<f::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::GroundLiteral<f::FluentTag>>, fp::Repository>, fp::GroundLiteralView<f::FluentTag>>);
static_assert(GroundLiteralContract<fp::GroundLiteral<f::DerivedTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::GroundLiteral<f::DerivedTag>>, fp::Repository>, fp::GroundLiteralView<f::DerivedTag>>);
