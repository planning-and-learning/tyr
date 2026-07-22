#include "tyr/formalism/planning/function_term_data.hpp"
#include "tyr/formalism/planning/function_term_index.hpp"
#include "tyr/formalism/planning/function_term_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept FunctionTermContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                               && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fp::Repository>>
                               && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fp::Repository>& view) {
                                      data.index;
                                      data.function;
                                      data.terms;
                                      data.clear();
                                      { data == data } -> std::same_as<bool>;
                                      view.get_index();
                                      view.get_function();
                                      view.get_terms();
                                      { view == view } -> std::same_as<bool>;
                                      { view < view } -> std::same_as<bool>;
                                  };

static_assert(FunctionTermContract<fp::FunctionTerm<f::StaticTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::FunctionTerm<f::StaticTag>>, fp::Repository>, fp::FunctionTermView<f::StaticTag>>);
static_assert(FunctionTermContract<fp::FunctionTerm<f::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::FunctionTerm<f::FluentTag>>, fp::Repository>, fp::FunctionTermView<f::FluentTag>>);
static_assert(FunctionTermContract<fp::FunctionTerm<f::AuxiliaryTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::FunctionTerm<f::AuxiliaryTag>>, fp::Repository>, fp::FunctionTermView<f::AuxiliaryTag>>);
