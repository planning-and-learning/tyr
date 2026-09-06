#include "tyr/formalism/planning/literal_data.hpp"
#include "tyr/formalism/planning/literal_index.hpp"
#include "tyr/formalism/planning/literal_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include <concepts>

namespace lifted_tests
{

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept LiteralContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
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

static_assert(LiteralContract<fp::Literal<::tyr::LiftedTag, f::StaticTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::Literal<::tyr::LiftedTag, f::StaticTag>>, fp::Repository>, fp::LiteralView<::tyr::LiftedTag, f::StaticTag>>);
static_assert(LiteralContract<fp::Literal<::tyr::LiftedTag, f::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::Literal<::tyr::LiftedTag, f::FluentTag>>, fp::Repository>, fp::LiteralView<::tyr::LiftedTag, f::FluentTag>>);
static_assert(LiteralContract<fp::Literal<::tyr::LiftedTag, f::DerivedTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::Literal<::tyr::LiftedTag, f::DerivedTag>>, fp::Repository>, fp::LiteralView<::tyr::LiftedTag, f::DerivedTag>>);

}

namespace ground_tests
{

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

static_assert(GroundLiteralContract<fp::Literal<::tyr::GroundTag, f::StaticTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::Literal<::tyr::GroundTag, f::StaticTag>>, fp::Repository>, fp::LiteralView<::tyr::GroundTag, f::StaticTag>>);
static_assert(GroundLiteralContract<fp::Literal<::tyr::GroundTag, f::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::Literal<::tyr::GroundTag, f::FluentTag>>, fp::Repository>, fp::LiteralView<::tyr::GroundTag, f::FluentTag>>);
static_assert(GroundLiteralContract<fp::Literal<::tyr::GroundTag, f::DerivedTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::Literal<::tyr::GroundTag, f::DerivedTag>>, fp::Repository>, fp::LiteralView<::tyr::GroundTag, f::DerivedTag>>);

}
