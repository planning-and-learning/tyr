#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept FdrFactContract = std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Data<Entity>, fp::Repository>>
                          && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Data<Entity>, fp::Repository>& view) {
                                 data.variable;
                                 data.value;
                                 data.clear();
                                 { data == data } -> std::same_as<bool>;
                                 view.get_variable();
                                 view.get_value();
                                 view.has_value();
                                 view.get_atom();
                                 { view == view } -> std::same_as<bool>;
                                 { view < view } -> std::same_as<bool>;
                             };

static_assert(FdrFactContract<fp::FDRFact<f::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Data<fp::FDRFact<f::FluentTag>>, fp::Repository>, fp::FDRFactView<f::FluentTag>>);
