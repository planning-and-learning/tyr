#include "tyr/formalism/datalog/function_term_value_data.hpp"
#include "tyr/formalism/datalog/function_term_value_index.hpp"
#include "tyr/formalism/datalog/function_term_value_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>

namespace fd = tyr::formalism::datalog;

template<typename Entity>
struct GroundFunctionTermValuePublicView;

template<tyr::formalism::FactKind T>
struct GroundFunctionTermValuePublicView<fd::FunctionTermValue<::tyr::GroundTag, T>>
{
    using type = fd::FunctionTermValueView<::tyr::GroundTag, T>;
};

template<typename Entity>
concept GroundFunctionTermValueContract =
    std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>> && std::totally_ordered<ygg::Data<Entity>>
    && std::totally_ordered<ygg::View<ygg::Index<Entity>, fd::Repository>>
    && std::same_as<ygg::View<ygg::Index<Entity>, fd::Repository>, typename GroundFunctionTermValuePublicView<Entity>::type>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fd::Repository>& view) {
           data.index;
           data.fterm;
           data.value;
           data.clear();
           view.get_index();
           view.get_fterm();
           view.get_value();
       };

static_assert([]<typename... Entities>(ygg::TypeList<Entities...>)
              { return (GroundFunctionTermValueContract<Entities> && ...); }(fd::FunctionTermValueTypes<::tyr::GroundTag> {}));
static_assert(std::constructible_from<ygg::Data<fd::FunctionTermValue<::tyr::GroundTag, tyr::formalism::StaticTag>>,
                                      fd::FunctionTermView<::tyr::GroundTag, tyr::formalism::StaticTag>,
                                      ygg::float_t>);
