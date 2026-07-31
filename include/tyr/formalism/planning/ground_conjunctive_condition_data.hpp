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

#ifndef TYR_FORMALISM_PLANNING_GROUND_CONJUNCTIVE_CONDITION_DATA_HPP_
#define TYR_FORMALISM_PLANNING_GROUND_CONJUNCTIVE_CONDITION_DATA_HPP_

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>
#include "tyr/formalism/binding_index.hpp"
#include "tyr/formalism/planning/boolean_operator_data.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/ground_conjunctive_condition_index.hpp"
#include "tyr/formalism/planning/ground_literal_index.hpp"

namespace ygg
{

template<>
struct Data<::tyr::formalism::planning::GroundConjunctiveCondition>
{
    ygg::Index<::tyr::formalism::planning::GroundConjunctiveCondition> index;
    ygg::IndexList<::tyr::formalism::planning::GroundLiteral<::tyr::formalism::StaticTag>> static_literals;
    ygg::IndexList<::tyr::formalism::planning::GroundLiteral<::tyr::formalism::DerivedTag>> derived_literals;
    ygg::DataList<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> positive_facts;
    ygg::DataList<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> negative_facts;
    ygg::DataList<::tyr::formalism::planning::BooleanOperator<ygg::Data<::tyr::formalism::planning::GroundFunctionExpression>>> numeric_constraints;

    Data() = default;
    Data(ygg::IndexList<::tyr::formalism::planning::GroundLiteral<::tyr::formalism::StaticTag>> static_literals_,
         ygg::IndexList<::tyr::formalism::planning::GroundLiteral<::tyr::formalism::DerivedTag>> derived_literals_,
         ygg::DataList<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> positive_facts_,
         ygg::DataList<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> negative_facts_,
         ygg::DataList<::tyr::formalism::planning::BooleanOperator<ygg::Data<::tyr::formalism::planning::GroundFunctionExpression>>> numeric_constraints_) :
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
    Data(const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::GroundLiteral<::tyr::formalism::StaticTag>>, C>>& static_literals_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::GroundLiteral<::tyr::formalism::DerivedTag>>, C>>& derived_literals_,
         const std::vector<::ygg::View<ygg::Data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>, C>>& positive_facts_,
         const std::vector<::ygg::View<ygg::Data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>, C>>& negative_facts_,
         const std::vector<::ygg::View<ygg::Data<::tyr::formalism::planning::BooleanOperator<ygg::Data<::tyr::formalism::planning::GroundFunctionExpression>>>, C>>& numeric_constraints_) :
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

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::GroundConjunctiveCondition>);
}

#endif
