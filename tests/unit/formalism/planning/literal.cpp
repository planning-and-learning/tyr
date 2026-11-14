#include "tyr/formalism/literal_data.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace tyr::tests::planning_literal
{

namespace lifted_tests
{

template<typename Entity>
concept LiteralContract = std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>>
                          && std::totally_ordered<urd::data<Entity>> && std::totally_ordered<urd::index_view<Entity, tyr::formalism::planning::Repository>>
                          && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::planning::Repository>& view) {
                                 data.index;
                                 data.atom;
                                 data.polarity;
                                 { data == data } -> std::same_as<bool>;
                                 view.index();
                                 view.get_atom();
                                 view.get_polarity();
                                 { view == view } -> std::same_as<bool>;
                                 { view < view } -> std::same_as<bool>;
                             };

static_assert(LiteralContract<tyr::formalism::Literal<::tyr::LiftedTag, tyr::formalism::StaticTag>>);
static_assert(std::same_as<urd::index_view<tyr::formalism::Literal<::tyr::LiftedTag, tyr::formalism::StaticTag>, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::LiteralView<::tyr::LiftedTag, tyr::formalism::StaticTag>>);
static_assert(LiteralContract<tyr::formalism::Literal<::tyr::LiftedTag, tyr::formalism::FluentTag>>);
static_assert(std::same_as<urd::index_view<tyr::formalism::Literal<::tyr::LiftedTag, tyr::formalism::FluentTag>, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::LiteralView<::tyr::LiftedTag, tyr::formalism::FluentTag>>);
static_assert(LiteralContract<tyr::formalism::Literal<::tyr::LiftedTag, tyr::formalism::DerivedTag>>);
static_assert(std::same_as<urd::index_view<tyr::formalism::Literal<::tyr::LiftedTag, tyr::formalism::DerivedTag>, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::LiteralView<::tyr::LiftedTag, tyr::formalism::DerivedTag>>);
}

namespace ground_tests
{

template<typename Entity>
concept GroundLiteralContract =
    std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>> && std::totally_ordered<urd::data<Entity>>
    && std::totally_ordered<urd::index_view<Entity, tyr::formalism::planning::Repository>>
    && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::planning::Repository>& view) {
           data.index;
           data.atom;
           data.polarity;
           { data == data } -> std::same_as<bool>;
           view.index();
           view.get_atom();
           view.get_polarity();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

static_assert(GroundLiteralContract<tyr::formalism::Literal<::tyr::GroundTag, tyr::formalism::StaticTag>>);
static_assert(std::same_as<urd::index_view<tyr::formalism::Literal<::tyr::GroundTag, tyr::formalism::StaticTag>, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::LiteralView<::tyr::GroundTag, tyr::formalism::StaticTag>>);
static_assert(GroundLiteralContract<tyr::formalism::Literal<::tyr::GroundTag, tyr::formalism::FluentTag>>);
static_assert(std::same_as<urd::index_view<tyr::formalism::Literal<::tyr::GroundTag, tyr::formalism::FluentTag>, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::LiteralView<::tyr::GroundTag, tyr::formalism::FluentTag>>);
static_assert(GroundLiteralContract<tyr::formalism::Literal<::tyr::GroundTag, tyr::formalism::DerivedTag>>);
static_assert(std::same_as<urd::index_view<tyr::formalism::Literal<::tyr::GroundTag, tyr::formalism::DerivedTag>, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::LiteralView<::tyr::GroundTag, tyr::formalism::DerivedTag>>);
}

}  // namespace tyr::tests::planning_literal
