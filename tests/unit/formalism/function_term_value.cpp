#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_value_data.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/repository.hpp"

#include <concepts>
#include <urd/elements/data.hpp>
#include <urd/elements/index.hpp>
#include <urd/elements/view.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::tests::formalism_function_term_value
{

template<typename Entity>
concept GroundFunctionTermValueContract =
    std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>> && std::totally_ordered<urd::data<Entity>>
    && std::totally_ordered<urd::index_view<Entity, tyr::formalism::Repository>>
    && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::Repository>& view) {
           data.index;
           data.fterm;
           data.value;
           { data == data } -> std::same_as<bool>;
           view.index();
           view.get_fterm();
           view.get_value();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

static_assert(GroundFunctionTermValueContract<tyr::formalism::FunctionTermValue<::tyr::GroundTag, tyr::formalism::StaticTag>>);
static_assert(std::same_as<urd::index_view<tyr::formalism::FunctionTermValue<::tyr::GroundTag, tyr::formalism::StaticTag>, tyr::formalism::Repository>,
                           tyr::formalism::FunctionTermValueView<::tyr::GroundTag, tyr::formalism::StaticTag>>);
static_assert(GroundFunctionTermValueContract<tyr::formalism::FunctionTermValue<::tyr::GroundTag, tyr::formalism::FluentTag>>);
static_assert(std::same_as<urd::index_view<tyr::formalism::FunctionTermValue<::tyr::GroundTag, tyr::formalism::FluentTag>, tyr::formalism::Repository>,
                           tyr::formalism::FunctionTermValueView<::tyr::GroundTag, tyr::formalism::FluentTag>>);
static_assert(GroundFunctionTermValueContract<tyr::formalism::FunctionTermValue<::tyr::GroundTag, tyr::formalism::AuxiliaryTag>>);
static_assert(std::same_as<urd::index_view<tyr::formalism::FunctionTermValue<::tyr::GroundTag, tyr::formalism::AuxiliaryTag>, tyr::formalism::Repository>,
                           tyr::formalism::FunctionTermValueView<::tyr::GroundTag, tyr::formalism::AuxiliaryTag>>);

}  // namespace tyr::tests::formalism_function_term_value
