#include "tyr/formalism/binding_data.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/formalism/binding_index.hpp"
#include "tyr/formalism/binding_view.hpp"
#include "tyr/formalism/datalog/copy.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/planning/copy.hpp"
#include "tyr/formalism/planning/interning.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <forward_list>
#include <gtest/gtest.h>
#include <string>
#include <utility>
#include <vector>

template<typename Relation, typename Repository>
struct BindingPublicView;

template<tyr::formalism::FactKind T>
struct BindingPublicView<tyr::formalism::Predicate<T>, tyr::formalism::datalog::Repository>
{
    using type = tyr::formalism::datalog::PredicateBindingView<T>;
};

template<tyr::formalism::FactKind T>
struct BindingPublicView<tyr::formalism::Function<T>, tyr::formalism::datalog::Repository>
{
    using type = tyr::formalism::datalog::FunctionBindingView<T>;
};

template<tyr::formalism::RelationKind R>
struct BindingPublicView<tyr::formalism::datalog::Rule<::tyr::LiftedTag, R>, tyr::formalism::datalog::Repository>
{
    using type = tyr::formalism::datalog::RuleBindingView<R>;
};

template<tyr::formalism::FactKind T>
struct BindingPublicView<tyr::formalism::Predicate<T>, tyr::formalism::planning::Repository>
{
    using type = tyr::formalism::planning::PredicateBindingView<T>;
};

template<tyr::formalism::FactKind T>
struct BindingPublicView<tyr::formalism::Function<T>, tyr::formalism::planning::Repository>
{
    using type = tyr::formalism::planning::FunctionBindingView<T>;
};

template<>
struct BindingPublicView<tyr::formalism::planning::Action<::tyr::LiftedTag>, tyr::formalism::planning::Repository>
{
    using type = tyr::formalism::planning::ActionBindingView;
};

template<>
struct BindingPublicView<tyr::formalism::planning::Axiom<::tyr::LiftedTag>, tyr::formalism::planning::Repository>
{
    using type = tyr::formalism::planning::AxiomBindingView;
};

template<typename Relation, typename Repository>
concept BindingContract =
    requires(const Repository& repository, ygg::formalism::RelationBindingIndex<Relation, ::tyr::formalism::ObjectTag> index) { repository.view(index); }
    && tyr::formalism::RelationBindingConcept<tyr::formalism::RelationBinding<Relation>>
    && std::totally_ordered<ygg::formalism::RelationBindingIndex<Relation, ::tyr::formalism::ObjectTag>> && std::totally_ordered<urd::data<tyr::formalism::RelationBinding<Relation>>>
    && std::totally_ordered<::ygg::formalism::RelationBindingView<ygg::formalism::RelationBindingIndex<Relation, ::tyr::formalism::ObjectTag>, Repository>>
    && std::same_as<::ygg::formalism::RelationBindingView<ygg::formalism::RelationBindingIndex<Relation, ::tyr::formalism::ObjectTag>, Repository>, typename BindingPublicView<Relation, Repository>::type>
    && requires(ygg::formalism::RelationBindingIndex<Relation, ::tyr::formalism::ObjectTag>& index,
                urd::data<tyr::formalism::RelationBinding<Relation>>& data,
                const ::ygg::formalism::RelationBindingView<ygg::formalism::RelationBindingIndex<Relation, ::tyr::formalism::ObjectTag>, Repository>& view) {
           index.relation;
           index.row;
           data.relation;
           data.objects;
           view.index();
           view.get_relation();
           view.get_objects();
           view.get_key();
       };

template<typename Repository, typename... Relations>
consteval bool binding_contracts(boost::mp11::mp_list<Relations...>)
{ return (BindingContract<Relations, Repository> && ...); }

static_assert(binding_contracts<tyr::formalism::datalog::Repository>(tyr::formalism::datalog::RelationRepositoryTypes {}));
static_assert(binding_contracts<tyr::formalism::planning::Repository>(tyr::formalism::planning::RelationRepositoryTypes {}));

using Binding = tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::StaticTag>>;
static_assert(std::same_as<Binding, ygg::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::StaticTag>, tyr::formalism::ObjectTag>>);

#if defined(TYR_RELATION_STORAGE_WORD)
static_assert(std::same_as<typename ygg::formalism::RelationRepositoryTraits<tyr::formalism::ObjectTag>::storage_type, ygg::formalism::RowPoolStorage>);
#else
static_assert(
    std::same_as<typename ygg::formalism::RelationRepositoryTraits<tyr::formalism::ObjectTag>::storage_type, ygg::formalism::BitRowPoolStorage>);
#endif

