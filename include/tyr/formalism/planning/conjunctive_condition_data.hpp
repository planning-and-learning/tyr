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
#include "tyr/formalism/planning/boolean_operator_data.hpp"
#include "tyr/formalism/planning/conjunctive_condition_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/literal_index.hpp"
#include "tyr/formalism/variable_index.hpp"

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{

template<>
struct Data<::tyr::formalism::planning::ConjunctiveCondition<::tyr::LiftedTag>>
{
    ygg::Index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::LiftedTag>> index;
    ygg::IndexList<::tyr::formalism::Variable> variables;
    ygg::IndexList<::tyr::formalism::planning::Literal<::tyr::LiftedTag, ::tyr::formalism::StaticTag>> static_literals;
    ygg::IndexList<::tyr::formalism::planning::Literal<::tyr::LiftedTag, ::tyr::formalism::FluentTag>> fluent_literals;
    ygg::IndexList<::tyr::formalism::planning::Literal<::tyr::LiftedTag, ::tyr::formalism::DerivedTag>> derived_literals;
    ygg::DataList<::tyr::formalism::planning::BooleanOperator<::tyr::LiftedTag>> numeric_constraints;

    Data() = default;
    Data(ygg::IndexList<::tyr::formalism::Variable> variables_,
         ygg::IndexList<::tyr::formalism::planning::Literal<::tyr::LiftedTag, ::tyr::formalism::StaticTag>> static_literals_,
         ygg::IndexList<::tyr::formalism::planning::Literal<::tyr::LiftedTag, ::tyr::formalism::FluentTag>> fluent_literals_,
         ygg::IndexList<::tyr::formalism::planning::Literal<::tyr::LiftedTag, ::tyr::formalism::DerivedTag>> derived_literals_,
         ygg::DataList<::tyr::formalism::planning::BooleanOperator<::tyr::LiftedTag>> numeric_constraints_) :
        index(),
        variables(std::move(variables_)),
        static_literals(std::move(static_literals_)),
        fluent_literals(std::move(fluent_literals_)),
        derived_literals(std::move(derived_literals_)),
        numeric_constraints(std::move(numeric_constraints_))
    {
    }
    // Python constructor
    template<typename C>
    Data(const std::vector<::ygg::View<ygg::Index<::tyr::formalism::Variable>, C>>& variables_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::Literal<::tyr::LiftedTag, ::tyr::formalism::StaticTag>>, C>>& static_literals_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::Literal<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>, C>>& fluent_literals_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::Literal<::tyr::LiftedTag, ::tyr::formalism::DerivedTag>>, C>>& derived_literals_,
         const std::vector<::ygg::View<ygg::Data<::tyr::formalism::planning::BooleanOperator<::tyr::LiftedTag>>, C>>& numeric_constraints_) :
        index(),
        variables(),
        static_literals(),
        fluent_literals(),
        derived_literals(),
        numeric_constraints()
    {
        set(variables_, variables);
        set(static_literals_, static_literals);
        set(fluent_literals_, fluent_literals);
        set(derived_literals_, derived_literals);
        set(numeric_constraints_, numeric_constraints);
    }
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(variables);
        ygg::clear(static_literals);
        ygg::clear(fluent_literals);
        ygg::clear(derived_literals);
        ygg::clear(numeric_constraints);
    }

    template<::tyr::formalism::FactKind T>
    const auto& get_literals() const
    {
        if constexpr (std::same_as<T, ::tyr::formalism::StaticTag>)
            return static_literals;
        else if constexpr (std::same_as<T, ::tyr::formalism::FluentTag>)
            return fluent_literals;
        else if constexpr (std::same_as<T, ::tyr::formalism::DerivedTag>)
            return derived_literals;
        else
            static_assert(ygg::dependent_false<T>::value, "Missing case");
    }

    auto cista_members() const noexcept { return std::tie(index, variables, static_literals, fluent_literals, derived_literals, numeric_constraints); }
    auto identifying_members() const noexcept { return std::tie(variables, static_literals, fluent_literals, derived_literals, numeric_constraints); }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::ConjunctiveCondition<::tyr::LiftedTag>>);

