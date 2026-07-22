#include "tyr/formalism/datalog/ground_numeric_effect_operator_data.hpp"
#include "tyr/formalism/datalog/ground_numeric_effect_operator_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>

namespace fd = tyr::formalism::datalog;

template<typename Entity>
struct GroundNumericEffectOperatorPublicView;

template<tyr::formalism::FactKind T>
struct GroundNumericEffectOperatorPublicView<fd::GroundNumericEffectOperator<T>>
{
    using type = fd::GroundNumericEffectOperatorView<T>;
};

template<typename Entity>
concept GroundNumericEffectOperatorContract =
    std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Data<Entity>, fd::Repository>>
    && std::same_as<ygg::View<ygg::Data<Entity>, fd::Repository>, typename GroundNumericEffectOperatorPublicView<Entity>::type>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Data<Entity>, fd::Repository>& view) {
           data.value;
           data.clear();
           view.get_variant();
       };

static_assert([]<typename... Entities>(ygg::TypeList<Entities...>)
              { return (GroundNumericEffectOperatorContract<Entities> && ...); }(fd::GroundNumericEffectOperatorTypes {}));
