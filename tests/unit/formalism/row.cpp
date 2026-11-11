#include "tyr/formalism/copy.hpp"
#include "tyr/formalism/datalog/copy.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/interning.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <forward_list>
#include <gtest/gtest.h>
#include <string>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/formalism/row_data.hpp>
#include <yggdrasil/formalism/row_view.hpp>

template<typename Relation, typename Repository>
struct RowPublicView;

template<tyr::formalism::FactKind T>
struct RowPublicView<tyr::formalism::Predicate<T>, tyr::formalism::datalog::Repository>
{
    using type = tyr::formalism::datalog::PredicateRowView<T>;
};

template<tyr::formalism::FactKind T>
struct RowPublicView<tyr::formalism::Function<T>, tyr::formalism::datalog::Repository>
{
    using type = tyr::formalism::datalog::FunctionRowView<T>;
};

template<tyr::formalism::RelationKind R>
struct RowPublicView<tyr::formalism::datalog::Rule<::tyr::LiftedTag, R>, tyr::formalism::datalog::Repository>
{
    using type = tyr::formalism::datalog::RuleRowView<R>;
};

template<tyr::formalism::FactKind T>
struct RowPublicView<tyr::formalism::Predicate<T>, tyr::formalism::planning::Repository>
{
    using type = tyr::formalism::planning::PredicateRowView<T>;
};

template<tyr::formalism::FactKind T>
struct RowPublicView<tyr::formalism::Function<T>, tyr::formalism::planning::Repository>
{
    using type = tyr::formalism::planning::FunctionRowView<T>;
};

template<>
struct RowPublicView<tyr::formalism::planning::Action<::tyr::LiftedTag>, tyr::formalism::planning::Repository>
{
    using type = tyr::formalism::planning::ActionRowView;
};

template<>
struct RowPublicView<tyr::formalism::planning::Axiom<::tyr::LiftedTag>, tyr::formalism::planning::Repository>
{
    using type = tyr::formalism::planning::AxiomRowView;
};

template<typename Relation, typename Repository>
concept RowContract =
    requires(const Repository& repository, ygg::RowIndex<Relation> index) { repository.view(index); } && std::totally_ordered<ygg::RowIndex<Relation>>
    && std::totally_ordered<urd::data<ygg::Row<Relation>>> && std::totally_ordered<ygg::RowView<Relation, Repository>>
    && std::same_as<ygg::RowView<Relation, Repository>, typename RowPublicView<Relation, Repository>::type>
    && requires(ygg::RowIndex<Relation>& index, urd::data<ygg::Row<Relation>>& data, const ygg::RowView<Relation, Repository>& view) {
           index.relation;
           index.row;
           data.relation;
           data.values;
           view.data();
           view.get_relation();
           view.template get<urd::index<tyr::formalism::Object>>();
           view.get(0);
           view.get_key();
       };

template<typename Repository, typename... Relations>
consteval bool row_contracts(boost::mp11::mp_list<Relations...>)
{ return (RowContract<Relations, Repository> && ...); }

static_assert(row_contracts<tyr::formalism::datalog::Repository>(tyr::formalism::datalog::RelationRepositoryTypes {}));
static_assert(row_contracts<tyr::formalism::planning::Repository>(tyr::formalism::planning::RelationRepositoryTypes {}));

using Binding = ygg::Row<tyr::formalism::Predicate<tyr::formalism::StaticTag>>;
static_assert(std::same_as<Binding, ygg::Row<tyr::formalism::Predicate<tyr::formalism::StaticTag>>>);

#if defined(TYR_RELATION_STORAGE_WORD)
static_assert(std::same_as<typename ygg::formalism::RelationRepositoryTraits<tyr::formalism::ObjectTag>::width, urd::word>);
#else
static_assert(std::same_as<typename ygg::formalism::RelationRepositoryTraits<tyr::formalism::ObjectTag>::width, urd::bit>);
#endif

