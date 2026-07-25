#include "tyr/formalism/binding_data.hpp"
#include "tyr/formalism/binding_index.hpp"
#include "tyr/formalism/binding_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;
namespace fp = tyr::formalism::planning;

template<typename Relation, typename Repository>
struct BindingPublicView;

template<f::FactKind T>
struct BindingPublicView<f::Predicate<T>, fd::Repository>
{
    using type = fd::PredicateBindingView<T>;
};

template<f::FactKind T>
struct BindingPublicView<f::Function<T>, fd::Repository>
{
    using type = fd::FunctionBindingView<T>;
};

template<f::RelationKind R>
struct BindingPublicView<fd::Rule<R>, fd::Repository>
{
    using type = fd::RuleBindingView<R>;
};

template<f::FactKind T>
struct BindingPublicView<f::Predicate<T>, fp::Repository>
{
    using type = fp::PredicateBindingView<T>;
};

template<f::FactKind T>
struct BindingPublicView<f::Function<T>, fp::Repository>
{
    using type = fp::FunctionBindingView<T>;
};

template<>
struct BindingPublicView<fp::Action, fp::Repository>
{
    using type = fp::ActionBindingView;
};

template<>
struct BindingPublicView<fp::Axiom, fp::Repository>
{
    using type = fp::AxiomBindingView;
};

template<typename Relation, typename Repository>
concept BindingContract =
    f::RelationBindingConcept<f::RelationBinding<Relation>> && std::totally_ordered<ygg::Index<f::RelationBinding<Relation>>>
    && std::totally_ordered<ygg::Data<f::RelationBinding<Relation>>> && std::totally_ordered<ygg::View<ygg::Index<f::RelationBinding<Relation>>, Repository>>
    && std::same_as<ygg::View<ygg::Index<f::RelationBinding<Relation>>, Repository>, typename BindingPublicView<Relation, Repository>::type>
    && requires(ygg::Index<f::RelationBinding<Relation>>& index,
                ygg::Data<f::RelationBinding<Relation>>& data,
                const ygg::View<ygg::Index<f::RelationBinding<Relation>>, Repository>& view) {
           index.relation;
           index.row;
           data.relation;
           data.objects;
           data.clear();
           view.get_index();
           view.get_relation();
           view.get_objects();
           view.get_key();
       };

template<typename Repository, typename... Relations>
consteval bool binding_contracts(ygg::TypeList<Relations...>)
{
    return (BindingContract<Relations, Repository> && ...);
}

static_assert(binding_contracts<fd::Repository>(fd::RelationRepositoryTypes {}));
static_assert(binding_contracts<fp::Repository>(fp::RelationRepositoryTypes {}));

using Binding = f::RelationBinding<f::Predicate<f::StaticTag>>;
static_assert(std::same_as<Binding, ygg::formalism::RelationBinding<f::Predicate<f::StaticTag>, f::ObjectTag>>);
