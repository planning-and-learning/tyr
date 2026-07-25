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

#include "tyr/planning/ground/match_tree/match_tree.hpp"

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/applicability.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/match_tree/canonicalization.hpp"
#include "tyr/planning/ground/match_tree/declarations.hpp"
#include "tyr/planning/ground/match_tree/nodes/atom_view.hpp"
#include "tyr/planning/ground/match_tree/nodes/constraint_view.hpp"
#include "tyr/planning/ground/match_tree/nodes/generator_view.hpp"
#include "tyr/planning/ground/match_tree/nodes/negative_fact_view.hpp"
#include "tyr/planning/ground/match_tree/nodes/node_data.hpp"
#include "tyr/planning/ground/match_tree/nodes/variable_view.hpp"
#include "tyr/planning/ground/match_tree/repository.hpp"
#include "tyr/planning/ground/state_builder.hpp"

#include <algorithm>
#include <cassert>
#include <deque>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <variant>
#include <yggdrasil/semantics/comparison.hpp>
#include <vector>
#include <yggdrasil/core/types.hpp>

namespace tyr::planning::match_tree
{

using PreconditionVariant =
    std::variant<ygg::Index<::tyr::formalism::planning::GroundAtom<::tyr::formalism::DerivedTag>>,
                 ygg::Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>>,
                 ygg::Data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>,
                 ygg::Data<::tyr::formalism::planning::BooleanOperator<ygg::Data<::tyr::formalism::planning::GroundFunctionExpression>>>>;

template<typename Tag>
using PreconditionOccurrences = ygg::UnorderedMap<PreconditionVariant, ygg::IndexList<Tag>>;

template<typename Tag>
using PreconditionDetails =
    ygg::UnorderedMap<ygg::Index<Tag>, ygg::UnorderedMap<PreconditionVariant, std::variant<std::monostate, bool, ::tyr::formalism::planning::FDRValue>>>;

template<typename Tag>
struct BaseEntry
{
    size_t depth;
    std::span<ygg::Index<Tag>> elements;

    BaseEntry(size_t depth, std::span<ygg::Index<Tag>> elements) : depth(depth), elements(elements) {}
};

template<typename Tag>
struct AtomStackEntry;

template<typename Tag>
struct VariableStackEntry;

template<typename Tag>
struct NegativeFactStackEntry;

template<typename Tag>
struct ConstraintStackEntry;

template<typename Tag>
struct GeneratorStackEntry;

template<typename Tag>
using StackEntry = std::variant<AtomStackEntry<Tag>, VariableStackEntry<Tag>, NegativeFactStackEntry<Tag>, ConstraintStackEntry<Tag>, GeneratorStackEntry<Tag>>;

template<typename Tag>
static std::optional<StackEntry<Tag>> try_create_stack_entry(BaseEntry<Tag> base,
                                                             const std::vector<std::pair<PreconditionVariant, ygg::IndexList<Tag>>>& sorted_preconditions,
                                                             const PreconditionDetails<Tag>& details,
                                                             const ::tyr::formalism::planning::Repository& context);

template<typename Tag>
struct AtomStackEntry
{
    BaseEntry<Tag> base;

    std::span<ygg::Index<Tag>> true_elements;
    std::span<ygg::Index<Tag>> false_elements;
    std::span<ygg::Index<Tag>> dontcare_elements;

    ygg::Data<AtomSelectorNode<Tag>> result;

    AtomStackEntry(BaseEntry<Tag> base,
                   ygg::Index<::tyr::formalism::planning::GroundAtom<::tyr::formalism::DerivedTag>> atom,
                   std::span<ygg::Index<Tag>> true_elements,
                   std::span<ygg::Index<Tag>> false_elements,
                   std::span<ygg::Index<Tag>> dontcare_elements) :
        base(base),
        true_elements(true_elements),
        false_elements(false_elements),
        dontcare_elements(dontcare_elements),
        result()
    {
        result.atom = atom;
    }

