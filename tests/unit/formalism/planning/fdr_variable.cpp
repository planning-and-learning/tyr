#include "tyr/formalism/planning/fdr_variable_data.hpp"
#include "tyr/formalism/planning/fdr_variable_index.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept FdrVariableContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                              && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fp::Repository>>
                              && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fp::Repository>& view) {
                                     data.index;
                                     data.atoms;
                                     data.clear();
                                     { data == data } -> std::same_as<bool>;
                                     view.get_index();
                                     view.get_domain_size();
                                     view.get_atoms();
                                     { view == view } -> std::same_as<bool>;
                                     { view < view } -> std::same_as<bool>;
                                 };

static_assert(FdrVariableContract<fp::FDRVariable<f::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::FDRVariable<f::FluentTag>>, fp::Repository>, fp::FDRVariableView<f::FluentTag>>);
