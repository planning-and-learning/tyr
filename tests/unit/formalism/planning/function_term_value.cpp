#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/function_term_value_data.hpp"
#include "tyr/formalism/planning/function_term_value_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

template<typename Entity>
concept GroundFunctionTermValueContract =
    std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>> && std::totally_ordered<ygg::Data<Entity>>
    && std::totally_ordered<ygg::View<ygg::Index<Entity>, tyr::formalism::planning::Repository>>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, tyr::formalism::planning::Repository>& view) {
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

static_assert(GroundFunctionTermValueContract<tyr::formalism::planning::FunctionTermValue<::tyr::GroundTag, tyr::formalism::StaticTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<tyr::formalism::planning::FunctionTermValue<::tyr::GroundTag, tyr::formalism::StaticTag>>,
                                     tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::FunctionTermValueView<::tyr::GroundTag, tyr::formalism::StaticTag>>);
static_assert(GroundFunctionTermValueContract<tyr::formalism::planning::FunctionTermValue<::tyr::GroundTag, tyr::formalism::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<tyr::formalism::planning::FunctionTermValue<::tyr::GroundTag, tyr::formalism::FluentTag>>,
                                     tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::FunctionTermValueView<::tyr::GroundTag, tyr::formalism::FluentTag>>);
static_assert(GroundFunctionTermValueContract<tyr::formalism::planning::FunctionTermValue<::tyr::GroundTag, tyr::formalism::AuxiliaryTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<tyr::formalism::planning::FunctionTermValue<::tyr::GroundTag, tyr::formalism::AuxiliaryTag>>,
                                     tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::FunctionTermValueView<::tyr::GroundTag, tyr::formalism::AuxiliaryTag>>);