namespace
{
template<typename Repository, typename Builder, typename Context>
void check_binding_copy()
{
    auto source = Repository();
    auto destination = Repository();

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

    auto predicate_binding_data = urd::data<ygg::Row<Predicate>> {};
    predicate_binding_data.relation = source_predicate.index();
    predicate_binding_data.values.push_back(source_a.index().value);
    const auto source_predicate_binding = intern(source, std::move(predicate_binding_data));

    using Function = tyr::formalism::Function<tyr::formalism::FluentTag>;
    const auto source_function = intern(source, urd::data<Function>(std::string("f"), 1));
    (void) intern(source, urd::data<Function>(std::string("g"), 1));
    (void) intern(destination, urd::data<Function>(std::string("g"), 1));
    const auto destination_function = intern(destination, urd::data<Function>(std::string("f"), 1));

    auto function_binding_data = urd::data<ygg::Row<Function>> {};
    function_binding_data.relation = source_function.index();
    function_binding_data.values.push_back(source_a.index().value);
    const auto source_function_binding = intern(source, std::move(function_binding_data));

    ASSERT_NE(source_a.index(), destination_a.index());
    ASSERT_NE(source_predicate.index(), destination_predicate.index());
    ASSERT_NE(source_function.index(), destination_function.index());

    auto builder = Builder {};
    auto context = Context { builder, destination };
    const auto merged_predicate_binding = copy(source_predicate_binding, context).first;
    const auto merged_function_binding = copy(source_function_binding, context).first;

    EXPECT_EQ(merged_predicate_binding.get_relation(), destination_predicate);
    ASSERT_EQ(urd::resolve(merged_predicate_binding.template get<urd::index<tyr::formalism::Object>>(), merged_predicate_binding.context()).size(), 1);
    EXPECT_EQ(urd::resolve(merged_predicate_binding.template get<urd::index<tyr::formalism::Object>>(), merged_predicate_binding.context())[0], destination_a);
    EXPECT_EQ(merged_function_binding.get_relation(), destination_function);
    ASSERT_EQ(urd::resolve(merged_function_binding.template get<urd::index<tyr::formalism::Object>>(), merged_function_binding.context()).size(), 1);
    EXPECT_EQ(urd::resolve(merged_function_binding.template get<urd::index<tyr::formalism::Object>>(), merged_function_binding.context())[0], destination_a);
    EXPECT_FALSE(copy(source_predicate_binding, context).second);
    EXPECT_FALSE(copy(source_function_binding, context).second);
}
}  // namespace

TEST(TyrFormalismDatalogCopy, RemapsBindingRelationsAndObjects)
{
    check_binding_copy<tyr::formalism::datalog::Repository,
                       tyr::formalism::datalog::Builder,
                       tyr::formalism::WriteContext<tyr::formalism::datalog::Builder, tyr::formalism::datalog::Repository>>();
}

TEST(TyrFormalismPlanningCopy, RemapsBindingRelationsAndObjects)
{
    check_binding_copy<tyr::formalism::planning::Repository,
                       tyr::formalism::planning::Builder,
                       tyr::formalism::WriteContext<tyr::formalism::planning::Builder, tyr::formalism::planning::Repository>>();
}

TEST(TyrFormalismPlanningRepository, SharedInterningPreservesBindingObjectOrder)
{
    auto repository = tyr::formalism::planning::Repository();
    auto builder = tyr::formalism::planning::Builder {};
    auto a_data = urd::data<tyr::formalism::Object>(std::string("a"));
    auto b_data = urd::data<tyr::formalism::Object>(std::string("b"));
    const auto a = tyr::formalism::planning::insert(repository, a_data).first;
    const auto b = tyr::formalism::planning::insert(repository, b_data).first;
    using Predicate = tyr::formalism::Predicate<tyr::formalism::StaticTag>;
    auto predicate_data = urd::data<Predicate>(std::string("p"), 2);
    const auto predicate = tyr::formalism::planning::insert(repository, predicate_data).first;

    auto data = tyr::formalism::planning::checkout<ygg::Row<Predicate>>(builder);
    data->relation = predicate.index();
    data->values.push_back(b.index().value);
    data->values.push_back(a.index().value);
    const auto [binding, inserted] = tyr::formalism::planning::insert(repository, *data);
    ASSERT_TRUE(inserted);
    EXPECT_EQ(binding.get_relation(), predicate);
    ASSERT_EQ(urd::resolve(binding.template get<urd::index<tyr::formalism::Object>>(), binding.context()).size(), 2);
    EXPECT_EQ(urd::resolve(binding.template get<urd::index<tyr::formalism::Object>>(), binding.context())[0], b);
    EXPECT_EQ(urd::resolve(binding.template get<urd::index<tyr::formalism::Object>>(), binding.context())[1], a);
    EXPECT_EQ(binding.type(0), ygg::type_id<urd::index<tyr::formalism::Object>>);
    EXPECT_THROW((void) binding.template get<double>(0), std::invalid_argument);  // an object column

    const auto [duplicate, duplicate_inserted] = tyr::formalism::planning::insert(repository, *data);
    EXPECT_FALSE(duplicate_inserted);
    EXPECT_EQ(duplicate, binding);
    std::swap(data->values[0], data->values[1]);
    const auto [reversed, reversed_inserted] = tyr::formalism::planning::insert(repository, *data);
    EXPECT_TRUE(reversed_inserted);
    EXPECT_NE(reversed, binding);
    EXPECT_EQ(urd::resolve(reversed.template get<urd::index<tyr::formalism::Object>>(), reversed.context())[0], a);
    EXPECT_EQ(urd::resolve(reversed.template get<urd::index<tyr::formalism::Object>>(), reversed.context())[1], b);
}
