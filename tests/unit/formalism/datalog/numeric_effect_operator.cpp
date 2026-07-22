#include "tyr/formalism/datalog/numeric_effect_operator_data.hpp"
#include "tyr/formalism/datalog/numeric_effect_operator_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>

namespace fd = tyr::formalism::datalog;

template<typename Entity>
struct NumericEffectOperatorPublicView;

template<tyr::formalism::FactKind T>
struct NumericEffectOperatorPublicView<fd::NumericEffectOperator<T>>
{
    using type = fd::NumericEffectOperatorView<T>;
};

template<typename Entity>
concept NumericEffectOperatorContract = std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Data<Entity>, fd::Repository>>
                                        && std::same_as<ygg::View<ygg::Data<Entity>, fd::Repository>, typename NumericEffectOperatorPublicView<Entity>::type>
                                        && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Data<Entity>, fd::Repository>& view) {
                                               data.value;
                                               data.clear();
                                               view.get_variant();
                                           };

static_assert([]<typename... Entities>(ygg::TypeList<Entities...>)
              { return (NumericEffectOperatorContract<Entities> && ...); }(fd::NumericEffectOperatorTypes {}));
