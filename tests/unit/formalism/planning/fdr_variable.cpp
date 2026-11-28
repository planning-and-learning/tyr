#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_variable_data.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/repository.hpp"

#include <concepts>
#include <urd/elements/data.hpp>
#include <urd/elements/index.hpp>
#include <urd/elements/view.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::tests::planning_fdr_variable
{

template<typename Entity>
concept FdrVariableContract = std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>>
                              && std::totally_ordered<urd::data<Entity>> && std::totally_ordered<urd::index_view<Entity, tyr::formalism::Repository>>
                              && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::Repository>& view) {
                                     data.index;
                                     data.atoms;
                                     { data == data } -> std::same_as<bool>;
                                     view.index();
                                     view.domain_size();
                                     view.atoms();
                                     { view == view } -> std::same_as<bool>;
                                     { view < view } -> std::same_as<bool>;
                                 };

static_assert(FdrVariableContract<tyr::formalism::FDRVariable<tyr::formalism::FluentTag>>);
static_assert(std::same_as<urd::index_view<tyr::formalism::FDRVariable<tyr::formalism::FluentTag>, tyr::formalism::Repository>,
                           tyr::formalism::FDRVariableView<tyr::formalism::FluentTag>>);

}  // namespace tyr::tests::planning_fdr_variable