    bool explored_true_child() const noexcept { return (true_elements.empty() || result.true_child.has_value()); }
    bool explored_false_child() const noexcept { return (false_elements.empty() || result.false_child.has_value()); }
    bool explored_dontcare_child() const noexcept { return (dontcare_elements.empty() || result.dontcare_child.has_value()); }
};

template<typename Tag>
struct VariableStackEntry
{
    BaseEntry<Tag> base;

    std::vector<std::span<ygg::Index<Tag>>> domain_elements;
    std::vector<ygg::uint_t> forward;
    std::span<ygg::Index<Tag>> dontcare_elements;
    size_t forward_pos;

    ygg::Data<VariableSelectorNode<Tag>> result;

    VariableStackEntry(BaseEntry<Tag> base,
                       ygg::Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> variable,
                       std::vector<std::span<ygg::Index<Tag>>> domain_elements_,
                       std::vector<ygg::uint_t> forward_,
                       std::span<ygg::Index<Tag>> dontcare_elements) :
        base(base),
        domain_elements(std::move(domain_elements_)),
        forward(std::move(forward_)),
        dontcare_elements(dontcare_elements),
        forward_pos(0),
        result()
    {
        result.variable = variable;
        result.domain_children.resize(domain_elements.size());
    }

    bool explored_children() const noexcept { return forward_pos == forward.size(); }
    bool explored_dontcare_child() const noexcept { return (dontcare_elements.empty() || result.dontcare_child.has_value()); }
};

template<typename Tag>
struct NegativeFactStackEntry
{
    BaseEntry<Tag> base;

    std::span<ygg::Index<Tag>> true_elements;
    std::span<ygg::Index<Tag>> dontcare_elements;

    ygg::Data<NegativeFactSelectorNode<Tag>> result;

    NegativeFactStackEntry(BaseEntry<Tag> base,
                           ygg::Data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> fact,
                           std::span<ygg::Index<Tag>> true_elements,
                           std::span<ygg::Index<Tag>> dontcare_elements) :
        base(base),
        true_elements(true_elements),
        dontcare_elements(dontcare_elements),
        result()
    {
        result.fact = fact;
    }

    bool explored_true_child() const noexcept { return (true_elements.empty() || result.true_child.has_value()); }
    bool explored_dontcare_child() const noexcept { return (dontcare_elements.empty() || result.dontcare_child.has_value()); }
};

template<typename Tag>
struct ConstraintStackEntry
{
    BaseEntry<Tag> base;

    ygg::Data<::tyr::formalism::planning::BooleanOperator<ygg::Data<::tyr::formalism::planning::GroundFunctionExpression>>> constraint;
    std::span<ygg::Index<Tag>> true_elements;
    std::span<ygg::Index<Tag>> dontcare_elements;

    ygg::Data<NumericConstraintSelectorNode<Tag>> result;

    ConstraintStackEntry(BaseEntry<Tag> base,
                         ygg::Data<::tyr::formalism::planning::BooleanOperator<ygg::Data<::tyr::formalism::planning::GroundFunctionExpression>>> constraint,
                         std::span<ygg::Index<Tag>> true_elements,
                         std::span<ygg::Index<Tag>> dontcare_elements) :
        base(base),
        constraint(constraint),
        true_elements(true_elements),
        dontcare_elements(dontcare_elements),
        result()
    {
        result.constraint = constraint;
    }

    bool explored_true_child() const noexcept { return (true_elements.empty() || result.true_child.has_value()); }
    bool explored_dontcare_child() const noexcept { return (dontcare_elements.empty() || result.dontcare_child.has_value()); }
};

template<typename Tag>
struct GeneratorStackEntry
{
    BaseEntry<Tag> base;

    ygg::Data<planning::match_tree::ElementGeneratorNode<Tag>> result;

