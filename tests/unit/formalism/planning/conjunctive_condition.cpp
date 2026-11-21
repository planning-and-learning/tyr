#include "tyr/formalism/conjunctive_condition_data.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/detail/repository_instantiations.hpp"
#include "tyr/formalism/interning.hpp"
#include "tyr/formalism/parameters_data.hpp"
#include "tyr/formalism/repository.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <gtest/gtest.h>
#include <ranges>
#include <string>
#include <utility>

namespace tyr::tests::planning_conjunctive_condition
{

namespace lifted_tests
{

using ConjunctiveConditionIndex = urd::index<tyr::formalism::ConjunctiveCondition<::tyr::LiftedTag>>;
using ConjunctiveConditionData = urd::data<tyr::formalism::ConjunctiveCondition<::tyr::LiftedTag>>;
using ConjunctiveConditionView = urd::index_view<tyr::formalism::ConjunctiveCondition<::tyr::LiftedTag>, tyr::formalism::Repository>;

static_assert(std::constructible_from<ConjunctiveConditionIndex, ygg::uint_t>);
static_assert(std::totally_ordered<ConjunctiveConditionIndex>);
static_assert(std::totally_ordered<ConjunctiveConditionData>);
static_assert(std::totally_ordered<ConjunctiveConditionView>);
static_assert(std::same_as<ConjunctiveConditionView, tyr::formalism::ConjunctiveConditionView<::tyr::LiftedTag>>);
static_assert(requires(ConjunctiveConditionData& data) {
    data.index;
    data.variables;
    data.static_literals;
    data.fluent_literals;
    data.derived_literals;
    data.numeric_constraints;
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const ConjunctiveConditionView& view) {
    requires std::same_as<std::ranges::range_value_t<decltype(view.template get_atoms<tyr::formalism::StaticTag>(true))>,
                          tyr::formalism::AtomView<::tyr::LiftedTag, tyr::formalism::StaticTag>>;
    requires std::same_as<std::ranges::range_value_t<decltype(view.template get_atoms<tyr::formalism::FluentTag>(true))>,
                          tyr::formalism::AtomView<::tyr::LiftedTag, tyr::formalism::FluentTag>>;
    requires std::same_as<std::ranges::range_value_t<decltype(view.template get_atoms<tyr::formalism::DerivedTag>(true))>,
                          tyr::formalism::AtomView<::tyr::LiftedTag, tyr::formalism::DerivedTag>>;
    view.index();
    view.get_variables();
    view.template get_literals<tyr::formalism::StaticTag>();
    view.template get_literals<tyr::formalism::FluentTag>();
    view.template get_literals<tyr::formalism::DerivedTag>();
    view.get_numeric_constraints();
    view.get_arity();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});
}

namespace ground_tests
{

using GroundConjunctiveConditionIndex = urd::index<tyr::formalism::ConjunctiveCondition<::tyr::GroundTag>>;
using GroundConjunctiveConditionData = urd::data<tyr::formalism::ConjunctiveCondition<::tyr::GroundTag>>;
using GroundConjunctiveConditionView = urd::index_view<tyr::formalism::ConjunctiveCondition<::tyr::GroundTag>, tyr::formalism::Repository>;

static_assert(std::constructible_from<GroundConjunctiveConditionIndex, ygg::uint_t>);
static_assert(std::totally_ordered<GroundConjunctiveConditionIndex>);
static_assert(std::totally_ordered<GroundConjunctiveConditionData>);
static_assert(std::totally_ordered<GroundConjunctiveConditionView>);
static_assert(std::same_as<GroundConjunctiveConditionView, tyr::formalism::ConjunctiveConditionView<::tyr::GroundTag>>);
static_assert(requires(GroundConjunctiveConditionData& data) {
    data.index;
    data.positive_facts;
    data.negative_facts;
    data.static_literals;
    data.derived_literals;
    data.numeric_constraints;
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const GroundConjunctiveConditionView& view) {
    requires std::same_as<std::ranges::range_value_t<decltype(view.template get_atoms<tyr::formalism::StaticTag>(true))>,
                          tyr::formalism::AtomView<::tyr::GroundTag, tyr::formalism::StaticTag>>;
    requires std::same_as<std::ranges::range_value_t<decltype(view.template get_atoms<tyr::formalism::FluentTag>(true))>,
                          tyr::formalism::AtomView<::tyr::GroundTag, tyr::formalism::FluentTag>>;
    requires std::same_as<std::ranges::range_value_t<decltype(view.template get_atoms<tyr::formalism::DerivedTag>(true))>,
                          tyr::formalism::AtomView<::tyr::GroundTag, tyr::formalism::DerivedTag>>;
    view.index();
    view.template get_literals<tyr::formalism::StaticTag>();
    view.template get_literals<tyr::formalism::DerivedTag>();
    view.template get_facts<tyr::formalism::PositiveTag>();
    view.template get_facts<tyr::formalism::NegativeTag>();
    view.get_numeric_constraints();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});
}

namespace
{
template<tyr::TaskKind Kind>
void expect_condition_atom_projection()
{
    auto repository = tyr::formalism::Repository();
    const auto intern = [&](auto data) { return tyr::formalism::insert(repository, data).first; };
    const auto atoms = [&]<tyr::formalism::FactKind F>()
    {
        const auto atom = [&](const char* name)
        {
            const auto predicate = intern(urd::data<tyr::formalism::Predicate<F>>(std::string(name), 0));
            if constexpr (std::same_as<Kind, tyr::GroundTag>)
            {
                auto binding = urd::data<ygg::Row<tyr::formalism::Predicate<F>>> {};
                binding.relation = predicate.index();
                return intern(urd::data<tyr::formalism::Atom<Kind, F>>(intern(std::move(binding)).index()));
            }
            else
                return intern(urd::data<tyr::formalism::Atom<Kind, F>>(intern(urd::data<tyr::formalism::Parameters> {}).index(), predicate.index(), {}));
        };
        const auto positive = atom("positive");
        return std::pair(positive, atom("negative"));
    };
    const auto static_atoms = atoms.template operator()<tyr::formalism::StaticTag>();
    const auto fluent_atoms = atoms.template operator()<tyr::formalism::FluentTag>();
    const auto derived_atoms = atoms.template operator()<tyr::formalism::DerivedTag>();
    const auto literals = [&]<tyr::formalism::FactKind F>(const auto& pair)
    {
        auto result = urd::flat_vector<urd::index<tyr::formalism::Literal<Kind, F>>> {};
        result.push_back(intern(urd::data<tyr::formalism::Literal<Kind, F>>(pair.first.index(), true)).index());
        result.push_back(intern(urd::data<tyr::formalism::Literal<Kind, F>>(pair.second.index(), false)).index());
        return result;
    };
    auto data = urd::data<tyr::formalism::ConjunctiveCondition<Kind>> {};
    data.static_literals = literals.template operator()<tyr::formalism::StaticTag>(static_atoms);
    data.derived_literals = literals.template operator()<tyr::formalism::DerivedTag>(derived_atoms);
    if constexpr (std::same_as<Kind, tyr::GroundTag>)
    {
        auto variable_data = urd::data<tyr::formalism::FDRVariable<tyr::formalism::FluentTag>> {};
        variable_data.atoms.push_back(fluent_atoms.first.index());
        variable_data.atoms.push_back(fluent_atoms.second.index());
        const auto variable = intern(std::move(variable_data));
        data.positive_facts.emplace_back(variable.index(), tyr::formalism::FDRValue(1));
        data.positive_facts.emplace_back(variable.index(), tyr::formalism::FDRValue::none());
        data.negative_facts.emplace_back(variable.index(), tyr::formalism::FDRValue(2));
        data.negative_facts.emplace_back(variable.index(), tyr::formalism::FDRValue::none());
    }
    else
        data.fluent_literals = literals.template operator()<tyr::formalism::FluentTag>(fluent_atoms);
    const auto condition = intern(std::move(data));
    const auto check = [&]<tyr::formalism::FactKind F>(const auto& pair)
    {
        // Both the condition argument and its container-view wrappers have expired
        // before iteration starts; only the repository must remain alive.
        auto positive = repository.view(condition.index()).template get_atoms<F>(true);
        auto negative = repository.view(condition.index()).template get_atoms<F>(false);
        EXPECT_TRUE(std::ranges::equal(positive, std::array { pair.first }));
        EXPECT_TRUE(std::ranges::equal(negative, std::array { pair.second }));
        auto filtered_positive = repository.view(condition.index()).template get_atoms<F>(true, pair.first.get_predicate());
        auto filtered_negative = repository.view(condition.index()).template get_atoms<F>(false, pair.second.get_predicate());
        EXPECT_TRUE(std::ranges::equal(filtered_positive, std::array { pair.first }));
        EXPECT_TRUE(std::ranges::equal(filtered_negative, std::array { pair.second }));
        auto opposite = condition.template get_atoms<F>(true, pair.second.get_predicate());
        EXPECT_TRUE(opposite.begin() == opposite.end());
        auto foreign_repository = tyr::formalism::Repository();
        auto foreign_data = urd::data<tyr::formalism::Predicate<F>>(std::string("positive"), 0);
        const auto foreign = tyr::formalism::insert(foreign_repository, foreign_data).first;
        ASSERT_EQ(foreign.index(), pair.first.get_predicate().index());
        ASSERT_NE(foreign, pair.first.get_predicate());
        auto absent = condition.template get_atoms<F>(true, foreign);
        EXPECT_TRUE(absent.begin() == absent.end());
    };
    check.template operator()<tyr::formalism::StaticTag>(static_atoms);
    check.template operator()<tyr::formalism::FluentTag>(fluent_atoms);
    check.template operator()<tyr::formalism::DerivedTag>(derived_atoms);
}
}

TEST(TyrFormalismPlanningConjunctiveCondition, ProjectsAtomPolarityWithoutBorrowingTemporaryViews)
{
    expect_condition_atom_projection<tyr::GroundTag>();
    expect_condition_atom_projection<tyr::LiftedTag>();
}

}  // namespace tyr::tests::planning_conjunctive_condition
