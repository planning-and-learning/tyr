#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/term_data.hpp"
#include "tyr/formalism/term_view.hpp"

#include <concepts>
#include <gtest/gtest.h>

using TermData = ygg::Data<tyr::formalism::Term>;
using TermView = ygg::View<TermData, tyr::formalism::planning::Repository>;

template<typename Repository>
concept TermContract = std::totally_ordered<TermData> && std::totally_ordered<ygg::View<TermData, Repository>>
                       && requires(TermData& data, const ygg::View<TermData, Repository>& view) {
                              data.variant;
                              data.clear();
                              view.get_variant();
                          };

static_assert(TermContract<tyr::formalism::datalog::Repository>);
static_assert(TermContract<tyr::formalism::planning::Repository>);
static_assert(std::same_as<ygg::View<TermData, tyr::formalism::datalog::Repository>, tyr::formalism::datalog::TermView>);
static_assert(std::same_as<TermView, tyr::formalism::planning::TermView>);

TEST(TyrFormalismTerm, PreservesParameterAlternative)
{
    auto data = TermData(tyr::formalism::ParameterIndex(3));
    auto is_parameter = false;
    data.variant.apply(
        [&](const auto& value)
        {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::same_as<Value, tyr::formalism::ParameterIndex>)
                is_parameter = value == tyr::formalism::ParameterIndex(3);
        });
    EXPECT_TRUE(is_parameter);
}

TEST(TyrFormalismTerm, CanonicalContextFollowsReferencedObjectsAndRootValues)
{
    auto factory = tyr::formalism::planning::RepositoryFactory {};
    auto parent = factory.create();
    auto object_data = ygg::Data<tyr::formalism::Object>(std::string("parent"));
    const auto object = tyr::formalism::planning::insert(parent, object_data).first;
    auto child = factory.create(&parent);
    object_data.name = "child";
    const auto child_object = tyr::formalism::planning::insert(child, object_data).first;

    const auto inherited = ygg::make_view(object.get_index(), child);
    EXPECT_EQ(inherited, object);
    EXPECT_EQ(&inherited.get_context(), &parent);
    const auto parent_term = ygg::make_view(TermData(object.get_index()), child);
    const auto child_term = ygg::make_view(TermData(child_object.get_index()), child);
    const auto parameter = ygg::make_view(TermData(tyr::formalism::ParameterIndex(0)), child);
    EXPECT_EQ(&parent_term.get_context(), &parent);
    EXPECT_EQ(&child_term.get_context(), &child);
    EXPECT_EQ(&parameter.get_context(), &parent);
    EXPECT_EQ(child_term.get_data(), TermData(child_object.get_index()));
}
