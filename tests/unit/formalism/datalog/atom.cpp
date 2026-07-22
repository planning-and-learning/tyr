#include "tyr/formalism/datalog/atom_data.hpp"
#include "tyr/formalism/datalog/atom_index.hpp"
#include "tyr/formalism/datalog/atom_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>

namespace fd = tyr::formalism::datalog;

template<typename Entity>
struct AtomPublicView;

template<tyr::formalism::FactKind T>
struct AtomPublicView<fd::Atom<T>>
{
    using type = fd::AtomView<T>;
};

template<typename Entity>
concept AtomContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                       && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fd::Repository>>
                       && std::same_as<ygg::View<ygg::Index<Entity>, fd::Repository>, typename AtomPublicView<Entity>::type>
                       && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fd::Repository>& view) {
                              data.index;
                              data.predicate;
                              data.terms;
                              data.clear();
                              view.get_index();
                              view.get_predicate();
                              view.get_terms();
                          };

static_assert([]<typename... Entities>(ygg::TypeList<Entities...>) { return (AtomContract<Entities> && ...); }(fd::AtomTypes {}));
