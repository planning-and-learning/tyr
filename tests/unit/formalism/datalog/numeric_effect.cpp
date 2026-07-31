#include "tyr/formalism/datalog/numeric_effect_data.hpp"
#include "tyr/formalism/datalog/numeric_effect_index.hpp"
#include "tyr/formalism/datalog/numeric_effect_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>
#include <tuple>
#include <utility>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

template<typename Entity>
concept NumericEffectContract =
    std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>> && std::totally_ordered<ygg::Data<Entity>>
    && std::totally_ordered<ygg::View<ygg::Index<Entity>, fd::Repository>>
    && std::same_as<std::tuple_element_t<0, decltype(std::declval<const ygg::Data<Entity>&>().identifying_members())>, const f::NumericEffectOperatorKind&>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fd::Repository>& view) {
           data.index;
           { data.operator_kind } -> std::same_as<f::NumericEffectOperatorKind&>;
           data.clear();
           view.get_index();
           { view.get_operator() } -> std::same_as<f::NumericEffectOperatorKind>;
           view.get_fterm();
           view.get_fexpr();
       };

using Fluent = fd::NumericEffect<f::FluentTag>;

static_assert(NumericEffectContract<Fluent>);
static_assert(std::same_as<ygg::View<ygg::Index<Fluent>, fd::Repository>, fd::NumericEffectView<f::FluentTag>>);
static_assert(std::constructible_from<ygg::Data<Fluent>, f::NumericEffectOperatorKind, fd::FunctionTermView<f::FluentTag>, fd::FunctionExpressionView>);
