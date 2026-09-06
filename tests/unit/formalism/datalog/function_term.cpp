#include "tyr/formalism/datalog/function_term_data.hpp"
#include "tyr/formalism/datalog/function_term_index.hpp"
#include "tyr/formalism/datalog/function_term_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include <concepts>

namespace lifted_tests
{

namespace fd = tyr::formalism::datalog;

template<typename Entity>
struct FunctionTermPublicView;

template<tyr::formalism::FactKind T>
struct FunctionTermPublicView<fd::FunctionTerm<::tyr::LiftedTag, T>>
{
    using type = fd::FunctionTermView<::tyr::LiftedTag, T>;
};

template<typename Entity>
concept FunctionTermContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                               && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fd::Repository>>
                               && std::same_as<ygg::View<ygg::Index<Entity>, fd::Repository>, typename FunctionTermPublicView<Entity>::type>
                               && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fd::Repository>& view) {
                                      data.index;
                                      data.function;
                                      data.terms;
                                      data.clear();
                                      view.get_index();
                                      view.get_function();
                                      view.get_terms();
                                  };

static_assert([]<typename... Entities>(ygg::TypeList<Entities...>) { return (FunctionTermContract<Entities> && ...); }(fd::FunctionTermTypes<::tyr::LiftedTag> {}));
static_assert(std::constructible_from<ygg::Data<fd::FunctionTerm<::tyr::LiftedTag, tyr::formalism::StaticTag>>, fd::FunctionView<tyr::formalism::StaticTag>, fd::TermViewList>);

}

namespace ground_tests
{

namespace fd = tyr::formalism::datalog;

template<typename Entity>
struct GroundFunctionTermPublicView;

template<tyr::formalism::FactKind T>
struct GroundFunctionTermPublicView<fd::FunctionTerm<::tyr::GroundTag, T>>
{
    using type = fd::FunctionTermView<::tyr::GroundTag, T>;
};

template<typename Entity>
concept GroundFunctionTermContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                                     && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fd::Repository>>
                                     && std::same_as<ygg::View<ygg::Index<Entity>, fd::Repository>, typename GroundFunctionTermPublicView<Entity>::type>
                                     && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fd::Repository>& view) {
                                            data.index;
                                            data.binding;
                                            data.clear();
                                            view.get_index();
                                            view.get_function();
                                            view.get_row();
                                            view.get_objects();
                                            view.get_key();
                                        };

static_assert([]<typename... Entities>(ygg::TypeList<Entities...>) { return (GroundFunctionTermContract<Entities> && ...); }(fd::FunctionTermTypes<::tyr::GroundTag> {}));
static_assert(std::constructible_from<ygg::Data<fd::FunctionTerm<::tyr::GroundTag, tyr::formalism::StaticTag>>, fd::FunctionBindingView<tyr::formalism::StaticTag>>);

}
