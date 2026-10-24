#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/literal_data.hpp"
#include "tyr/formalism/planning/literal_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace lifted_tests
{

template<typename Entity>
concept LiteralContract =
    std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>> && std::totally_ordered<ygg::Data<Entity>>
    && std::totally_ordered<ygg::View<ygg::Index<Entity>, tyr::formalism::planning::Repository>>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, tyr::formalism::planning::Repository>& view) {
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

static_assert(LiteralContract<tyr::formalism::planning::Literal<::tyr::LiftedTag, tyr::formalism::StaticTag>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<tyr::formalism::planning::Literal<::tyr::LiftedTag, tyr::formalism::StaticTag>>, tyr::formalism::planning::Repository>,
                 tyr::formalism::planning::LiteralView<::tyr::LiftedTag, tyr::formalism::StaticTag>>);
static_assert(LiteralContract<tyr::formalism::planning::Literal<::tyr::LiftedTag, tyr::formalism::FluentTag>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<tyr::formalism::planning::Literal<::tyr::LiftedTag, tyr::formalism::FluentTag>>, tyr::formalism::planning::Repository>,
                 tyr::formalism::planning::LiteralView<::tyr::LiftedTag, tyr::formalism::FluentTag>>);
static_assert(LiteralContract<tyr::formalism::planning::Literal<::tyr::LiftedTag, tyr::formalism::DerivedTag>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<tyr::formalism::planning::Literal<::tyr::LiftedTag, tyr::formalism::DerivedTag>>, tyr::formalism::planning::Repository>,
                 tyr::formalism::planning::LiteralView<::tyr::LiftedTag, tyr::formalism::DerivedTag>>);
}

namespace ground_tests
{

template<typename Entity>
concept GroundLiteralContract =
    std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>> && std::totally_ordered<ygg::Data<Entity>>
    && std::totally_ordered<ygg::View<ygg::Index<Entity>, tyr::formalism::planning::Repository>>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, tyr::formalism::planning::Repository>& view) {
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

static_assert(GroundLiteralContract<tyr::formalism::planning::Literal<::tyr::GroundTag, tyr::formalism::StaticTag>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<tyr::formalism::planning::Literal<::tyr::GroundTag, tyr::formalism::StaticTag>>, tyr::formalism::planning::Repository>,
                 tyr::formalism::planning::LiteralView<::tyr::GroundTag, tyr::formalism::StaticTag>>);
static_assert(GroundLiteralContract<tyr::formalism::planning::Literal<::tyr::GroundTag, tyr::formalism::FluentTag>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<tyr::formalism::planning::Literal<::tyr::GroundTag, tyr::formalism::FluentTag>>, tyr::formalism::planning::Repository>,
                 tyr::formalism::planning::LiteralView<::tyr::GroundTag, tyr::formalism::FluentTag>>);
static_assert(GroundLiteralContract<tyr::formalism::planning::Literal<::tyr::GroundTag, tyr::formalism::DerivedTag>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<tyr::formalism::planning::Literal<::tyr::GroundTag, tyr::formalism::DerivedTag>>, tyr::formalism::planning::Repository>,
                 tyr::formalism::planning::LiteralView<::tyr::GroundTag, tyr::formalism::DerivedTag>>);
}
