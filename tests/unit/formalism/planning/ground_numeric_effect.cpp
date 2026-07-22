#include "tyr/formalism/planning/ground_numeric_effect_data.hpp"
#include "tyr/formalism/planning/ground_numeric_effect_index.hpp"
#include "tyr/formalism/planning/ground_numeric_effect_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept GroundNumericEffectContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
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

static_assert(GroundNumericEffectContract<fp::GroundNumericEffect<f::Assign, f::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::GroundNumericEffect<f::Assign, f::FluentTag>>, fp::Repository>,
                           fp::GroundNumericEffectView<f::Assign, f::FluentTag>>);
static_assert(GroundNumericEffectContract<fp::GroundNumericEffect<f::Increase, f::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::GroundNumericEffect<f::Increase, f::FluentTag>>, fp::Repository>,
                           fp::GroundNumericEffectView<f::Increase, f::FluentTag>>);
static_assert(GroundNumericEffectContract<fp::GroundNumericEffect<f::Decrease, f::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::GroundNumericEffect<f::Decrease, f::FluentTag>>, fp::Repository>,
                           fp::GroundNumericEffectView<f::Decrease, f::FluentTag>>);
static_assert(GroundNumericEffectContract<fp::GroundNumericEffect<f::ScaleUp, f::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::GroundNumericEffect<f::ScaleUp, f::FluentTag>>, fp::Repository>,
                           fp::GroundNumericEffectView<f::ScaleUp, f::FluentTag>>);
static_assert(GroundNumericEffectContract<fp::GroundNumericEffect<f::ScaleDown, f::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::GroundNumericEffect<f::ScaleDown, f::FluentTag>>, fp::Repository>,
                           fp::GroundNumericEffectView<f::ScaleDown, f::FluentTag>>);
static_assert(GroundNumericEffectContract<fp::GroundNumericEffect<f::Increase, f::AuxiliaryTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::GroundNumericEffect<f::Increase, f::AuxiliaryTag>>, fp::Repository>,
                           fp::GroundNumericEffectView<f::Increase, f::AuxiliaryTag>>);
