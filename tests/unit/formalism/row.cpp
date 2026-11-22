#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/repository.hpp"

#include <concepts>
#include <forward_list>
#include <gtest/gtest.h>
#include <string>
#include <urd/elements/copy.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/formalism/copy.hpp>
#include <yggdrasil/formalism/row_data.hpp>
#include <yggdrasil/formalism/row_view.hpp>

template<typename Relation, typename Repository>
struct RowPublicView;

template<>
struct RowPublicView<tyr::formalism::Rule<::tyr::LiftedTag>, tyr::formalism::Repository>
{
    using type = tyr::formalism::RuleRowView;
};

template<tyr::formalism::FactKind T>
struct RowPublicView<tyr::formalism::Predicate<T>, tyr::formalism::Repository>
{
    using type = tyr::formalism::PredicateRowView<T>;
};

template<tyr::formalism::FactKind T>
struct RowPublicView<tyr::formalism::Function<T>, tyr::formalism::Repository>
{
    using type = tyr::formalism::FunctionRowView<T>;
};

template<>
struct RowPublicView<tyr::formalism::Action<::tyr::LiftedTag>, tyr::formalism::Repository>
{
    using type = tyr::formalism::ActionRowView;
};

template<>
struct RowPublicView<tyr::formalism::Axiom<::tyr::LiftedTag>, tyr::formalism::Repository>
{
    using type = tyr::formalism::AxiomRowView;
};

template<typename Relation, typename Repository>
concept RowContract =
    requires(const Repository& repository, urd::key<Relation, ygg::Row<Relation>> index) { repository.view(index); }
    && std::totally_ordered<urd::key<Relation, ygg::Row<Relation>>> && std::totally_ordered<urd::data<ygg::Row<Relation>>>
    && std::totally_ordered<ygg::RowView<Relation, Repository>>
    && std::same_as<ygg::RowView<Relation, Repository>, typename RowPublicView<Relation, Repository>::type>
    && requires(urd::key<Relation, ygg::Row<Relation>>& index, urd::data<ygg::Row<Relation>>& data, const ygg::RowView<Relation, Repository>& view) {
           index.template get<Relation>();
           index.template get<ygg::Row<Relation>>();
           data.relation;
           data.values;
           view.get_values();
           view.get_relation();
           view.template get<urd::index<tyr::formalism::Object>>();
           view.get(0);
           view.get_relation().index();
       };

template<typename Repository, typename... Relations>
consteval bool row_contracts(boost::mp11::mp_list<Relations...>)
{ return (RowContract<Relations, Repository> && ...); }

static_assert(row_contracts<tyr::formalism::Repository>(tyr::formalism::RelationRepositoryTypes {}));

using Binding = ygg::Row<tyr::formalism::Predicate<tyr::formalism::StaticTag>>;
static_assert(std::same_as<Binding, ygg::Row<tyr::formalism::Predicate<tyr::formalism::StaticTag>>>);

// A row copied into another repository: its relation copied (at another index there), its values as they are (its
// objects name the same objects in both: they are interned alike, as the layers of one task share theirs).
TEST(TyrFormalismPlanningCopy, CopiesBindingRelationsAndKeepsTheirValues)
{
    auto source = tyr::formalism::Repository();
    auto destination = tyr::formalism::Repository();
    const auto intern = []<typename T>(auto& repository, urd::data<T> data) { return repository.insert(data).first; };

    const auto source_a = intern(source, urd::data<tyr::formalism::Object>(std::string("a")));
    const auto destination_a = intern(destination, urd::data<tyr::formalism::Object>(std::string("a")));
    ASSERT_EQ(source_a.index(), destination_a.index());

    using Predicate = tyr::formalism::Predicate<tyr::formalism::FluentTag>;
    const auto source_predicate = intern(source, urd::data<Predicate>(std::string("p"), 1));
    (void) intern(source, urd::data<Predicate>(std::string("q"), 1));
    (void) intern(destination, urd::data<Predicate>(std::string("q"), 1));
    const auto destination_predicate = intern(destination, urd::data<Predicate>(std::string("p"), 1));
    ASSERT_NE(source_predicate.index(), destination_predicate.index());

    auto binding_data = urd::data<ygg::Row<Predicate>> {};
    binding_data.relation = source_predicate.index();
    binding_data.values.push_back(source_a.index().value);
    const auto source_binding = intern(source, std::move(binding_data));

    auto builder = tyr::formalism::Builder {};
    const auto copied = destination.view(urd::copy(source_binding, destination, builder));
    EXPECT_EQ(copied.get_relation(), destination_predicate);
    ASSERT_EQ(copied.template resolve<tyr::formalism::Object>().size(), 1);
    EXPECT_EQ(copied.template resolve<tyr::formalism::Object>()[0], destination_a);
    EXPECT_EQ(urd::copy(source_binding, destination, builder), copied.index());  // interned once
    EXPECT_EQ(urd::copy(copied, destination, builder), copied.index());          // its own
}

TEST(TyrFormalismPlanningRepository, SharedInterningPreservesBindingObjectOrder)
{
    auto repository = tyr::formalism::Repository();
    auto builder = tyr::formalism::Builder {};
    auto a_data = urd::data<tyr::formalism::Object>(std::string("a"));
    auto b_data = urd::data<tyr::formalism::Object>(std::string("b"));
    const auto a = repository.insert(a_data).first;
    const auto b = repository.insert(b_data).first;
    using Predicate = tyr::formalism::Predicate<tyr::formalism::StaticTag>;
    auto predicate_data = urd::data<Predicate>(std::string("p"), 2);
    const auto predicate = repository.insert(predicate_data).first;

    auto data = tyr::formalism::checkout<ygg::Row<Predicate>>(builder);
    urd::store(data->relation, predicate);
    data->values.push_back(b.index().value);
    data->values.push_back(a.index().value);
    const auto [binding, inserted] = repository.insert(*data, builder);
    ASSERT_TRUE(inserted);
    EXPECT_EQ(binding.get_relation(), predicate);
    ASSERT_EQ(binding.template resolve<tyr::formalism::Object>().size(), 2);
    EXPECT_EQ(binding.template resolve<tyr::formalism::Object>()[0], b);
    EXPECT_EQ(binding.template resolve<tyr::formalism::Object>()[1], a);
    EXPECT_EQ(binding.type(0), ygg::type_id<urd::index<tyr::formalism::Object>>);
    EXPECT_THROW((void) binding.template get<double>(0), std::invalid_argument);  // an object column

    const auto [duplicate, duplicate_inserted] = repository.insert(*data, builder);
    EXPECT_FALSE(duplicate_inserted);
    EXPECT_EQ(duplicate, binding);
    std::swap(data->values[0], data->values[1]);
    const auto [reversed, reversed_inserted] = repository.insert(*data, builder);
    EXPECT_TRUE(reversed_inserted);
    EXPECT_NE(reversed, binding);
    EXPECT_EQ(reversed.template resolve<tyr::formalism::Object>()[0], a);
    EXPECT_EQ(reversed.template resolve<tyr::formalism::Object>()[1], b);
}