namespace
{
template<typename Factory, typename Builder, typename Context>
void check_binding_copy()
{
    auto source = Factory().create();
    auto destination = Factory().create();

    const auto intern = []<typename T>(auto& repository, urd::data<T> data) { return ygg::formalism::insert(repository, data).first; };

    const auto source_a = intern(source, urd::data<tyr::formalism::Object>(std::string("a")));
    (void) intern(source, urd::data<tyr::formalism::Object>(std::string("b")));
    (void) intern(destination, urd::data<tyr::formalism::Object>(std::string("b")));
    const auto destination_a = intern(destination, urd::data<tyr::formalism::Object>(std::string("a")));

    using Predicate = tyr::formalism::Predicate<tyr::formalism::FluentTag>;
    const auto source_predicate = intern(source, urd::data<Predicate>(std::string("p"), 1));
    (void) intern(source, urd::data<Predicate>(std::string("q"), 1));
    (void) intern(destination, urd::data<Predicate>(std::string("q"), 1));
    const auto destination_predicate = intern(destination, urd::data<Predicate>(std::string("p"), 1));

    auto predicate_binding_data = urd::data<tyr::formalism::RelationBinding<Predicate>> {};
    predicate_binding_data.relation = source_predicate.index();
    predicate_binding_data.objects.push_back(source_a.index());
    const auto source_predicate_binding = intern(source, std::move(predicate_binding_data));

    using Function = tyr::formalism::Function<tyr::formalism::FluentTag>;
    const auto source_function = intern(source, urd::data<Function>(std::string("f"), 1));
    (void) intern(source, urd::data<Function>(std::string("g"), 1));
    (void) intern(destination, urd::data<Function>(std::string("g"), 1));
    const auto destination_function = intern(destination, urd::data<Function>(std::string("f"), 1));

    auto function_binding_data = urd::data<tyr::formalism::RelationBinding<Function>> {};
    function_binding_data.relation = source_function.index();
    function_binding_data.objects.push_back(source_a.index());
    const auto source_function_binding = intern(source, std::move(function_binding_data));

    ASSERT_NE(source_a.index(), destination_a.index());
    ASSERT_NE(source_predicate.index(), destination_predicate.index());
    ASSERT_NE(source_function.index(), destination_function.index());

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
}  // namespace

TEST(TyrFormalismDatalogCopy, RemapsBindingRelationsAndObjects)
{ check_binding_copy<tyr::formalism::datalog::RepositoryFactory, tyr::formalism::datalog::Builder, tyr::formalism::datalog::CopyContext>(); }

TEST(TyrFormalismPlanningCopy, RemapsBindingRelationsAndObjects)
{ check_binding_copy<tyr::formalism::planning::RepositoryFactory, tyr::formalism::planning::Builder, tyr::formalism::planning::CopyContext>(); }

TEST(TyrFormalismPlanningRepository, SharedInterningPreservesBindingObjectOrder)
{
    auto repository = tyr::formalism::planning::RepositoryFactory().create();
    auto builder = tyr::formalism::planning::Builder {};
    auto a_data = urd::data<tyr::formalism::Object>(std::string("a"));
    auto b_data = urd::data<tyr::formalism::Object>(std::string("b"));
    const auto a = tyr::formalism::planning::insert(repository, a_data).first;
    const auto b = tyr::formalism::planning::insert(repository, b_data).first;
    using Predicate = tyr::formalism::Predicate<tyr::formalism::StaticTag>;
    auto predicate_data = urd::data<Predicate>(std::string("p"), 2);
    const auto predicate = tyr::formalism::planning::insert(repository, predicate_data).first;

    auto data = tyr::formalism::planning::checkout<tyr::formalism::RelationBinding<Predicate>>(builder);
    data->relation = predicate.index();
    data->objects.push_back(b.index());
    data->objects.push_back(a.index());
    const auto [binding, inserted] = tyr::formalism::planning::insert(repository, *data);
    ASSERT_TRUE(inserted);
    EXPECT_EQ(binding.get_relation(), predicate);
    ASSERT_EQ(binding.get_objects().size(), 2);
    EXPECT_EQ(binding.get_objects()[0], b);
    EXPECT_EQ(binding.get_objects()[1], a);

    const auto [duplicate, duplicate_inserted] = tyr::formalism::planning::insert(repository, *data);
    EXPECT_FALSE(duplicate_inserted);
    EXPECT_EQ(duplicate, binding);
    std::swap(data->objects[0], data->objects[1]);
    const auto [reversed, reversed_inserted] = tyr::formalism::planning::insert(repository, *data);
    EXPECT_TRUE(reversed_inserted);
    EXPECT_NE(reversed, binding);
    EXPECT_EQ(reversed.get_objects()[0], a);
    EXPECT_EQ(reversed.get_objects()[1], b);
}
