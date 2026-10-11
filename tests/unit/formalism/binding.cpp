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
    ygg::ViewConcept<ygg::Index<tyr::formalism::RelationBinding<Relation>>, Repository>
    && tyr::formalism::RelationBindingConcept<tyr::formalism::RelationBinding<Relation>>
    && std::totally_ordered<ygg::Index<tyr::formalism::RelationBinding<Relation>>> && std::totally_ordered<ygg::Data<tyr::formalism::RelationBinding<Relation>>>
    && std::totally_ordered<ygg::View<ygg::Index<tyr::formalism::RelationBinding<Relation>>, Repository>>
    && std::same_as<ygg::View<ygg::Index<tyr::formalism::RelationBinding<Relation>>, Repository>, typename BindingPublicView<Relation, Repository>::type>
    && requires(ygg::Index<tyr::formalism::RelationBinding<Relation>>& index,
                ygg::Data<tyr::formalism::RelationBinding<Relation>>& data,
                const ygg::View<ygg::Index<tyr::formalism::RelationBinding<Relation>>, Repository>& view) {
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

static_assert(binding_contracts<tyr::formalism::datalog::Repository>(tyr::formalism::datalog::RelationRepositoryTypes {}));
static_assert(binding_contracts<tyr::formalism::planning::Repository>(tyr::formalism::planning::RelationRepositoryTypes {}));

template<typename Range>
concept ForwardBindingRange = requires { typename tyr::formalism::RelationBindingsForwardRange<tyr::formalism::Predicate<tyr::formalism::StaticTag>, Range>; };

template<typename Range>
concept RandomAccessBindingRange =
    requires { typename tyr::formalism::RelationBindingsRandomAccessRange<tyr::formalism::Predicate<tyr::formalism::StaticTag>, Range>; };

static_assert(ForwardBindingRange<std::vector<ygg::Index<tyr::formalism::Row>>>);
static_assert(RandomAccessBindingRange<std::vector<ygg::Index<tyr::formalism::Row>>>);
static_assert(ForwardBindingRange<std::forward_list<ygg::Index<tyr::formalism::Row>>>);
static_assert(!RandomAccessBindingRange<std::forward_list<ygg::Index<tyr::formalism::Row>>>);
static_assert(!ForwardBindingRange<std::vector<int>>);
static_assert(!RandomAccessBindingRange<std::vector<int>>);
static_assert(!ForwardBindingRange<int>);
static_assert(!RandomAccessBindingRange<int>);

using Binding = tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::StaticTag>>;
static_assert(std::same_as<Binding, ygg::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::StaticTag>, tyr::formalism::ObjectTag>>);

#if defined(TYR_RELATION_STORAGE_WORD)
static_assert(std::same_as<typename ygg::formalism::RelationRepositoryTraits<tyr::formalism::ObjectTag>::storage_type, ygg::formalism::BlockArraySetStorage>);
#else
static_assert(
    std::same_as<typename ygg::formalism::RelationRepositoryTraits<tyr::formalism::ObjectTag>::storage_type, ygg::formalism::BitPackedArraySetStorage>);
#endif

template<typename Factory, typename Builder, typename Context>
void check_binding_copy()
{
    auto source = Factory().create();
    auto destination = Factory().create();

    const auto intern = []<typename T>(auto& repository, ygg::Data<T> data) { return ygg::formalism::insert(repository, data).first; };

    const auto source_a = intern(source, ygg::Data<tyr::formalism::Object>(std::string("a")));
    (void) intern(source, ygg::Data<tyr::formalism::Object>(std::string("b")));
    (void) intern(destination, ygg::Data<tyr::formalism::Object>(std::string("b")));
    const auto destination_a = intern(destination, ygg::Data<tyr::formalism::Object>(std::string("a")));

    using Predicate = tyr::formalism::Predicate<tyr::formalism::FluentTag>;
    const auto source_predicate = intern(source, ygg::Data<Predicate>(std::string("p"), 1));
    (void) intern(source, ygg::Data<Predicate>(std::string("q"), 1));
    (void) intern(destination, ygg::Data<Predicate>(std::string("q"), 1));
    const auto destination_predicate = intern(destination, ygg::Data<Predicate>(std::string("p"), 1));

    auto predicate_binding_data = ygg::Data<tyr::formalism::RelationBinding<Predicate>> {};
    predicate_binding_data.relation = source_predicate.get_index();
    predicate_binding_data.objects.push_back(source_a.get_index());
    const auto source_predicate_binding = intern(source, std::move(predicate_binding_data));

    using Function = tyr::formalism::Function<tyr::formalism::FluentTag>;
    const auto source_function = intern(source, ygg::Data<Function>(std::string("f"), 1));
    (void) intern(source, ygg::Data<Function>(std::string("g"), 1));
    (void) intern(destination, ygg::Data<Function>(std::string("g"), 1));
    const auto destination_function = intern(destination, ygg::Data<Function>(std::string("f"), 1));

    auto function_binding_data = ygg::Data<tyr::formalism::RelationBinding<Function>> {};
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

TEST(TyrFormalismDatalogCopy, RemapsBindingRelationsAndObjects)
{
    check_binding_copy<tyr::formalism::datalog::RepositoryFactory, tyr::formalism::datalog::Builder, tyr::formalism::datalog::CopyContext>();
}

TEST(TyrFormalismPlanningCopy, RemapsBindingRelationsAndObjects)
{
    check_binding_copy<tyr::formalism::planning::RepositoryFactory, tyr::formalism::planning::Builder, tyr::formalism::planning::CopyContext>();
}

TEST(TyrFormalismPlanningRepository, SharedInterningPreservesBindingObjectOrder)
{
    auto repository = tyr::formalism::planning::RepositoryFactory().create();
    auto builder = tyr::formalism::planning::Builder {};
    auto a_data = ygg::Data<tyr::formalism::Object>(std::string("a"));
    auto b_data = ygg::Data<tyr::formalism::Object>(std::string("b"));
    const auto a = tyr::formalism::planning::insert(repository, a_data).first;
    const auto b = tyr::formalism::planning::insert(repository, b_data).first;
    using Predicate = tyr::formalism::Predicate<tyr::formalism::StaticTag>;
    auto predicate_data = ygg::Data<Predicate>(std::string("p"), 2);
    const auto predicate = tyr::formalism::planning::insert(repository, predicate_data).first;

    auto data = tyr::formalism::planning::checkout<tyr::formalism::RelationBinding<Predicate>>(builder);
    data->relation = predicate.get_index();
    data->objects.push_back(b.get_index());
    data->objects.push_back(a.get_index());
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
