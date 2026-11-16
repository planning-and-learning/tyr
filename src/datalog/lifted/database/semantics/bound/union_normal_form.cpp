/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/semantics/bound/union_normal_form.hpp"

#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::datalog::database::bound
{
UnionNormalForm union_normal_form(QueryView<> query) { return detail::UnionNormalFormBuilder {}.build(query); }
}

namespace tyr::datalog::database::bound::detail
{
urd::Store<urd::sequence<ConjunctiveBranch>> UnionNormalFormBuilder::product(const urd::Store<urd::sequence<ConjunctiveBranch>>& left_operand,
                                                                             const urd::Store<urd::sequence<ConjunctiveBranch>>& right_operand)
{
    urd::Store<urd::sequence<ConjunctiveBranch>> result;
    for (const auto& l : left_operand)
        for (const auto& r : right_operand)
        {
            auto& merged = result.emplace_back(l);
            merged.atoms.insert(merged.atoms.end(), r.atoms.begin(), r.atoms.end());
            merged.equal.insert(merged.equal.end(), r.equal.begin(), r.equal.end());
            merged.value.insert(merged.value.end(), r.value.begin(), r.value.end());
            merged.negations.insert(merged.negations.end(), r.negations.begin(), r.negations.end());
        }
    return result;
}
}

namespace tyr::datalog::database::bound::detail
{
urd::Store<urd::sequence<ConjunctiveBranch>> UnionNormalFormBuilder::flatten(QueryView<> query, std::span<const uint32_t> mapping)
{
    const auto columns = columns_of(query);
    return std::visit(
        [&]<typename Tag>(QueryView<Tag> concrete) -> urd::Store<urd::sequence<ConjunctiveBranch>>
        {
            if constexpr (std::same_as<Tag, QueryInputTag>)
            {
                const auto slot = concrete.get_input_slot();
                if (m_result.input_columns.size() <= slot)
                    m_result.input_columns.resize(slot + 1);
                if (!m_result.input_columns[slot].empty() && !std::ranges::equal(m_result.input_columns[slot], columns))
                    throw std::invalid_argument("Union normal form: an input slot has two schemas.");
                m_result.input_columns[slot].assign(columns.begin(), columns.end());
                auto branch = ConjunctiveBranch {};
                branch.atoms.push_back(ConjunctiveAtom { concrete.get_input_slot(), urd::Store<urd::sequence<uint32_t>>(mapping.begin(), mapping.end()) });
                return { std::move(branch) };
            }
            else if constexpr (std::same_as<Tag, QueryEmptyTag>)
                return {};
            else if constexpr (std::same_as<Tag, QueryJoinTag> || std::same_as<Tag, QuerySemijoinTag>)
            {
                // A semijoin's right-only columns are not in the output, so they get fresh variables.
                const auto left_operand = concrete.get_left_operand();
                const auto right_operand = concrete.get_right_operand();
                return product(flatten(left_operand, by_label(columns_of(left_operand), columns, mapping)),
                               flatten(right_operand, by_label(columns_of(right_operand), columns, mapping)));
            }
            else if constexpr (std::same_as<Tag, QueryGenericJoinTag>)
            {
                // Labels outside the output are shared by all inputs that name them.
                urd::Store<urd::sequence<ygg::ColumnLayout>> labels(columns.begin(), columns.end());
                urd::Store<urd::sequence<uint32_t>> variables(mapping.begin(), mapping.end());
                auto result = urd::Store<urd::sequence<ConjunctiveBranch>>(1);
                const auto& repository = get_repository(concrete.context());
                for (const auto input : concrete.get_inputs())
                {
                    const auto child = input;
                    for (const auto& column : columns_of(child))
                        if (std::ranges::find(labels, column.label, &ygg::ColumnLayout::label) == labels.end())
                        {
                            labels.push_back(column);
                            variables.push_back(fresh(column));
                        }
                    result = product(result, flatten(child, by_label(columns_of(child), labels, variables)));
                }
                return result;
            }
            else if constexpr (std::same_as<Tag, QueryProjectTag>)
            {
                const auto arg = concrete.get_operand();
                return flatten(arg, by_label(columns_of(arg), columns, mapping));
            }
            else if constexpr (std::same_as<Tag, QueryRenameTag>)
                return flatten(concrete.get_operand(), mapping);
            else if constexpr (std::same_as<Tag, QuerySelectEqualTag> || std::same_as<Tag, QuerySelectValueTag>)
            {
                auto branches = flatten(concrete.get_operand(), mapping);
                for (auto& branch : branches)
                {
                    if constexpr (std::same_as<Tag, QuerySelectEqualTag>)
                        branch.equal.emplace_back(mapping[concrete.get_plan().left_position()], mapping[concrete.get_plan().right_position()]);
                    else
                    {
                        branch.value.emplace_back(mapping[concrete.get_plan().position()], concrete.get_constant());
                    }
                }
                return branches;
            }
            else if constexpr (std::same_as<Tag, QueryUnionTag>)
            {
                const auto left_operand = concrete.get_left_operand();
                const auto right_operand = concrete.get_right_operand();
                auto result = flatten(left_operand, by_label(columns_of(left_operand), columns, mapping));
                auto right = flatten(right_operand, by_label(columns_of(right_operand), columns, mapping));
                result.insert(result.end(), std::make_move_iterator(right.begin()), std::make_move_iterator(right.end()));
                return result;
            }
            else if constexpr (std::same_as<Tag, QueryDifferenceTag> || std::same_as<Tag, QueryAntijoinTag>)
            {
                // An antijoin's right-only columns get fresh variables inside the negation.
                const auto left_operand = concrete.get_left_operand();
                const auto right_operand = concrete.get_right_operand();
                auto result = flatten(left_operand, by_label(columns_of(left_operand), columns, mapping));
                auto negation = ConjunctiveNegation { urd::Store<urd::sequence<uint32_t>>(mapping.begin(), mapping.end()),
                                                      flatten(right_operand, by_label(columns_of(right_operand), columns, mapping)) };
                for (auto& branch : result)
                    branch.negations.push_back(negation);
                return result;
            }
            else
            {
                static_assert(std::same_as<Tag, QueryDistanceTag>, "unhandled query operator");
                throw std::invalid_argument("Union normal form: distances are not first-order queries.");
            }
        },
        query.get_variant());
}
}
