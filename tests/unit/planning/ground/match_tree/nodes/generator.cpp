#include "tyr/planning/ground/match_tree/declarations.hpp"
#include "tyr/planning/ground/match_tree/nodes/generator_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/node_view.hpp"
#include "tyr/planning/ground/match_tree/repository.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <utility>

template<typename Tag>
concept ElementGeneratorContract =
    urd::resolves<tyr::planning::match_tree::Repository<Tag>, tyr::planning::match_tree::ElementGeneratorNode<Tag>>
    && std::constructible_from<urd::index<tyr::planning::match_tree::ElementGeneratorNode<Tag>>, ygg::uint_t>
    && std::totally_ordered<urd::index<tyr::planning::match_tree::ElementGeneratorNode<Tag>>>
    && std::totally_ordered<urd::data<tyr::planning::match_tree::ElementGeneratorNode<Tag>>>
    && std::totally_ordered<urd::index_view<tyr::planning::match_tree::ElementGeneratorNode<Tag>, tyr::planning::match_tree::Repository<Tag>>>
    && requires(urd::data<tyr::planning::match_tree::ElementGeneratorNode<Tag>>& data,
                const urd::index_view<tyr::planning::match_tree::ElementGeneratorNode<Tag>, tyr::planning::match_tree::Repository<Tag>>& view) {
           data.index;
           data.elements;
           view.get_elements();
       };

using MatchTreeTags = boost::mp11::mp_list<tyr::formalism::planning::Action<::tyr::GroundTag>, tyr::formalism::planning::Axiom<::tyr::GroundTag>>;
static_assert([]<typename... Tags>(boost::mp11::mp_list<Tags...>) { return (ElementGeneratorContract<Tags> && ...); }(MatchTreeTags {}));

using Action = tyr::formalism::planning::Action<::tyr::GroundTag>;
using Axiom = tyr::formalism::planning::Axiom<::tyr::GroundTag>;
using Generator = tyr::planning::match_tree::ElementGeneratorNode<Action>;
static_assert(ygg::formalism::SupportsSymbol<tyr::planning::match_tree::Repository<Action>, Generator>);
static_assert(!ygg::formalism::SupportsSymbol<tyr::planning::match_tree::Repository<Action>, tyr::planning::match_tree::ElementGeneratorNode<Axiom>>);
static_assert(!ygg::formalism::SupportsSymbol<tyr::planning::match_tree::Repository<Action>, Action>);

template<typename T>
concept MatchTreeCanIntern = requires(tyr::planning::match_tree::Repository<Action>& repository, urd::data<T>& data) {
    repository.insert(data);
    tyr::planning::match_tree::insert(repository, data);
};
static_assert(MatchTreeCanIntern<Generator>);
static_assert(!MatchTreeCanIntern<tyr::planning::match_tree::ElementGeneratorNode<Axiom>>);

static_assert(noexcept(std::declval<const tyr::planning::match_tree::Repository<Action>&>()[std::declval<urd::index<Generator>>()]));
static_assert(noexcept(std::declval<const tyr::planning::match_tree::Repository<Action>&>().view(std::declval<urd::index<Generator>>())));

TEST(TyrMatchTreeRepository, SharedInterningKeepsTheMatchTreeContext)
{
    auto formalism_repository = tyr::formalism::planning::RepositoryFactory().create();
    auto repository = tyr::planning::match_tree::Repository<Action>(0, formalism_repository);
    auto builder = tyr::planning::match_tree::GroundActionBuilder {};
    auto data = tyr::planning::match_tree::checkout<Generator>(builder);
    const auto [view, inserted] = tyr::planning::match_tree::insert(repository, *data);
    EXPECT_TRUE(inserted);
    EXPECT_EQ(&view.context(), &repository);
    EXPECT_TRUE(view.get_elements().empty());
    EXPECT_EQ(data->index, view.index());

    data->index = urd::index<Generator>();
    const auto [duplicate, duplicate_inserted] = tyr::planning::match_tree::insert(repository, *data);
    EXPECT_FALSE(duplicate_inserted);
    EXPECT_EQ(duplicate, view);
    EXPECT_EQ(data->index, view.index());
    EXPECT_EQ(&duplicate.context(), &repository);
    const auto found = repository.find(*data);
    ASSERT_TRUE(found);
    EXPECT_EQ(&found.value().context(), &repository);
    EXPECT_EQ(&repository.front_local<Generator>(), &view.data());
    EXPECT_EQ(repository.size<Generator>(), 1);
}
