#include "tyr/formalism/planning/ground_function_term_value_data.hpp"
#include "tyr/formalism/planning/ground_function_term_value_index.hpp"
#include "tyr/formalism/planning/ground_function_term_value_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept GroundFunctionTermValueContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                                          && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fp::Repository>>
                                          && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fp::Repository>& view) {
                                                 data.index;
                                                 data.fterm;
                                                 data.value;
                                                 data.clear();
                                                 { data == data } -> std::same_as<bool>;
                                                 view.get_index();
                                                 view.get_fterm();
                                                 view.get_value();
                                                 { view == view } -> std::same_as<bool>;
                                                 { view < view } -> std::same_as<bool>;
                                             };

static_assert(GroundFunctionTermValueContract<fp::GroundFunctionTermValue<f::StaticTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::GroundFunctionTermValue<f::StaticTag>>, fp::Repository>, fp::GroundFunctionTermValueView<f::StaticTag>>);
static_assert(GroundFunctionTermValueContract<fp::GroundFunctionTermValue<f::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::GroundFunctionTermValue<f::FluentTag>>, fp::Repository>, fp::GroundFunctionTermValueView<f::FluentTag>>);
static_assert(GroundFunctionTermValueContract<fp::GroundFunctionTermValue<f::AuxiliaryTag>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<fp::GroundFunctionTermValue<f::AuxiliaryTag>>, fp::Repository>, fp::GroundFunctionTermValueView<f::AuxiliaryTag>>);
