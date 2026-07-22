#include "tyr/formalism/datalog/function_term_data.hpp"
#include "tyr/formalism/datalog/function_term_index.hpp"
#include "tyr/formalism/datalog/function_term_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>

namespace fd = tyr::formalism::datalog;

template<typename Entity>
struct FunctionTermPublicView;

template<tyr::formalism::FactKind T>
struct FunctionTermPublicView<fd::FunctionTerm<T>>
{
    using type = fd::FunctionTermView<T>;
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

static_assert([]<typename... Entities>(ygg::TypeList<Entities...>) { return (FunctionTermContract<Entities> && ...); }(fd::FunctionTermTypes {}));
