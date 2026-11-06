/*
 * Copyright (C) 2025-2026 Dominik Drexler
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef TYR_FORMALISM_PLANNING_CONJUNCTIVE_CONDITION_DATA_HPP_
#define TYR_FORMALISM_PLANNING_CONJUNCTIVE_CONDITION_DATA_HPP_

#include "tyr/formalism/binding_index.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/binary_operator_data.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"

#include <tuple>
#include <utility>
#include <vector>
#include <urd/elements/flat.hpp>
#include <urd/repr/core/dependent_false.hpp>

namespace urd
{

template<>
struct data<::tyr::formalism::planning::ConjunctiveCondition<::tyr::LiftedTag>>
{
    urd::index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::LiftedTag>> index;
    urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables;
    urd::flat_vector<urd::index<::tyr::formalism::planning::Literal<::tyr::LiftedTag, ::tyr::formalism::StaticTag>>> static_literals;
    urd::flat_vector<urd::index<::tyr::formalism::planning::Literal<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>> fluent_literals;
    urd::flat_vector<urd::index<::tyr::formalism::planning::Literal<::tyr::LiftedTag, ::tyr::formalism::DerivedTag>>> derived_literals;
    urd::flat_vector<urd::index<::tyr::formalism::planning::BinaryOperator<::tyr::LiftedTag, ::tyr::formalism::BooleanOperatorKind>>> numeric_constraints;

    data() = default;
    data(urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::Literal<::tyr::LiftedTag, ::tyr::formalism::StaticTag>>> static_literals_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::Literal<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>> fluent_literals_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::Literal<::tyr::LiftedTag, ::tyr::formalism::DerivedTag>>> derived_literals_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::BinaryOperator<::tyr::LiftedTag, ::tyr::formalism::BooleanOperatorKind>>> numeric_constraints_) :
        index(),
        variables(std::move(variables_)),
        static_literals(std::move(static_literals_)),
        fluent_literals(std::move(fluent_literals_)),
        derived_literals(std::move(derived_literals_)),
        numeric_constraints(std::move(numeric_constraints_))
    {
    }
    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;

    template<::tyr::formalism::FactKind T>
    [[nodiscard]] const auto& get_literals() const
    {
        if constexpr (std::same_as<T, ::tyr::formalism::StaticTag>)
            return static_literals;
        else if constexpr (std::same_as<T, ::tyr::formalism::FluentTag>)
            return fluent_literals;
        else if constexpr (std::same_as<T, ::tyr::formalism::DerivedTag>)
            return derived_literals;
        else
            static_assert(urd::repr::dependent_false<T>::value, "Missing case");
    }

    static constexpr auto cista_members() noexcept
    {
        return urd::
            Members<&data::index, &data::variables, &data::static_literals, &data::fluent_literals, &data::derived_literals, &data::numeric_constraints> {};
    }
    static constexpr auto identifying_members() noexcept
    { return urd::Members<&data::variables, &data::static_literals, &data::fluent_literals, &data::derived_literals, &data::numeric_constraints> {}; }
};

template<>
struct data<::tyr::formalism::planning::ConjunctiveCondition<::tyr::GroundTag>>
{
    urd::index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::GroundTag>> index;
    urd::flat_vector<urd::index<::tyr::formalism::planning::Literal<::tyr::GroundTag, ::tyr::formalism::StaticTag>>> static_literals;
    urd::flat_vector<urd::index<::tyr::formalism::planning::Literal<::tyr::GroundTag, ::tyr::formalism::DerivedTag>>> derived_literals;
    urd::flat_vector<urd::data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>> positive_facts;
    urd::flat_vector<urd::data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>> negative_facts;
    urd::flat_vector<urd::index<::tyr::formalism::planning::BinaryOperator<::tyr::GroundTag, ::tyr::formalism::BooleanOperatorKind>>> numeric_constraints;

    data() = default;
    data(urd::flat_vector<urd::index<::tyr::formalism::planning::Literal<::tyr::GroundTag, ::tyr::formalism::StaticTag>>> static_literals_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::Literal<::tyr::GroundTag, ::tyr::formalism::DerivedTag>>> derived_literals_,
         urd::flat_vector<urd::data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>> positive_facts_,
         urd::flat_vector<urd::data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>> negative_facts_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::BinaryOperator<::tyr::GroundTag, ::tyr::formalism::BooleanOperatorKind>>> numeric_constraints_) :
        index(),
        static_literals(std::move(static_literals_)),
        derived_literals(std::move(derived_literals_)),
        positive_facts(std::move(positive_facts_)),
        negative_facts(std::move(negative_facts_)),
        numeric_constraints(std::move(numeric_constraints_))
    {
    }
    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;

    template<::tyr::formalism::FactKind T>
    [[nodiscard]] const auto& get_literals() const
    {
        if constexpr (std::same_as<T, ::tyr::formalism::StaticTag>)
            return static_literals;
        else if constexpr (std::same_as<T, ::tyr::formalism::DerivedTag>)
            return derived_literals;
        else
            static_assert(urd::repr::dependent_false<T>::value, "Missing case");
    }

    template<::tyr::formalism::PolarityKind T>
    [[nodiscard]] const auto& get_facts() const
    {
        if constexpr (std::same_as<T, ::tyr::formalism::PositiveTag>)
            return positive_facts;
        else if constexpr (std::same_as<T, ::tyr::formalism::NegativeTag>)
            return negative_facts;
        else
            static_assert(urd::repr::dependent_false<T>::value, "Missing case");
    }

    static constexpr auto cista_members() noexcept
    {
        return urd::
            Members<&data::index, &data::positive_facts, &data::negative_facts, &data::static_literals, &data::derived_literals, &data::numeric_constraints> {};
    }
    static constexpr auto identifying_members() noexcept
    { return urd::Members<&data::positive_facts, &data::negative_facts, &data::static_literals, &data::derived_literals, &data::numeric_constraints> {}; }
};

}

#endif
