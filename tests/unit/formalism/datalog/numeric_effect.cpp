#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/numeric_effect_data.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"

#include <concepts>
#include <tuple>
#include <utility>

namespace tyr::tests::datalog_numeric_effect
{

namespace lifted_tests
{

template<typename Entity>
concept NumericEffectContract =
    std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>> && std::totally_ordered<urd::data<Entity>>
    && std::totally_ordered<urd::index_view<Entity, tyr::formalism::datalog::Repository>>
    && std::same_as<std::remove_cvref_t<std::tuple_element_t<0, urd::detail::identity_t<urd::data<Entity>>>>, tyr::formalism::NumericEffectOperatorKind>
    && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::datalog::Repository>& view) {
           data.index;
           { data.operator_kind } -> std::same_as<tyr::formalism::NumericEffectOperatorKind&>;
           view.index();
           { view.get_operator() } -> std::same_as<tyr::formalism::NumericEffectOperatorKind>;
           view.get_fterm();
           view.get_fexpr();
       };

using Fluent = tyr::formalism::NumericEffect<::tyr::LiftedTag, tyr::formalism::FluentTag>;

static_assert(NumericEffectContract<Fluent>);
static_assert(std::same_as<urd::index_view<Fluent, tyr::formalism::datalog::Repository>,
                           tyr::formalism::datalog::NumericEffectView<::tyr::LiftedTag, tyr::formalism::FluentTag>>);
}

namespace ground_tests
{

template<typename Entity>
concept GroundNumericEffectContract =
    std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>> && std::totally_ordered<urd::data<Entity>>
    && std::totally_ordered<urd::index_view<Entity, tyr::formalism::datalog::Repository>>
    && std::same_as<std::remove_cvref_t<std::tuple_element_t<0, urd::detail::identity_t<urd::data<Entity>>>>, tyr::formalism::NumericEffectOperatorKind>
    && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::datalog::Repository>& view) {
           data.index;
           { data.operator_kind } -> std::same_as<tyr::formalism::NumericEffectOperatorKind&>;
           view.index();
           { view.get_operator() } -> std::same_as<tyr::formalism::NumericEffectOperatorKind>;
           view.get_fterm();
           view.get_fexpr();
       };

using Fluent = tyr::formalism::NumericEffect<::tyr::GroundTag, tyr::formalism::FluentTag>;

static_assert(GroundNumericEffectContract<Fluent>);
static_assert(std::same_as<urd::index_view<Fluent, tyr::formalism::datalog::Repository>,
                           tyr::formalism::datalog::NumericEffectView<::tyr::GroundTag, tyr::formalism::FluentTag>>);
}

}  // namespace tyr::tests::datalog_numeric_effect