    explicit GeneratorStackEntry(BaseEntry<Tag> base) : base(base), result()
    {
        result.elements.insert(result.elements.end(), base.elements.begin(), base.elements.end());

        // std::cout << "Num elements in generator node: " << result.elements.size() << std::endl;
    }
};

template<typename Entry, typename Tag>
auto store_result(Entry& entry, Repository<Tag>& repository)
{
    canonicalize(entry.result);
    return repository.get_or_create(entry.result).first;
}

template<typename Tag>
bool explored(const AtomStackEntry<Tag>& el) noexcept
{
    return el.explored_true_child() && el.explored_false_child() && el.explored_dontcare_child();
}

template<typename Tag>
std::optional<StackEntry<Tag>> next_entry(const AtomStackEntry<Tag>& el,
                                          const std::vector<std::pair<PreconditionVariant, ygg::IndexList<Tag>>>& sorted_preconditions,
                                          const PreconditionDetails<Tag>& details,
                                          const ::tyr::formalism::planning::Repository& context)
{
    if (!el.explored_true_child())
        return try_create_stack_entry(BaseEntry<Tag> { el.base.depth + 1, el.true_elements }, sorted_preconditions, details, context);
    else if (!el.explored_false_child())
        return try_create_stack_entry(BaseEntry<Tag> { el.base.depth + 1, el.false_elements }, sorted_preconditions, details, context);
    else if (!el.explored_dontcare_child())
        return try_create_stack_entry(BaseEntry<Tag> { el.base.depth + 1, el.dontcare_elements }, sorted_preconditions, details, context);
    else
        throw std::logic_error("Unexpected case.");
}

template<typename Tag>
void push_result(AtomStackEntry<Tag>& el, ygg::Data<Node<Tag>> node)
{
    if (!el.explored_true_child())
        el.result.true_child = node;
    else if (!el.explored_false_child())
        el.result.false_child = node;
    else if (!el.explored_dontcare_child())
        el.result.dontcare_child = node;
    else
        throw std::logic_error("Unexpected case.");
}

template<typename Tag>
bool explored(const VariableStackEntry<Tag>& el) noexcept
{
    return el.explored_children() && el.explored_dontcare_child();
}

template<typename Tag>
std::optional<StackEntry<Tag>> next_entry(const VariableStackEntry<Tag>& el,
                                          const std::vector<std::pair<PreconditionVariant, ygg::IndexList<Tag>>>& sorted_preconditions,
                                          const PreconditionDetails<Tag>& details,
                                          const ::tyr::formalism::planning::Repository& context)
{
    if (!el.explored_children())
        return try_create_stack_entry(BaseEntry<Tag> { el.base.depth + 1, el.domain_elements.at(el.forward.at(el.forward_pos)) },
                                      sorted_preconditions,
                                      details,
                                      context);
    else if (!el.explored_dontcare_child())
        return try_create_stack_entry(BaseEntry<Tag> { el.base.depth + 1, el.dontcare_elements }, sorted_preconditions, details, context);
    else
        throw std::logic_error("Unexpected case.");
}

template<typename Tag>
void push_result(VariableStackEntry<Tag>& el, ygg::Data<Node<Tag>> node)
{
    if (!el.explored_children())
    {
        el.result.domain_children.at(el.forward.at(el.forward_pos)) = node;
        ++el.forward_pos;
    }
    else if (!el.explored_dontcare_child())
        el.result.dontcare_child = node;
    else
        throw std::logic_error("Unexpected case.");
}

template<typename Tag>
bool explored(const NegativeFactStackEntry<Tag>& el) noexcept
{
    return el.explored_true_child() && el.explored_dontcare_child();
}

template<typename Tag>
std::optional<StackEntry<Tag>> next_entry(const NegativeFactStackEntry<Tag>& el,
                                          const std::vector<std::pair<PreconditionVariant, ygg::IndexList<Tag>>>& sorted_preconditions,
                                          const PreconditionDetails<Tag>& details,
                                          const ::tyr::formalism::planning::Repository& context)
{
    if (!el.explored_true_child())
        return try_create_stack_entry(BaseEntry<Tag> { el.base.depth + 1, el.true_elements }, sorted_preconditions, details, context);
    else if (!el.explored_dontcare_child())
        return try_create_stack_entry(BaseEntry<Tag> { el.base.depth + 1, el.dontcare_elements }, sorted_preconditions, details, context);
    else
        throw std::logic_error("Unexpected case.");
}

template<typename Tag>
void push_result(NegativeFactStackEntry<Tag>& el, ygg::Data<Node<Tag>> node)
{
    if (!el.explored_true_child())
        el.result.true_child = node;
    else if (!el.explored_dontcare_child())
        el.result.dontcare_child = node;
    else
        throw std::logic_error("Unexpected case.");
}

template<typename Tag>
bool explored(const ConstraintStackEntry<Tag>& el) noexcept
{
    return el.explored_true_child() && el.explored_dontcare_child();
}

template<typename Tag>
std::optional<StackEntry<Tag>> next_entry(const ConstraintStackEntry<Tag>& el,
                                          const std::vector<std::pair<PreconditionVariant, ygg::IndexList<Tag>>>& sorted_preconditions,
                                          const PreconditionDetails<Tag>& details,
                                          const ::tyr::formalism::planning::Repository& context)
{
    if (!el.explored_true_child())
        return try_create_stack_entry(BaseEntry<Tag> { el.base.depth + 1, el.true_elements }, sorted_preconditions, details, context);
    else if (!el.explored_dontcare_child())
        return try_create_stack_entry(BaseEntry<Tag> { el.base.depth + 1, el.dontcare_elements }, sorted_preconditions, details, context);
    else
        throw std::logic_error("Unexpected case.");
}

template<typename Tag>
void push_result(ConstraintStackEntry<Tag>& el, ygg::Data<Node<Tag>> node)
{
    if (!el.result.true_child && !el.true_elements.empty())
        el.result.true_child = node;
    else if (!el.result.dontcare_child && !el.dontcare_elements.empty())
        el.result.dontcare_child = node;
    else
        throw std::logic_error("Unexpected case.");
}

template<typename Tag>
bool explored(const GeneratorStackEntry<Tag>& el) noexcept
{
    return true;
}

template<typename Tag>
std::optional<StackEntry<Tag>> next_entry(const GeneratorStackEntry<Tag>& el,
                                          const std::vector<std::pair<PreconditionVariant, ygg::IndexList<Tag>>>& sorted_preconditions,
                                          const PreconditionDetails<Tag>& details,
                                          const ::tyr::formalism::planning::Repository& context)
{
    return std::nullopt;
}

template<typename Tag>
void push_result(GeneratorStackEntry<Tag>& el, ygg::Data<Node<Tag>> node)
{
    throw std::logic_error("Unexpected case.");
}

inline auto get_condition(::tyr::formalism::planning::GroundAxiomView el) { return el.get_body(); }

inline auto get_condition(::tyr::formalism::planning::GroundActionView el) { return el.get_condition(); }

template<typename Tag>
static std::optional<StackEntry<Tag>> try_create_atom_stack_entry(ygg::Index<::tyr::formalism::planning::GroundAtom<::tyr::formalism::DerivedTag>> atom,
                                                                  BaseEntry<Tag> base,
                                                                  const PreconditionDetails<Tag>& details)
{
    assert(!base.elements.empty());

    auto category = [&](ygg::Index<Tag> e) -> ygg::uint_t
    {
        if (!details.at(e).contains(atom))
            return 2;  // dontcare

        const auto polarity = std::get<bool>(details.at(e).at(atom));
        return polarity ? 0 : 1;  // true first, then false
    };

    // stable_sort: the comparator has ties, and unstable sorts permute them differently across
    // standard library implementations (libstdc++ vs libc++).
    std::stable_sort(base.elements.begin(),
              base.elements.end(),
              [&](auto&& lhs, auto&& rhs)
              {
                  const auto lhs_cat = category(lhs);
                  const auto rhs_cat = category(rhs);
                  if (lhs_cat != rhs_cat)
                      return lhs_cat < rhs_cat;  // 0 < 1 < 2
                  return lhs < rhs;
              });

    const auto mid1 = std::find_if(base.elements.begin(), base.elements.end(), [&](ygg::Index<Tag> e) { return category(e) >= 1; });
    const auto mid2 = std::find_if(mid1, base.elements.end(), [&](ygg::Index<Tag> e) { return category(e) >= 2; });

    const auto true_elements = std::span<ygg::Index<Tag>>(base.elements.begin(), mid1);
    const auto false_elements = std::span<ygg::Index<Tag>>(mid1, mid2);
    const auto dontcare_elements = std::span<ygg::Index<Tag>>(mid2, base.elements.end());

    if (true_elements.empty() && false_elements.empty())
        return std::nullopt;  ///< no element cares about the atom

    return AtomStackEntry<Tag>(base, atom, true_elements, false_elements, dontcare_elements);
}

template<typename Tag>
static std::optional<StackEntry<Tag>> try_create_variable_stack_entry(ygg::Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> variable,
                                                                      BaseEntry<Tag> base,
                                                                      const PreconditionDetails<Tag>& details,
                                                                      const ::tyr::formalism::planning::Repository& context)
{
    assert(!base.elements.empty());

    const auto domain_size = ygg::make_view(variable, context).get_domain_size();

    auto category = [&](ygg::Index<Tag> e) -> ygg::uint_t
    {
        if (!details.at(e).contains(variable))
            return domain_size;  // dontcare

        const auto value = std::get<::tyr::formalism::planning::FDRValue>(details.at(e).at(variable));
        return ygg::uint_t(value);
    };

    // stable_sort: the comparator has ties, and unstable sorts permute them differently across
    // standard library implementations (libstdc++ vs libc++).
    std::stable_sort(base.elements.begin(),
              base.elements.end(),
              [&](auto&& lhs, auto&& rhs)
              {
                  const auto lhs_cat = category(lhs);
                  const auto rhs_cat = category(rhs);
                  if (lhs_cat != rhs_cat)
                      return lhs_cat < rhs_cat;  // 0 < 1 < ... < domain_size (dontcare)
                  return lhs < rhs;
              });

    auto children_elements = std::vector<std::span<ygg::Index<Tag>>> {};
    children_elements.reserve(domain_size);

    auto it = base.elements.begin();
    for (ygg::uint_t i = 0; i < domain_size; ++i)
    {
        const auto mid = std::find_if(it, base.elements.end(), [&](ygg::Index<Tag> e) { return category(e) > i; });
        children_elements.push_back(std::span<ygg::Index<Tag>>(it, mid));
        it = mid;
    }

    auto forward = std::vector<ygg::uint_t>();
    for (ygg::uint_t i = 0; i < domain_size; ++i)
    {
        if (!children_elements[i].empty())
            forward.push_back(i);
    }

    const auto dontcare_elements = std::span<ygg::Index<Tag>>(it, base.elements.end());

    if (forward.empty())
        return std::nullopt;  ///< no element cares about the atom

    return VariableStackEntry<Tag>(base, variable, children_elements, forward, dontcare_elements);
}

template<typename Tag>
static std::optional<StackEntry<Tag>> try_create_negative_fact_stack_entry(ygg::Data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> fact,
                                                                           BaseEntry<Tag> base,
                                                                           const PreconditionDetails<Tag>& details)
{
    assert(!base.elements.empty());

    // stable_sort: the comparator has ties, and unstable sorts permute them differently across
    // standard library implementations (libstdc++ vs libc++).
    std::stable_sort(base.elements.begin(),
              base.elements.end(),
              [&](auto&& lhs, auto&& rhs)
              {
                  const auto lhs_has = details.at(lhs).contains(fact);
                  const auto rhs_has = details.at(rhs).contains(fact);
                  if (lhs_has == rhs_has)
                      return lhs < rhs;
                  return lhs_has > rhs_has;  // true < dontcare
              });

    const auto mid = std::find_if(base.elements.begin(), base.elements.end(), [&](auto&& e) { return !details.at(e).contains(fact); });

    const auto true_elements = std::span<ygg::Index<Tag>>(base.elements.begin(), mid);
    const auto dontcare_elements = std::span<ygg::Index<Tag>>(mid, base.elements.end());

    if (true_elements.empty())
        return std::nullopt;  ///< no element cares about the constraint

    return NegativeFactStackEntry<Tag>(base, fact, true_elements, dontcare_elements);
}

template<typename Tag>
static std::optional<StackEntry<Tag>> try_create_constraint_stack_entry(
    ygg::Data<::tyr::formalism::planning::BooleanOperator<ygg::Data<::tyr::formalism::planning::GroundFunctionExpression>>> constraint,
    BaseEntry<Tag> base,
    const PreconditionDetails<Tag>& details)
{
    assert(!base.elements.empty());

    // stable_sort: the comparator has ties, and unstable sorts permute them differently across
    // standard library implementations (libstdc++ vs libc++).
    std::stable_sort(base.elements.begin(),
              base.elements.end(),
              [&](auto&& lhs, auto&& rhs)
              {
                  const auto lhs_has = details.at(lhs).contains(constraint);
                  const auto rhs_has = details.at(rhs).contains(constraint);
                  if (lhs_has == rhs_has)
                      return lhs < rhs;
                  return lhs_has > rhs_has;  // true < dontcare
              });

    const auto mid = std::find_if(base.elements.begin(), base.elements.end(), [&](auto&& e) { return !details.at(e).contains(constraint); });

    const auto true_elements = std::span<ygg::Index<Tag>>(base.elements.begin(), mid);
    const auto dontcare_elements = std::span<ygg::Index<Tag>>(mid, base.elements.end());

    if (true_elements.empty())
        return std::nullopt;  ///< no element cares about the constraint

    return ConstraintStackEntry<Tag>(base, constraint, true_elements, dontcare_elements);
}

template<typename Tag>
static StackEntry<Tag> create_generator_stack_entry(BaseEntry<Tag> base)
{
    assert(!base.elements.empty());
    return GeneratorStackEntry(base);
}

template<typename Tag>
static std::optional<StackEntry<Tag>>
try_create_selector_stack_entry(BaseEntry<Tag> base,
                                const std::vector<std::pair<PreconditionVariant, ygg::IndexList<Tag>>>& sorted_preconditions,
                                const PreconditionDetails<Tag>& details,
                                const ::tyr::formalism::planning::Repository& context)
{
    return std::visit(
        [&](auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::same_as<Alternative, ygg::Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>>>)
                return try_create_variable_stack_entry(arg, base, details, context);
            else if constexpr (std::same_as<Alternative, ygg::Data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>>)
                return try_create_negative_fact_stack_entry(arg, base, details);
            else if constexpr (std::same_as<Alternative, ygg::Index<::tyr::formalism::planning::GroundAtom<::tyr::formalism::DerivedTag>>>)
                return try_create_atom_stack_entry(arg, base, details);
            else if constexpr (std::same_as<
                                   Alternative,
                                   ygg::Data<::tyr::formalism::planning::BooleanOperator<ygg::Data<::tyr::formalism::planning::GroundFunctionExpression>>>>)
                return try_create_constraint_stack_entry(arg, base, details);
            else
                static_assert(ygg::dependent_false<Alternative>::value, "Missing case");
        },
        sorted_preconditions[base.depth].first);
}

