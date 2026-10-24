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
    std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>> && std::totally_ordered<ygg::Data<Entity>>
    && std::totally_ordered<ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>>
    && std::same_as<ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>, typename FunctionTermPublicView<Entity>::type>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>& view) {
           data.index;
           data.function;
           data.terms;
           data.clear();
           view.get_index();
           view.get_function();
           view.get_terms();
       };

static_assert([]<typename... Entities>(ygg::TypeList<Entities...>)
              { return (FunctionTermContract<Entities> && ...); }(tyr::formalism::datalog::FunctionTermTypes<::tyr::LiftedTag> {}));
static_assert(std::constructible_from<ygg::Data<tyr::formalism::datalog::FunctionTerm<::tyr::LiftedTag, tyr::formalism::StaticTag>>,
                                      tyr::formalism::datalog::FunctionView<tyr::formalism::StaticTag>,
                                      tyr::formalism::datalog::TermViewList>);
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
    std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>> && std::totally_ordered<ygg::Data<Entity>>
    && std::totally_ordered<ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>>
    && std::same_as<ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>, typename GroundFunctionTermPublicView<Entity>::type>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>& view) {
           data.index;
           data.binding;
           data.clear();
           view.get_index();
           view.get_function();
           view.get_row();
           view.get_objects();
           view.get_key();
       };

static_assert([]<typename... Entities>(ygg::TypeList<Entities...>)
              { return (GroundFunctionTermContract<Entities> && ...); }(tyr::formalism::datalog::FunctionTermTypes<::tyr::GroundTag> {}));
static_assert(std::constructible_from<ygg::Data<tyr::formalism::datalog::FunctionTerm<::tyr::GroundTag, tyr::formalism::StaticTag>>,
                                      tyr::formalism::datalog::FunctionBindingView<tyr::formalism::StaticTag>>);
}
