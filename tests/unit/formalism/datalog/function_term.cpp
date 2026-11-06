#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/function_term_data.hpp"
#include "tyr/formalism/datalog/function_term_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>

namespace lifted_tests
{

template<typename Entity>
struct FunctionTermPublicView;

template<tyr::formalism::FactKind T>
struct FunctionTermPublicView<tyr::formalism::datalog::FunctionTerm<::tyr::LiftedTag, T>>
{
    using type = tyr::formalism::datalog::FunctionTermView<::tyr::LiftedTag, T>;
};

template<typename Entity>
concept FunctionTermContract =
    std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>> && std::totally_ordered<urd::data<Entity>>
    && std::totally_ordered<urd::view<Entity, tyr::formalism::datalog::Repository>>
    && std::same_as<urd::view<Entity, tyr::formalism::datalog::Repository>, typename FunctionTermPublicView<Entity>::type>
    && requires(urd::data<Entity>& data, const urd::view<Entity, tyr::formalism::datalog::Repository>& view) {
           data.index;
           data.function;
           data.terms;
           view.index();
           view.get_function();
           view.get_terms();
       };

static_assert([]<typename... Entities>(boost::mp11::mp_list<Entities...>)
              { return (FunctionTermContract<Entities> && ...); }(tyr::formalism::datalog::FunctionTermTypes<::tyr::LiftedTag> {}));
}

namespace ground_tests
{

template<typename Entity>
struct GroundFunctionTermPublicView;

template<tyr::formalism::FactKind T>
struct GroundFunctionTermPublicView<tyr::formalism::datalog::FunctionTerm<::tyr::GroundTag, T>>
{
    using type = tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, T>;
};

template<typename Entity>
concept GroundFunctionTermContract =
    std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>> && std::totally_ordered<urd::data<Entity>>
    && std::totally_ordered<urd::view<Entity, tyr::formalism::datalog::Repository>>
    && std::same_as<urd::view<Entity, tyr::formalism::datalog::Repository>, typename GroundFunctionTermPublicView<Entity>::type>
    && requires(urd::data<Entity>& data, const urd::view<Entity, tyr::formalism::datalog::Repository>& view) {
           data.index;
           data.binding;
           view.index();
           view.get_function();
           view.get_row();
           view.get_objects();
           view.get_key();
       };

static_assert([]<typename... Entities>(boost::mp11::mp_list<Entities...>)
              { return (GroundFunctionTermContract<Entities> && ...); }(tyr::formalism::datalog::FunctionTermTypes<::tyr::GroundTag> {}));
}
