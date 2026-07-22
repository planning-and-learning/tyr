#include "tyr/formalism/datalog/ground_atom_data.hpp"
#include "tyr/formalism/datalog/ground_atom_index.hpp"
#include "tyr/formalism/datalog/ground_atom_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>

namespace fd = tyr::formalism::datalog;

template<typename Entity>
struct GroundAtomPublicView;

template<tyr::formalism::FactKind T>
struct GroundAtomPublicView<fd::GroundAtom<T>>
{
    using type = fd::GroundAtomView<T>;
};

template<typename Entity>
concept GroundAtomContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                             && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fd::Repository>>
                             && std::same_as<ygg::View<ygg::Index<Entity>, fd::Repository>, typename GroundAtomPublicView<Entity>::type>
                             && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fd::Repository>& view) {
                                    data.index;
                                    data.binding;
                                    data.clear();
                                    view.get_index();
                                    view.get_predicate();
                                    view.get_row();
                                    view.get_objects();
                                    view.get_key();
                                };

static_assert([]<typename... Entities>(ygg::TypeList<Entities...>) { return (GroundAtomContract<Entities> && ...); }(fd::GroundAtomTypes {}));