template<typename Tag>
static std::optional<StackEntry<Tag>> try_create_stack_entry(BaseEntry<Tag> base,
                                                             const std::vector<std::pair<PreconditionVariant, ygg::IndexList<Tag>>>& sorted_preconditions,
                                                             const PreconditionDetails<Tag>& details,
                                                             const ::tyr::formalism::planning::Repository& context)
{
    if (!base.elements.empty())
    {
        for (; base.depth < sorted_preconditions.size(); ++base.depth)
            if (auto entry = try_create_selector_stack_entry(base, sorted_preconditions, details, context))
                return std::move(entry.value());
    }
    else
        return std::nullopt;

    return create_generator_stack_entry(base);
}

template<typename Tag>
MatchTree<Tag>::MatchTree(ygg::IndexList<Tag> elements_, const ::tyr::formalism::planning::Repository& context_) :
    m_elements(std::move(elements_)),
    m_context(std::make_unique<Repository<Tag>>(ygg::uint_t(0), context_)),  // we use constant index 0 since we dont compare node views anyway.
    m_root(),
    m_evaluate_stack()
{
    auto occurrences = PreconditionOccurrences<Tag> {};
    auto details = PreconditionDetails<Tag> {};

    // std::cout << "Num elements: " << m_elements.size() << std::endl;

    for (const auto element : m_elements)
    {
        const auto condition = get_condition(ygg::make_view(element, context_));

        details.try_emplace(element);  //

        for (const auto fact : condition.template get_facts<::tyr::formalism::PositiveTag>())
        {
            const auto key = fact.get_variable().get_index();
            occurrences[key].push_back(element);
            details[element][key] = fact.get_value();
        }

        for (const auto fact : condition.template get_facts<::tyr::formalism::NegativeTag>())
        {
            const auto key = fact.get_data();
            occurrences[key].push_back(element);
            details[element][key] = std::monostate {};
        }

        for (const auto literal : condition.template get_literals<::tyr::formalism::DerivedTag>())
        {
            const auto key = literal.get_atom().get_index();
            occurrences[key].push_back(element);
            details[element][key] = literal.get_polarity();
        }

        for (const auto constraint : condition.get_numeric_constraints())
        {
            const auto key = constraint.get_data();
            occurrences[key].push_back(element);
            details[element][key] = std::monostate {};
        }
    }

    std::vector<std::pair<PreconditionVariant, ygg::IndexList<Tag>>> sorted_preconditions(occurrences.begin(), occurrences.end());

    // Total order (occurrence count, then canonical precondition key): sorted_preconditions is built
    // from unordered-map iteration and feeds the tree construction, so ties must not be broken by the
    // container's iteration order or the standard library's unstable sort.
    const auto precondition_less = [](const PreconditionVariant& lhs, const PreconditionVariant& rhs)
    {
        if (lhs.index() != rhs.index())
            return lhs.index() < rhs.index();
        return std::visit([&](const auto& lhs_alt) { return lhs_alt < std::get<std::decay_t<decltype(lhs_alt)>>(rhs); }, lhs);
    };
    std::sort(sorted_preconditions.begin(),
              sorted_preconditions.end(),
              [&](const auto& a, const auto& b)
              {
                  if (a.second.size() != b.second.size())
                      return a.second.size() > b.second.size();
                  return precondition_less(a.first, b.first);
              });

    // std::cout << details << std::endl;
    // std::cout << sorted_preconditions << std::endl;

    auto stack = std::deque<StackEntry<Tag>> {};
    auto initial_entry =
        try_create_stack_entry(BaseEntry<Tag>(size_t(0), std::span(m_elements.begin(), m_elements.end())), sorted_preconditions, details, context_);
    if (!initial_entry)
        return;

    stack.emplace_back(std::move(initial_entry.value()));

    // iterative post-order dfs
    while (!stack.empty())
    {
        auto& entry = stack.back();

        std::optional<ygg::Data<Node<Tag>>> produced;
        std::optional<StackEntry<Tag>> next;

        std::visit(
            [&](auto& frame)
            {
                if (!explored(frame))
                {
                    // std::cout << "next_entry" << std::endl;
                    next = next_entry(frame, sorted_preconditions, details, context_);
                }
                else
                {
                    const auto view = store_result(frame, *m_context);
                    produced = ygg::Data<Node<Tag>>(view.get_handle());
                }
            },
            entry);

        if (next)
        {
            // std::cout << "push next" << std::endl;
            stack.push_back(std::move(next.value()));
            continue;
        }

        assert(produced);

        stack.pop_back();

        if (stack.empty())
        {
            m_root = ygg::make_view(produced.value(), *m_context);
            break;
        }
        else
        {
            // std::cout << "push result" << std::endl;
            std::visit([&](auto& parent) { push_result(parent, std::move(produced.value())); }, stack.back());
        }
    }

    // std::cout << "Num nodes: " << num_nodes << std::endl;
}

