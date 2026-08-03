#include "tyr/formalism/binding_data.hpp"
#include "tyr/formalism/binding_index.hpp"
#include "tyr/formalism/binding_view.hpp"
#include "tyr/formalism/datalog/merge.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <string>
#include <utility>

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

#if defined(TYR_RELATION_STORAGE_WORD)
static_assert(std::same_as<typename ygg::formalism::RelationRepositoryTraits<f::ObjectTag>::storage_type,
                           ygg::formalism::BlockArraySetStorage>);
#else
static_assert(std::same_as<typename ygg::formalism::RelationRepositoryTraits<f::ObjectTag>::storage_type,
                           ygg::formalism::BitPackedArraySetStorage>);
#endif

TEST(TyrFormalismDatalogMergeTest, ReinternsBindingRelationsAndObjects)
{
    auto source = fd::RepositoryFactory().create();
    auto destination = fd::RepositoryFactory().create();

    const auto intern = []<typename T>(fd::Repository& repository, ygg::Data<T> data)
    {
        canonicalize(data);
        return repository.get_or_create(data).first;
    };

    const auto source_a = intern(source, ygg::Data<f::Object>(std::string("a")));
    (void) intern(source, ygg::Data<f::Object>(std::string("b")));
    (void) intern(destination, ygg::Data<f::Object>(std::string("b")));
    const auto destination_a = intern(destination, ygg::Data<f::Object>(std::string("a")));

    using Predicate = f::Predicate<f::FluentTag>;
    const auto source_predicate = intern(source, ygg::Data<Predicate>(std::string("p"), 1));
    (void) intern(source, ygg::Data<Predicate>(std::string("q"), 1));
    (void) intern(destination, ygg::Data<Predicate>(std::string("q"), 1));
    const auto destination_predicate = intern(destination, ygg::Data<Predicate>(std::string("p"), 1));

    auto predicate_binding_data = ygg::Data<f::RelationBinding<Predicate>> {};
    predicate_binding_data.relation = source_predicate.get_index();
    predicate_binding_data.objects.push_back(source_a.get_index());
    const auto source_predicate_binding = intern(source, std::move(predicate_binding_data));

    using Function = f::Function<f::FluentTag>;
    const auto source_function = intern(source, ygg::Data<Function>(std::string("f"), 1));
    (void) intern(source, ygg::Data<Function>(std::string("g"), 1));
    (void) intern(destination, ygg::Data<Function>(std::string("g"), 1));
    const auto destination_function = intern(destination, ygg::Data<Function>(std::string("f"), 1));

    auto function_binding_data = ygg::Data<f::RelationBinding<Function>> {};
    function_binding_data.relation = source_function.get_index();
    function_binding_data.objects.push_back(source_a.get_index());
    const auto source_function_binding = intern(source, std::move(function_binding_data));

    ASSERT_NE(source_a.get_index(), destination_a.get_index());
    ASSERT_NE(source_predicate.get_index(), destination_predicate.get_index());
    ASSERT_NE(source_function.get_index(), destination_function.get_index());

    auto builder = fd::Builder {};
    auto context = fd::MergeContext { builder, destination };
    const auto merged_predicate_binding = fd::merge_d2d(source_predicate_binding, context).first;
    const auto merged_function_binding = fd::merge_d2d(source_function_binding, context).first;

    EXPECT_EQ(merged_predicate_binding.get_relation(), destination_predicate);
    ASSERT_EQ(merged_predicate_binding.get_objects().size(), 1);
    EXPECT_EQ(merged_predicate_binding.get_objects()[0], destination_a);
    EXPECT_EQ(merged_function_binding.get_relation(), destination_function);
    ASSERT_EQ(merged_function_binding.get_objects().size(), 1);
    EXPECT_EQ(merged_function_binding.get_objects()[0], destination_a);
}
