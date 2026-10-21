#include "tyr/formalism/planning/conjunctive_condition_data.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/interning.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <gtest/gtest.h>
#include <ranges>
#include <string>
#include <utility>

namespace lifted_tests
{

using ConjunctiveConditionIndex = ygg::Index<tyr::formalism::planning::ConjunctiveCondition<::tyr::LiftedTag>>;
using ConjunctiveConditionData = ygg::Data<tyr::formalism::planning::ConjunctiveCondition<::tyr::LiftedTag>>;
using ConjunctiveConditionView = ygg::View<ConjunctiveConditionIndex, tyr::formalism::planning::Repository>;

static_assert(std::constructible_from<ConjunctiveConditionIndex, ygg::uint_t>);
static_assert(std::totally_ordered<ConjunctiveConditionIndex>);
static_assert(std::totally_ordered<ConjunctiveConditionData>);
static_assert(std::totally_ordered<ConjunctiveConditionView>);
static_assert(std::same_as<ConjunctiveConditionView, tyr::formalism::planning::ConjunctiveConditionView<::tyr::LiftedTag>>);
static_assert(requires(ConjunctiveConditionData& data) {
    data.index;
    data.variables;
    data.static_literals;
    data.fluent_literals;
    data.derived_literals;
    data.numeric_constraints;
    data.clear();
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const ConjunctiveConditionView& view) {
    requires std::same_as<std::ranges::range_value_t<decltype(view.template get_atoms_view<tyr::formalism::StaticTag>(true))>,
                          tyr::formalism::planning::AtomView<::tyr::LiftedTag, tyr::formalism::StaticTag>>;
    requires std::same_as<std::ranges::range_value_t<decltype(view.template get_atoms_view<tyr::formalism::FluentTag>(true))>,
                          tyr::formalism::planning::AtomView<::tyr::LiftedTag, tyr::formalism::FluentTag>>;
    requires std::same_as<std::ranges::range_value_t<decltype(view.template get_atoms_view<tyr::formalism::DerivedTag>(true))>,
                          tyr::formalism::planning::AtomView<::tyr::LiftedTag, tyr::formalism::DerivedTag>>;
    view.get_index();
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

using GroundConjunctiveConditionIndex = ygg::Index<tyr::formalism::planning::ConjunctiveCondition<::tyr::GroundTag>>;
using GroundConjunctiveConditionData = ygg::Data<tyr::formalism::planning::ConjunctiveCondition<::tyr::GroundTag>>;
using GroundConjunctiveConditionView = ygg::View<GroundConjunctiveConditionIndex, tyr::formalism::planning::Repository>;

static_assert(std::constructible_from<GroundConjunctiveConditionIndex, ygg::uint_t>);
static_assert(std::totally_ordered<GroundConjunctiveConditionIndex>);
static_assert(std::totally_ordered<GroundConjunctiveConditionData>);
static_assert(std::totally_ordered<GroundConjunctiveConditionView>);
static_assert(std::same_as<GroundConjunctiveConditionView, tyr::formalism::planning::ConjunctiveConditionView<::tyr::GroundTag>>);
static_assert(requires(GroundConjunctiveConditionData& data) {
    data.index;
    data.positive_facts;
    data.negative_facts;
    data.static_literals;
    data.derived_literals;
    data.numeric_constraints;
    data.clear();
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const GroundConjunctiveConditionView& view) {
    requires std::same_as<std::ranges::range_value_t<decltype(view.template get_atoms_view<tyr::formalism::StaticTag>(true))>,
                          tyr::formalism::planning::AtomView<::tyr::GroundTag, tyr::formalism::StaticTag>>;
    requires std::same_as<std::ranges::range_value_t<decltype(view.template get_atoms_view<tyr::formalism::FluentTag>(true))>,
                          tyr::formalism::planning::AtomView<::tyr::GroundTag, tyr::formalism::FluentTag>>;
    requires std::same_as<std::ranges::range_value_t<decltype(view.template get_atoms_view<tyr::formalism::DerivedTag>(true))>,
                          tyr::formalism::planning::AtomView<::tyr::GroundTag, tyr::formalism::DerivedTag>>;
    view.get_index();
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
    auto factory = tyr::formalism::planning::RepositoryFactory();
    auto repository = factory.create();
    const auto intern = [&](auto data) { return tyr::formalism::planning::insert(repository, data).first; };
    const auto atoms = [&]<tyr::formalism::FactKind F>()
    {
        const auto atom = [&](const char* name)
        {
            const auto predicate = intern(ygg::Data<tyr::formalism::Predicate<F>>(std::string(name), 0));
            if constexpr (std::same_as<Kind, tyr::GroundTag>)
            {
                auto binding = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<F>>> {};
                binding.relation = predicate.get_index();
                return intern(ygg::Data<tyr::formalism::planning::Atom<Kind, F>>(intern(std::move(binding)).get_index()));
            }
            else
                return intern(ygg::Data<tyr::formalism::planning::Atom<Kind, F>>(predicate.get_index(), {}));
        };
        const auto positive = atom("positive");
        return std::pair(positive, atom("negative"));
    };
    const auto static_atoms = atoms.template operator()<tyr::formalism::StaticTag>();
    const auto fluent_atoms = atoms.template operator()<tyr::formalism::FluentTag>();
    const auto derived_atoms = atoms.template operator()<tyr::formalism::DerivedTag>();
    const auto literals = [&]<tyr::formalism::FactKind F>(const auto& pair)
    {
        auto result = ygg::IndexList<tyr::formalism::planning::Literal<Kind, F>> {};
        result.push_back(intern(ygg::Data<tyr::formalism::planning::Literal<Kind, F>>(pair.first.get_index(), true)).get_index());
        result.push_back(intern(ygg::Data<tyr::formalism::planning::Literal<Kind, F>>(pair.second.get_index(), false)).get_index());
        return result;
    };
    auto data = ygg::Data<tyr::formalism::planning::ConjunctiveCondition<Kind>> {};
    data.static_literals = literals.template operator()<tyr::formalism::StaticTag>(static_atoms);
    data.derived_literals = literals.template operator()<tyr::formalism::DerivedTag>(derived_atoms);
    if constexpr (std::same_as<Kind, tyr::GroundTag>)
    {
        auto variable_data = ygg::Data<tyr::formalism::planning::FDRVariable<tyr::formalism::FluentTag>> {};
        variable_data.atoms.push_back(fluent_atoms.first.get_index());
        variable_data.atoms.push_back(fluent_atoms.second.get_index());
        const auto variable = intern(std::move(variable_data));
        data.positive_facts.emplace_back(variable.get_index(), tyr::formalism::planning::FDRValue(1));
        data.positive_facts.emplace_back(variable.get_index(), tyr::formalism::planning::FDRValue::none());
        data.negative_facts.emplace_back(variable.get_index(), tyr::formalism::planning::FDRValue(2));
        data.negative_facts.emplace_back(variable.get_index(), tyr::formalism::planning::FDRValue::none());
    }
    else
        data.fluent_literals = literals.template operator()<tyr::formalism::FluentTag>(fluent_atoms);
    const auto condition = intern(std::move(data));
    const auto check = [&]<tyr::formalism::FactKind F>(const auto& pair)
    {
        // Both the condition argument and its container-view wrappers have expired
        // before iteration starts; only the repository must remain alive.
        auto positive = ygg::make_view(condition.get_index(), repository).template get_atoms_view<F>(true);
        auto negative = ygg::make_view(condition.get_index(), repository).template get_atoms_view<F>(false);
        EXPECT_TRUE(std::ranges::equal(positive, std::array { pair.first }));
        EXPECT_TRUE(std::ranges::equal(negative, std::array { pair.second }));
        auto filtered_positive = ygg::make_view(condition.get_index(), repository).template get_atoms_view<F>(true, pair.first.get_predicate());
        auto filtered_negative = ygg::make_view(condition.get_index(), repository).template get_atoms_view<F>(false, pair.second.get_predicate());
        EXPECT_TRUE(std::ranges::equal(filtered_positive, std::array { pair.first }));
        EXPECT_TRUE(std::ranges::equal(filtered_negative, std::array { pair.second }));
        auto opposite = condition.template get_atoms_view<F>(true, pair.second.get_predicate());
        EXPECT_TRUE(opposite.begin() == opposite.end());
        auto foreign_repository = factory.create();
        auto foreign_data = ygg::Data<tyr::formalism::Predicate<F>>(std::string("positive"), 0);
        const auto foreign = tyr::formalism::planning::insert(foreign_repository, foreign_data).first;
        ASSERT_EQ(foreign.get_index(), pair.first.get_predicate().get_index());
        ASSERT_NE(foreign, pair.first.get_predicate());
        auto absent = condition.template get_atoms_view<F>(true, foreign);
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
