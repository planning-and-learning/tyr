#include "tyr/planning/ground/match_tree/nodes/generator_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/generator_index.hpp"
#include "tyr/planning/ground/match_tree/nodes/generator_view.hpp"
#include "tyr/planning/ground/match_tree/repository.hpp"

#include <concepts>
#include <gtest/gtest.h>

namespace fp = tyr::formalism::planning;
namespace mt = tyr::planning::match_tree;

template<typename Tag>
concept ElementGeneratorContract =
    ygg::ViewConcept<ygg::Index<mt::ElementGeneratorNode<Tag>>, mt::Repository<Tag>>
    && std::constructible_from<ygg::Index<mt::ElementGeneratorNode<Tag>>, ygg::uint_t> && std::totally_ordered<ygg::Index<mt::ElementGeneratorNode<Tag>>>
    && std::totally_ordered<ygg::Data<mt::ElementGeneratorNode<Tag>>>
    && std::totally_ordered<ygg::View<ygg::Index<mt::ElementGeneratorNode<Tag>>, mt::Repository<Tag>>>
    && requires(ygg::Data<mt::ElementGeneratorNode<Tag>>& data, const ygg::View<ygg::Index<mt::ElementGeneratorNode<Tag>>, mt::Repository<Tag>>& view) {
           data.index;
           data.elements;
           data.clear();
           view.get_elements();
       };

using MatchTreeTags = ygg::TypeList<fp::Action<::tyr::GroundTag>, fp::Axiom<::tyr::GroundTag>>;
static_assert([]<typename... Tags>(ygg::TypeList<Tags...>) { return (ElementGeneratorContract<Tags> && ...); }(MatchTreeTags {}));

using Action = fp::Action<::tyr::GroundTag>;
using Axiom = fp::Axiom<::tyr::GroundTag>;
using Generator = mt::ElementGeneratorNode<Action>;
static_assert(ygg::formalism::SupportsSymbol<mt::Repository<Action>, Generator>);
static_assert(!ygg::formalism::SupportsSymbol<mt::Repository<Action>, mt::ElementGeneratorNode<Axiom>>);
static_assert(!ygg::formalism::SupportsSymbol<mt::Repository<Action>, Action>);

template<typename T>
concept MatchTreeCanIntern = requires(mt::Repository<Action>& repository, ygg::Data<T>& data) {
    repository.get_or_create(data);
    mt::get_or_create(repository, data);
};
static_assert(MatchTreeCanIntern<Generator>);
static_assert(!MatchTreeCanIntern<mt::ElementGeneratorNode<Axiom>>);

TEST(TyrMatchTreeRepository, SharedInterningKeepsTheMatchTreeContext)
{
    auto formalism_repository = fp::RepositoryFactory().create();
    auto repository = mt::Repository<Action>(0, formalism_repository);
    auto builder = mt::GroundActionBuilder {};
    auto data = mt::checkout<Generator>(builder);
    const auto [view, inserted] = mt::get_or_create(repository, *data);
    EXPECT_TRUE(inserted);
    EXPECT_EQ(&view.get_context(), &repository);
    EXPECT_TRUE(view.get_elements().empty());
    EXPECT_EQ(data->index, view.get_handle());

    data->index = ygg::Index<Generator>::max();
    const auto [duplicate, duplicate_inserted] = mt::get_or_create(repository, *data);
    EXPECT_FALSE(duplicate_inserted);
    EXPECT_EQ(duplicate, view);
    EXPECT_EQ(data->index, view.get_handle());
}