template<>
struct Data<::tyr::formalism::planning::ConjunctiveCondition<::tyr::GroundTag>>
{
    ygg::Index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::GroundTag>> index;
    ygg::IndexList<::tyr::formalism::planning::Literal<::tyr::GroundTag, ::tyr::formalism::StaticTag>> static_literals;
    ygg::IndexList<::tyr::formalism::planning::Literal<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> derived_literals;
    ygg::DataList<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> positive_facts;
    ygg::DataList<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> negative_facts;
    ygg::DataList<::tyr::formalism::planning::BooleanOperator<::tyr::GroundTag>> numeric_constraints;

    Data() = default;
    Data(ygg::IndexList<::tyr::formalism::planning::Literal<::tyr::GroundTag, ::tyr::formalism::StaticTag>> static_literals_,
         ygg::IndexList<::tyr::formalism::planning::Literal<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> derived_literals_,
         ygg::DataList<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> positive_facts_,
         ygg::DataList<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> negative_facts_,
         ygg::DataList<::tyr::formalism::planning::BooleanOperator<::tyr::GroundTag>> numeric_constraints_) :
        index(),
        static_literals(std::move(static_literals_)),
        derived_literals(std::move(derived_literals_)),
        positive_facts(std::move(positive_facts_)),
        negative_facts(std::move(negative_facts_)),
        numeric_constraints(std::move(numeric_constraints_))
    {
    }
    // Python constructor
    template<typename C>
    Data(const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::Literal<::tyr::GroundTag, ::tyr::formalism::StaticTag>>, C>>& static_literals_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::Literal<::tyr::GroundTag, ::tyr::formalism::DerivedTag>>, C>>& derived_literals_,
         const std::vector<::ygg::View<ygg::Data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>, C>>& positive_facts_,
         const std::vector<::ygg::View<ygg::Data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>, C>>& negative_facts_,
         const std::vector<::ygg::View<ygg::Data<::tyr::formalism::planning::BooleanOperator<::tyr::GroundTag>>, C>>& numeric_constraints_) :
        index(),
        static_literals(),
        derived_literals(),
        positive_facts(),
        negative_facts(),
        numeric_constraints()
    {
        set(static_literals_, static_literals);
        set(derived_literals_, derived_literals);
        set(positive_facts_, positive_facts);
        set(negative_facts_, negative_facts);
        set(numeric_constraints_, numeric_constraints);
    }
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(static_literals);
        ygg::clear(derived_literals);
        ygg::clear(positive_facts);
        ygg::clear(negative_facts);
        ygg::clear(numeric_constraints);
    }

    template<::tyr::formalism::FactKind T>
    const auto& get_literals() const
    {
        if constexpr (std::same_as<T, ::tyr::formalism::StaticTag>)
            return static_literals;
        else if constexpr (std::same_as<T, ::tyr::formalism::DerivedTag>)
            return derived_literals;
        else
            static_assert(ygg::dependent_false<T>::value, "Missing case");
    }

    template<::tyr::formalism::PolarityKind T>
    const auto& get_facts() const
    {
        if constexpr (std::same_as<T, ::tyr::formalism::PositiveTag>)
            return positive_facts;
        else if constexpr (std::same_as<T, ::tyr::formalism::NegativeTag>)
            return negative_facts;
        else
            static_assert(ygg::dependent_false<T>::value, "Missing case");
    }

    auto cista_members() const noexcept { return std::tie(index, positive_facts, negative_facts, static_literals, derived_literals, numeric_constraints); }
    auto identifying_members() const noexcept { return std::tie(positive_facts, negative_facts, static_literals, derived_literals, numeric_constraints); }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::ConjunctiveCondition<::tyr::GroundTag>>);
}

#endif