template<typename Tag>
MatchTree<Tag>::~MatchTree() = default;

template<typename Tag>
MatchTreePtr<Tag> MatchTree<Tag>::create(ygg::IndexList<Tag> elements, const ::tyr::formalism::planning::Repository& context)
{
    return std::make_unique<MatchTree<Tag>>(std::move(elements), context);
}

template<typename Tag>
void MatchTree<Tag>::generate(const StateContext<GroundTag>& state,
                              std::vector<ygg::View<ygg::Index<Tag>, ::tyr::formalism::planning::Repository>>& out_applicable_elements)
{
    out_applicable_elements.clear();
    m_evaluate_stack.clear();

    if (m_root)
        m_evaluate_stack.push_back(*m_root);

    while (!m_evaluate_stack.empty())
    {
        const auto node = m_evaluate_stack.back();
        m_evaluate_stack.pop_back();

        visit(
            [&](auto&& arg)
            {
                using Handle = std::decay_t<decltype(arg.get_handle())>;

                if constexpr (std::is_same_v<Handle, ygg::Index<AtomSelectorNode<Tag>>>)
                {
                    const auto holds = state.unpacked_state.test(arg.get_atom());

                    if (const auto child = holds ? arg.get_true_child() : arg.get_false_child())
                        m_evaluate_stack.push_back(*child);

                    if (const auto child = arg.get_dontcare_child())
                        m_evaluate_stack.push_back(*child);
                }
                else if constexpr (std::is_same_v<Handle, ygg::Index<NumericConstraintSelectorNode<Tag>>>)
                {
                    const auto holds = evaluate(arg.get_constraint(), state);

                    if (holds)
                        if (const auto child = arg.get_true_child())
                            m_evaluate_stack.push_back(*child);

                    if (const auto child = arg.get_dontcare_child())
                        m_evaluate_stack.push_back(*child);
                }
                else if constexpr (std::is_same_v<Handle, ygg::Index<VariableSelectorNode<Tag>>>)
                {
                    const auto value = state.unpacked_state.get(arg.get_variable());
                    const auto children = arg.get_domain_children();
                    assert(ygg::uint_t(value) < children.size());

                    if (const auto child = children[ygg::uint_t(value)])
                        m_evaluate_stack.push_back(*child);

                    if (const auto child = arg.get_dontcare_child())
                        m_evaluate_stack.push_back(*child);
                }
                else if constexpr (std::is_same_v<Handle, ygg::Index<NegativeFactSelectorNode<Tag>>>)
                {
                    const auto fact = arg.get_fact();
                    const auto holds = state.unpacked_state.get(fact.get_variable()) != fact.get_value();

                    if (holds)
                        if (const auto child = arg.get_true_child())
                            m_evaluate_stack.push_back(*child);

                    if (const auto child = arg.get_dontcare_child())
                        m_evaluate_stack.push_back(*child);
                }
                else if constexpr (std::is_same_v<Handle, ygg::Index<ElementGeneratorNode<Tag>>>)
                {
                    for (const auto element : arg.get_elements())
                        out_applicable_elements.push_back(element);
                }
                else
                {
                    static_assert(ygg::dependent_false<Handle>::value, "Missing case");
                }
            },
            node.get_variant());
    }

}

template class MatchTree<::tyr::formalism::planning::GroundAction>;
template class MatchTree<::tyr::formalism::planning::GroundAxiom>;

}
