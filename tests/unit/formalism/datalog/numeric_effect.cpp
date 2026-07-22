#include "tyr/formalism/datalog/numeric_effect_data.hpp"
#include "tyr/formalism/datalog/numeric_effect_index.hpp"
#include "tyr/formalism/datalog/numeric_effect_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>

namespace fd = tyr::formalism::datalog;

template<typename Entity>
struct NumericEffectPublicView;

template<tyr::formalism::NumericEffectOpKind Op, tyr::formalism::FactKind T>
struct NumericEffectPublicView<fd::NumericEffect<Op, T>>
{
    using type = fd::NumericEffectView<Op, T>;
};

template<typename Entity>
concept NumericEffectContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                                && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fd::Repository>>
                                && std::same_as<ygg::View<ygg::Index<Entity>, fd::Repository>, typename NumericEffectPublicView<Entity>::type>
                                && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fd::Repository>& view) {
                                       data.index;
                                       data.fterm;
                                       data.fexpr;
                                       data.clear();
                                       view.get_index();
                                       view.get_fterm();
                                       view.get_fexpr();
                                   };

static_assert([]<typename... Entities>(ygg::TypeList<Entities...>) { return (NumericEffectContract<Entities> && ...); }(fd::NumericEffectTypes {}));
