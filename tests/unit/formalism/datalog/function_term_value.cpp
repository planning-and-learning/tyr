#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/function_term_value_data.hpp"
#include "tyr/formalism/datalog/function_term_value_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>

template<typename Entity>
struct GroundFunctionTermValuePublicView;

template<tyr::formalism::FactKind T>
struct GroundFunctionTermValuePublicView<tyr::formalism::datalog::FunctionTermValue<::tyr::GroundTag, T>>
{
    using type = tyr::formalism::datalog::FunctionTermValueView<::tyr::GroundTag, T>;
};

template<typename Entity>
concept GroundFunctionTermValueContract =
    std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>> && std::totally_ordered<urd::data<Entity>>
    && std::totally_ordered<urd::index_view<Entity, tyr::formalism::datalog::Repository>>
    && std::same_as<urd::index_view<Entity, tyr::formalism::datalog::Repository>, typename GroundFunctionTermValuePublicView<Entity>::type>
    && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::datalog::Repository>& view) {
           data.index;
           data.fterm;
           data.value;
           view.index();
           view.get_fterm();
           view.get_value();
       };

static_assert([]<typename... Entities>(boost::mp11::mp_list<Entities...>)
              { return (GroundFunctionTermValueContract<Entities> && ...); }(tyr::formalism::datalog::FunctionTermValueTypes<::tyr::GroundTag> {}));
