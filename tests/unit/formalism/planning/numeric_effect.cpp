#include "tyr/formalism/planning/numeric_effect_data.hpp"
#include "tyr/formalism/planning/numeric_effect_index.hpp"
#include "tyr/formalism/planning/numeric_effect_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept NumericEffectContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                                && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fp::Repository>>
                                && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fp::Repository>& view) {
                                       data.index;
                                       data.fterm;
                                       data.fexpr;
                                       data.clear();
                                       { data == data } -> std::same_as<bool>;
                                       view.get_index();
                                       view.get_fterm();
                                       view.get_fexpr();
                                       { view == view } -> std::same_as<bool>;
                                       { view < view } -> std::same_as<bool>;
                                   };

static_assert(NumericEffectContract<fp::NumericEffect<f::Assign, f::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::NumericEffect<f::Assign, f::FluentTag>>, fp::Repository>, fp::NumericEffectView<f::Assign, f::FluentTag>>);
static_assert(NumericEffectContract<fp::NumericEffect<f::Increase, f::FluentTag>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<fp::NumericEffect<f::Increase, f::FluentTag>>, fp::Repository>, fp::NumericEffectView<f::Increase, f::FluentTag>>);
static_assert(NumericEffectContract<fp::NumericEffect<f::Decrease, f::FluentTag>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<fp::NumericEffect<f::Decrease, f::FluentTag>>, fp::Repository>, fp::NumericEffectView<f::Decrease, f::FluentTag>>);
static_assert(NumericEffectContract<fp::NumericEffect<f::ScaleUp, f::FluentTag>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<fp::NumericEffect<f::ScaleUp, f::FluentTag>>, fp::Repository>, fp::NumericEffectView<f::ScaleUp, f::FluentTag>>);
static_assert(NumericEffectContract<fp::NumericEffect<f::ScaleDown, f::FluentTag>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<fp::NumericEffect<f::ScaleDown, f::FluentTag>>, fp::Repository>, fp::NumericEffectView<f::ScaleDown, f::FluentTag>>);
static_assert(NumericEffectContract<fp::NumericEffect<f::Increase, f::AuxiliaryTag>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<fp::NumericEffect<f::Increase, f::AuxiliaryTag>>, fp::Repository>, fp::NumericEffectView<f::Increase, f::AuxiliaryTag>>);
