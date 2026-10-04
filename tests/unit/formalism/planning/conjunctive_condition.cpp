#include "tyr/formalism/planning/conjunctive_condition_data.hpp"
#include "tyr/formalism/planning/conjunctive_condition_index.hpp"
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
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

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

using ConjunctiveConditionIndex = ygg::Index<fp::ConjunctiveCondition<::tyr::LiftedTag>>;
using ConjunctiveConditionData = ygg::Data<fp::ConjunctiveCondition<::tyr::LiftedTag>>;
using ConjunctiveConditionView = ygg::View<ConjunctiveConditionIndex, fp::Repository>;

static_assert(std::constructible_from<ConjunctiveConditionIndex, ygg::uint_t>);
static_assert(std::totally_ordered<ConjunctiveConditionIndex>);
static_assert(std::totally_ordered<ConjunctiveConditionData>);
static_assert(std::totally_ordered<ConjunctiveConditionView>);
static_assert(std::same_as<ConjunctiveConditionView, fp::ConjunctiveConditionView<::tyr::LiftedTag>>);
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
    requires std::same_as<std::ranges::range_value_t<decltype(view.template get_atoms_view<f::StaticTag>(true))>, fp::AtomView<::tyr::LiftedTag, f::StaticTag>>;
    requires std::same_as<std::ranges::range_value_t<decltype(view.template get_atoms_view<f::FluentTag>(true))>, fp::AtomView<::tyr::LiftedTag, f::FluentTag>>;
    requires std::same_as<std::ranges::range_value_t<decltype(view.template get_atoms_view<f::DerivedTag>(true))>, fp::AtomView<::tyr::LiftedTag, f::DerivedTag>>;
    view.get_index();
    view.get_variables();
    view.template get_literals<f::StaticTag>();
    view.template get_literals<f::FluentTag>();
    view.template get_literals<f::DerivedTag>();
    view.get_numeric_constraints();
    view.get_arity();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});

}

namespace ground_tests
{

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

using GroundConjunctiveConditionIndex = ygg::Index<fp::ConjunctiveCondition<::tyr::GroundTag>>;
using GroundConjunctiveConditionData = ygg::Data<fp::ConjunctiveCondition<::tyr::GroundTag>>;
using GroundConjunctiveConditionView = ygg::View<GroundConjunctiveConditionIndex, fp::Repository>;

static_assert(std::constructible_from<GroundConjunctiveConditionIndex, ygg::uint_t>);
static_assert(std::totally_ordered<GroundConjunctiveConditionIndex>);
static_assert(std::totally_ordered<GroundConjunctiveConditionData>);
static_assert(std::totally_ordered<GroundConjunctiveConditionView>);
static_assert(std::same_as<GroundConjunctiveConditionView, fp::ConjunctiveConditionView<::tyr::GroundTag>>);
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
    requires std::same_as<std::ranges::range_value_t<decltype(view.template get_atoms_view<f::StaticTag>(true))>, fp::AtomView<::tyr::GroundTag, f::StaticTag>>;
    requires std::same_as<std::ranges::range_value_t<decltype(view.template get_atoms_view<f::FluentTag>(true))>, fp::AtomView<::tyr::GroundTag, f::FluentTag>>;
    requires std::same_as<std::ranges::range_value_t<decltype(view.template get_atoms_view<f::DerivedTag>(true))>, fp::AtomView<::tyr::GroundTag, f::DerivedTag>>;
    view.get_index();
    view.template get_literals<f::StaticTag>();
    view.template get_literals<f::DerivedTag>();
    view.template get_facts<f::PositiveTag>();
    view.template get_facts<f::NegativeTag>();
    view.get_numeric_constraints();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});

}

namespace
{
namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<tyr::TaskKind Kind>
void expect_condition_atom_projection()
{
    auto repository = fp::RepositoryFactory().create();
    const auto intern = [&](auto data) { return fp::insert(repository, data).first; };
    const auto atoms = [&]<f::FactKind F>()
    {
        const auto atom = [&](const char* name)
        {
            const auto predicate = intern(ygg::Data<f::Predicate<F>>(std::string(name), 0));
            if constexpr (std::same_as<Kind, tyr::GroundTag>)
            {
                auto binding = ygg::Data<f::RelationBinding<f::Predicate<F>>> {};
                binding.relation = predicate.get_index();
                return intern(ygg::Data<fp::Atom<Kind, F>>(intern(std::move(binding)).get_index()));
            }
            else
                return intern(ygg::Data<fp::Atom<Kind, F>>(predicate.get_index(), {}));
        };
        return std::pair(atom("positive"), atom("negative"));
    };
    const auto static_atoms = atoms.template operator()<f::StaticTag>();
    const auto fluent_atoms = atoms.template operator()<f::FluentTag>();
    const auto derived_atoms = atoms.template operator()<f::DerivedTag>();
    const auto literals = [&]<f::FactKind F>(const auto& pair)
    {
        auto result = ygg::IndexList<fp::Literal<Kind, F>> {};
        result.push_back(intern(ygg::Data<fp::Literal<Kind, F>>(pair.first.get_index(), true)).get_index());
        result.push_back(intern(ygg::Data<fp::Literal<Kind, F>>(pair.second.get_index(), false)).get_index());
        return result;
    };
    auto data = ygg::Data<fp::ConjunctiveCondition<Kind>> {};
    data.static_literals = literals.template operator()<f::StaticTag>(static_atoms);
    data.derived_literals = literals.template operator()<f::DerivedTag>(derived_atoms);
    if constexpr (std::same_as<Kind, tyr::GroundTag>)
    {
        auto variable_data = ygg::Data<fp::FDRVariable<f::FluentTag>> {};
        variable_data.atoms.push_back(fluent_atoms.first.get_index());
        variable_data.atoms.push_back(fluent_atoms.second.get_index());
        const auto variable = intern(std::move(variable_data));
        data.positive_facts.emplace_back(variable.get_index(), fp::FDRValue(1));
        data.positive_facts.emplace_back(variable.get_index(), fp::FDRValue::none());
        data.negative_facts.emplace_back(variable.get_index(), fp::FDRValue(2));
        data.negative_facts.emplace_back(variable.get_index(), fp::FDRValue::none());
    }
    else
        data.fluent_literals = literals.template operator()<f::FluentTag>(fluent_atoms);
    const auto condition = intern(std::move(data));
    const auto check = [&]<f::FactKind F>(const auto& pair)
    {
        // Both the condition argument and its container-view wrappers have expired
        // before iteration starts; only the repository must remain alive.
        auto positive = ygg::make_view(condition.get_index(), repository).template get_atoms_view<F>(true);
        auto negative = ygg::make_view(condition.get_index(), repository).template get_atoms_view<F>(false);
        EXPECT_TRUE(std::ranges::equal(positive, std::array { pair.first }));
        EXPECT_TRUE(std::ranges::equal(negative, std::array { pair.second }));
    };
    check.template operator()<f::StaticTag>(static_atoms);
    check.template operator()<f::FluentTag>(fluent_atoms);
    check.template operator()<f::DerivedTag>(derived_atoms);
}
}

TEST(TyrFormalismPlanningConjunctiveCondition, ProjectsAtomPolarityWithoutBorrowingTemporaryViews)
{
    expect_condition_atom_projection<tyr::GroundTag>();
    expect_condition_atom_projection<tyr::LiftedTag>();
}
