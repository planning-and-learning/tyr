#include "tyr/formalism/binding_data.hpp"
#include "tyr/formalism/binding_index.hpp"
#include "tyr/formalism/binding_view.hpp"
#include "tyr/formalism/datalog/copy.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/planning/copy.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <forward_list>
#include <gtest/gtest.h>
#include <string>
#include <utility>
#include <vector>

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
struct BindingPublicView<fd::Rule<::tyr::LiftedTag, R>, fd::Repository>
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
struct BindingPublicView<fp::Action<::tyr::LiftedTag>, fp::Repository>
{
    using type = fp::ActionBindingView;
};

template<>
struct BindingPublicView<fp::Axiom<::tyr::LiftedTag>, fp::Repository>
{
    using type = fp::AxiomBindingView;
};

template<typename Relation, typename Repository>
concept BindingContract =
    ygg::ViewConcept<ygg::Index<f::RelationBinding<Relation>>, Repository> && f::RelationBindingConcept<f::RelationBinding<Relation>>
    && std::totally_ordered<ygg::Index<f::RelationBinding<Relation>>> && std::totally_ordered<ygg::Data<f::RelationBinding<Relation>>>
    && std::totally_ordered<ygg::View<ygg::Index<f::RelationBinding<Relation>>, Repository>>
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

template<typename Range>
concept ForwardBindingRange = requires { typename f::RelationBindingsForwardRange<f::Predicate<f::StaticTag>, Range>; };

template<typename Range>
concept RandomAccessBindingRange = requires { typename f::RelationBindingsRandomAccessRange<f::Predicate<f::StaticTag>, Range>; };

static_assert(ForwardBindingRange<std::vector<ygg::Index<f::Row>>>);
static_assert(RandomAccessBindingRange<std::vector<ygg::Index<f::Row>>>);
static_assert(ForwardBindingRange<std::forward_list<ygg::Index<f::Row>>>);
static_assert(!RandomAccessBindingRange<std::forward_list<ygg::Index<f::Row>>>);
static_assert(!ForwardBindingRange<std::vector<int>>);
static_assert(!RandomAccessBindingRange<std::vector<int>>);
static_assert(!ForwardBindingRange<int>);
static_assert(!RandomAccessBindingRange<int>);

using Binding = f::RelationBinding<f::Predicate<f::StaticTag>>;
static_assert(std::same_as<Binding, ygg::formalism::RelationBinding<f::Predicate<f::StaticTag>, f::ObjectTag>>);

#if defined(TYR_RELATION_STORAGE_WORD)
static_assert(std::same_as<typename ygg::formalism::RelationRepositoryTraits<f::ObjectTag>::storage_type, ygg::formalism::BlockArraySetStorage>);
#else
static_assert(std::same_as<typename ygg::formalism::RelationRepositoryTraits<f::ObjectTag>::storage_type, ygg::formalism::BitPackedArraySetStorage>);
#endif

template<typename Factory, typename Builder, typename Context>
void check_binding_copy()
{
    auto source = Factory().create();
    auto destination = Factory().create();

    const auto intern = []<typename T>(auto& repository, ygg::Data<T> data) { return ygg::formalism::insert(repository, data).first; };

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

    auto builder = Builder {};
    auto context = Context { builder, destination };
    const auto merged_predicate_binding = copy(source_predicate_binding, context).first;
    const auto merged_function_binding = copy(source_function_binding, context).first;

    EXPECT_EQ(merged_predicate_binding.get_relation(), destination_predicate);
    ASSERT_EQ(merged_predicate_binding.get_objects().size(), 1);
    EXPECT_EQ(merged_predicate_binding.get_objects()[0], destination_a);
    EXPECT_EQ(merged_function_binding.get_relation(), destination_function);
    ASSERT_EQ(merged_function_binding.get_objects().size(), 1);
    EXPECT_EQ(merged_function_binding.get_objects()[0], destination_a);
    EXPECT_FALSE(copy(source_predicate_binding, context).second);
    EXPECT_FALSE(copy(source_function_binding, context).second);
}

TEST(TyrFormalismDatalogCopy, RemapsBindingRelationsAndObjects) { check_binding_copy<fd::RepositoryFactory, fd::Builder, fd::CopyContext>(); }

TEST(TyrFormalismPlanningCopy, RemapsBindingRelationsAndObjects) { check_binding_copy<fp::RepositoryFactory, fp::Builder, fp::CopyContext>(); }

TEST(TyrFormalismPlanningRepository, SharedInterningPreservesBindingObjectOrder)
{
    auto repository = fp::RepositoryFactory().create();
    auto builder = fp::Builder {};
    auto a_data = ygg::Data<f::Object>(std::string("a"));
    auto b_data = ygg::Data<f::Object>(std::string("b"));
    const auto a = fp::insert(repository, a_data).first;
    const auto b = fp::insert(repository, b_data).first;
    using Predicate = f::Predicate<f::StaticTag>;
    auto predicate_data = ygg::Data<Predicate>(std::string("p"), 2);
    const auto predicate = fp::insert(repository, predicate_data).first;

    auto data = fp::checkout<f::RelationBinding<Predicate>>(builder);
    data->relation = predicate.get_index();
    data->objects.push_back(b.get_index());
    data->objects.push_back(a.get_index());
    const auto [binding, inserted] = fp::insert(repository, *data);
    ASSERT_TRUE(inserted);
    EXPECT_EQ(binding.get_relation(), predicate);
    ASSERT_EQ(binding.get_objects().size(), 2);
    EXPECT_EQ(binding.get_objects()[0], b);
    EXPECT_EQ(binding.get_objects()[1], a);

    const auto [duplicate, duplicate_inserted] = fp::insert(repository, *data);
    EXPECT_FALSE(duplicate_inserted);
    EXPECT_EQ(duplicate, binding);
    std::swap(data->objects[0], data->objects[1]);
    const auto [reversed, reversed_inserted] = fp::insert(repository, *data);
    EXPECT_TRUE(reversed_inserted);
    EXPECT_NE(reversed, binding);
    EXPECT_EQ(reversed.get_objects()[0], a);
    EXPECT_EQ(reversed.get_objects()[1], b);
}
