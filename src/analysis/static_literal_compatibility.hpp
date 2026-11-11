/*
 * Copyright (C) 2026 Dominik Drexler
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TYR_SRC_ANALYSIS_STATIC_LITERAL_COMPATIBILITY_HPP_
#define TYR_SRC_ANALYSIS_STATIC_LITERAL_COMPATIBILITY_HPP_

#include "tyr/algorithms/kckp/kckp.hpp"
#include "tyr/analysis/variable_domain.hpp"
#include "tyr/formalism/declarations.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <optional>
#include <urd/bits.hpp>
#include <urd/elements/flat.hpp>
#include <urd/repr/maps/index_map.hpp>
#include <urd/repr/sequences/jagged.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <urd/store.hpp>
#include <vector>
#include <yggdrasil/core/config.hpp>

namespace tyr::analysis::detail
{
/// A parameter term as its number: a ParameterIndex, or a plain integer.
template<typename T>
ygg::uint_t parameter_number(const T& term) noexcept
{
    if constexpr (requires { term.value; })
        return term.value;
    else
        return static_cast<ygg::uint_t>(term);
}

/**
 * Compiles fixed static literals into final per-rule unary and pair compatibility tables.
 * Static facts never change, so the literal mappings can be discarded after this one-time projection. Fluent consistency instead retains rule-to-literal
 * mappings and consults assignment sets shared by all rules, avoiding reconstruction of per-rule compatibility tables after every state change.
 */
class StaticLiteralCompatibility
{
private:
    using Object = urd::index<formalism::Object>;
    /// The bits of a parameter's domain: bit i stands for the domain's i-th object.
    using Bitset = urd::Store<urd::set<size_t>, urd::dense>;

    class BinaryCompatibility
    {
    public:
        BinaryCompatibility() = default;
        BinaryCompatibility(size_t first_size, size_t second_size) : m_first_size(first_size), m_second_size(second_size) {}

        [[nodiscard]] bool constrained() const noexcept { return m_constrained; }

        void constrain()
        {
            if (!constrained())
            {
                m_constrained = true;
                m_rows = Rows(std::vector<size_t>(m_first_size, m_second_size));
            }
        }

        void set(size_t first, size_t second)
        {
            assert(constrained());
            m_rows[first].set(second);
        }

        [[nodiscard]] bool test(size_t first, size_t second) const noexcept { return !constrained() || m_rows[first].test(second); }

        [[nodiscard]] size_t count() const noexcept
        {
            auto result = size_t { 0 };
            for (size_t first = 0; first < m_first_size; ++first)
                result += m_rows[first].count();
            return result;
        }

        void intersect(const BinaryCompatibility& support)
        {
            assert(support.constrained());
            if (!constrained())
            {
                m_constrained = true;
                m_rows = support.m_rows;
                return;
            }

            for (size_t first = 0; first < m_first_size; ++first)
                m_rows[first] &= support.m_rows[first];
        }

        void subtract(const BinaryCompatibility& support)
        {
            assert(support.constrained());
            if (!constrained())
            {
                constrain();
                for (size_t first = 0; first < m_first_size; ++first)
                    m_rows[first].set();
            }

            for (size_t first = 0; first < m_first_size; ++first)
                m_rows[first] -= support.m_rows[first];
        }

        template<typename Callback>
        void for_each(Callback callback) const
        {
            assert(constrained());
            for (size_t first = 0; first < m_first_size; ++first)
                for (const auto second : urd::set_bit_indices(m_rows[first]))
                    callback(first, second);
        }

    private:
        /// m_first_size rows of m_second_size bits once constrained, no rows before.
        using Rows = urd::Store<urd::sequence<urd::Store<urd::set<size_t>, urd::dense>>, urd::fixed_shape>;

        size_t m_first_size { 0 };
        size_t m_second_size { 0 };
        bool m_constrained { false };
        Rows m_rows;
    };

    static size_t num_pairs(size_t arity) noexcept { return arity < 2 ? 0 : arity * (arity - 1) / 2; }

    static size_t pair_index(size_t arity, ygg::uint_t first, ygg::uint_t second) noexcept
    {
        assert(first < second && second < arity);
        return static_cast<size_t>(first) * (2 * arity - static_cast<size_t>(first) - 1) / 2 + static_cast<size_t>(second - first - 1);
    }

    static urd::Store<urd::sequence<BinaryCompatibility>> make_binary_compatibilities(const VariableDomainList& domains)
    {
        auto result = urd::Store<urd::sequence<BinaryCompatibility>> {};
        result.reserve(num_pairs(domains.size()));
        for (size_t first = 0; first < domains.size(); ++first)
            for (size_t second = first + 1; second < domains.size(); ++second)
                result.emplace_back(domains[first].objects.size(), domains[second].objects.size());
        return result;
    }

    /// The bit of object in the domain of parameter, if the domain has the object.
    [[nodiscard]] std::optional<size_t> object_bit(ygg::uint_t parameter, Object object) const noexcept
    {
        if (parameter >= m_object_to_bit.size())
            return std::nullopt;

        const auto& bits = m_object_to_bit[parameter];
        const auto bit = bits.find(object);
        return bit == bits.end() ? std::nullopt : std::optional<size_t>(bit->second);
    }

public:
    template<typename LiteralRange, typename AtomRange>
    StaticLiteralCompatibility(LiteralRange literals, AtomRange atoms, const VariableDomainList& domains, size_t num_objects) :

        m_domains(domains),
        m_object_to_bit(domains.size()),
        m_unary_compatible(domains.size()),
        m_binary_compatible(make_binary_compatibilities(domains))
    {
        const auto arity = domains.size();
        for (size_t parameter = 0; parameter < arity; ++parameter)
            for (size_t bit = 0; bit < domains[parameter].objects.size(); ++bit)
            {
                m_unary_compatible[parameter].insert(bit);
                const auto object = domains[parameter].objects[bit];
                const auto object_index = static_cast<size_t>(static_cast<ygg::uint_t>(object.value));
                assert(object_index < num_objects);
                if (object_index < num_objects)
                {
                    assert(!m_object_to_bit[parameter].contains(object));
                    m_object_to_bit[parameter].insert_or_assign(object, bit);
                }
            }

        for (const auto& literal : literals)
        {
            const auto& terms = literal.get_atom().get_terms();
            auto parameters = urd::Store<urd::sequence<ygg::uint_t>> {};
            parameters.reserve(terms.size());
            for (const auto term : terms)
                std::visit(
                    [&](auto value)
                    {
                        if constexpr (!requires { value.index(); })
                        {
                            const auto parameter = parameter_number(value);
                            assert(parameter < arity);
                            parameters.push_back(parameter);
                        }
                    },
                    term);

            std::sort(parameters.begin(), parameters.end());
            parameters.erase(std::unique(parameters.begin(), parameters.end()), parameters.end());

            auto unary_supports = urd::Store<urd::sequence<Bitset>>(arity);
            auto binary_supports = make_binary_compatibilities(domains);
            for (size_t first = 0; first < parameters.size(); ++first)
                for (size_t second = first + 1; second < parameters.size(); ++second)
                    binary_supports[pair_index(arity, parameters[first], parameters[second])].constrain();

            auto assignment = urd::Store<urd::sequence<Object>>(arity);
            auto assigned = urd::Store<urd::set<ygg::uint_t>, urd::dense> {};
            auto has_matching_atom = false;
            const auto predicate = literal.get_atom().get_predicate().index();
            for (const auto& atom : atoms)
            {
                if (atom.get_predicate().index() != predicate)
                    continue;

                assigned.clear();
                auto matches = true;
                auto position = size_t { 0 };
                for (const auto object : atom.get_objects())
                {
                    if (position == terms.size())
                    {
                        matches = false;
                        break;
                    }

                    const auto object_index = object.index();
                    std::visit(
                        [&](auto value)
                        {
                            if constexpr (requires { value.index(); })
                            {
                                matches = matches && value.index() == object_index;
                            }
                            else
                            {
                                const auto parameter = parameter_number(value);
                                if (assigned.contains(parameter))
                                    matches = matches && assignment[parameter] == object_index;
                                else
                                {
                                    assignment[parameter] = object_index;
                                    assigned.insert(parameter);
                                }
                            }
                        },
                        terms[position++]);
                }
                if (!matches || position != terms.size())
                    continue;

                has_matching_atom = true;
                for (const auto parameter : parameters)
                    if (const auto bit = object_bit(parameter, assignment[parameter]))
                        unary_supports[parameter].insert(*bit);
                for (size_t first = 0; first < parameters.size(); ++first)
                    for (size_t second = first + 1; second < parameters.size(); ++second)
                    {
                        const auto first_parameter = parameters[first];
                        const auto second_parameter = parameters[second];
                        const auto first_bit = object_bit(first_parameter, assignment[first_parameter]);
                        const auto second_bit = object_bit(second_parameter, assignment[second_parameter]);
                        if (first_bit && second_bit)
                            binary_supports[pair_index(arity, first_parameter, second_parameter)].set(*first_bit, *second_bit);
                    }
            }

            if (parameters.empty())
            {
                if (literal.get_polarity() != has_matching_atom)
                    m_satisfiable = false;
            }
            else if (literal.get_polarity())
            {
                for (const auto parameter : parameters)
                    m_unary_compatible[parameter] &= unary_supports[parameter];
                for (size_t first = 0; first < parameters.size(); ++first)
                    for (size_t second = first + 1; second < parameters.size(); ++second)
                    {
                        const auto first_parameter = parameters[first];
                        const auto second_parameter = parameters[second];
                        const auto index = pair_index(arity, first_parameter, second_parameter);
                        m_binary_compatible[index].intersect(binary_supports[index]);
                    }
            }
            else if (parameters.size() == 1)
            {
                m_unary_compatible[parameters.front()] -= unary_supports[parameters.front()];
            }
            else if (parameters.size() == 2)
            {
                const auto index = pair_index(arity, parameters[0], parameters[1]);
                m_binary_compatible[index].subtract(binary_supports[index]);
            }
        }
    }

    [[nodiscard]] bool is_satisfiable() const noexcept { return m_satisfiable; }

    [[nodiscard]] bool is_binary_constrained(ygg::uint_t first_parameter, ygg::uint_t second_parameter) const noexcept
    {
        if (first_parameter >= m_unary_compatible.size() || second_parameter >= m_unary_compatible.size() || first_parameter == second_parameter)
            return false;
        if (first_parameter > second_parameter)
            std::swap(first_parameter, second_parameter);
        return m_binary_compatible[pair_index(m_unary_compatible.size(), first_parameter, second_parameter)].constrained();
    }

    [[nodiscard]] size_t num_binary_compatible(ygg::uint_t first_parameter, ygg::uint_t second_parameter) const noexcept
    {
        assert(first_parameter < second_parameter);
        return m_binary_compatible[pair_index(m_unary_compatible.size(), first_parameter, second_parameter)].count();
    }

    template<typename Callback>
    void for_each_binary_compatible(ygg::uint_t first_parameter, ygg::uint_t second_parameter, Callback callback) const
    {
        assert(first_parameter < second_parameter);
        const auto& compatible = m_binary_compatible[pair_index(m_unary_compatible.size(), first_parameter, second_parameter)];
        compatible.for_each([&](const auto first, const auto second)
                            { callback(m_domains[first_parameter].objects[first], m_domains[second_parameter].objects[second]); });
    }

    [[nodiscard]] bool unary_compatible(ygg::uint_t parameter, Object object) const noexcept
    {
        if (parameter >= m_unary_compatible.size())
            return true;

        const auto bit = object_bit(parameter, object);
        return bit && m_unary_compatible[parameter].contains(*bit);
    }

    [[nodiscard]] bool binary_compatible(ygg::uint_t first_parameter, Object first_object, ygg::uint_t second_parameter, Object second_object) const noexcept
    {
        if (first_parameter >= m_unary_compatible.size() || second_parameter >= m_unary_compatible.size())
            return true;

        if (first_parameter > second_parameter)
        {
            std::swap(first_parameter, second_parameter);
            std::swap(first_object, second_object);
        }
        if (first_parameter == second_parameter)
            return false;

        const auto first_bit = object_bit(first_parameter, first_object);
        const auto second_bit = object_bit(second_parameter, second_object);
        if (!first_bit || !second_bit)
            return false;

        return m_binary_compatible[pair_index(m_unary_compatible.size(), first_parameter, second_parameter)].test(*first_bit, *second_bit);
    }

private:
    bool m_satisfiable = true;
    VariableDomainList m_domains;
    urd::Store<urd::sequence<urd::Store<urd::map<Object, size_t>, urd::dense>>> m_object_to_bit;
    urd::Store<urd::sequence<Bitset>> m_unary_compatible;
    urd::Store<urd::sequence<BinaryCompatibility>> m_binary_compatible;
};

struct PairwiseCompatibilityResult
{
    VariableDomainList domains;
    kckp::Graph graph;
};

inline PairwiseCompatibilityResult create_pairwise_compatibility_graph(const VariableDomainList& initial_domains,
                                                                       size_t num_objects,
                                                                       std::initializer_list<const StaticLiteralCompatibility*> compatibilities)
{
    auto domains = VariableDomainList(initial_domains.size());
    for (ygg::uint_t partition = 0; partition < initial_domains.size(); ++partition)
        for (const auto object : initial_domains[partition].objects)
            if (std::ranges::all_of(compatibilities, [&](const auto* compatibility) { return compatibility->unary_compatible(partition, object); }))
                domains[partition].objects.push_back(object);

    auto partition_sizes = urd::Store<urd::sequence<size_t>> {};
    partition_sizes.reserve(domains.size());
    auto vertex_labels = urd::Store<urd::sequence<ygg::uint_t>> {};
    for (const auto& domain : domains)
    {
        partition_sizes.push_back(domain.objects.size());
        for (const auto object : domain.objects)
            vertex_labels.push_back(static_cast<ygg::uint_t>(object.value));
    }

    const auto layout = kckp::create_graph_layout(partition_sizes, vertex_labels);
    auto adjacency = kckp::AdjacencyMatrix(layout);
    auto universal_partition_pairs = urd::Store<urd::set<size_t>, urd::dense> {};
    auto object_to_bit = urd::Store<urd::sequence<urd::Store<urd::map<urd::index<formalism::Object>, ygg::uint_t>, urd::dense>>>(layout.num_partitions);
    for (ygg::uint_t partition = 0; partition < layout.num_partitions; ++partition)
    {
        object_to_bit[partition].reserve(num_objects);
        for (ygg::uint_t bit = 0; bit < domains[partition].objects.size(); ++bit)
            object_to_bit[partition].insert_or_assign(domains[partition].objects[bit], bit);
    }

    for (ygg::uint_t first_partition = 0; first_partition < layout.num_partitions; ++first_partition)
        for (ygg::uint_t second_partition = first_partition + 1; second_partition < layout.num_partitions; ++second_partition)
        {
            const StaticLiteralCompatibility* seed = nullptr;
            for (const auto* compatibility : compatibilities)
                if (compatibility->is_binary_constrained(first_partition, second_partition)
                    && (!seed
                        || compatibility->num_binary_compatible(first_partition, second_partition)
                               < seed->num_binary_compatible(first_partition, second_partition)))
                    seed = compatibility;

            if (!seed)
            {
                universal_partition_pairs.insert(static_cast<size_t>(first_partition) * layout.num_partitions + second_partition);
                universal_partition_pairs.insert(static_cast<size_t>(second_partition) * layout.num_partitions + first_partition);
                continue;
            }

            const auto& first_bits = object_to_bit[first_partition];
            const auto& second_bits = object_to_bit[second_partition];
            auto num_compatible = size_t { 0 };
            seed->for_each_binary_compatible(
                first_partition,
                second_partition,
                [&](const auto first_object, const auto second_object)
                {
                    const auto first_bit = first_bits.find(first_object);
                    const auto second_bit = second_bits.find(second_object);
                    if (first_bit == first_bits.end() || second_bit == second_bits.end()
                        || !std::ranges::all_of(compatibilities,
                                                [&](const auto* compatibility)
                                                { return compatibility->binary_compatible(first_partition, first_object, second_partition, second_object); }))
                        return;

                    const auto first_vertex = layout.info.infos[first_partition].bit_offset + first_bit->second;
                    const auto second_vertex = layout.info.infos[second_partition].bit_offset + second_bit->second;
                    adjacency.get_bitset(first_vertex, second_partition).set(second_bit->second);
                    adjacency.get_bitset(second_vertex, first_partition).set(first_bit->second);
                    ++num_compatible;
                });

            if (num_compatible == domains[first_partition].objects.size() * domains[second_partition].objects.size())
            {
                universal_partition_pairs.insert(static_cast<size_t>(first_partition) * layout.num_partitions + second_partition);
                universal_partition_pairs.insert(static_cast<size_t>(second_partition) * layout.num_partitions + first_partition);
            }
        }

    const auto satisfiable = std::ranges::all_of(compatibilities, [](const auto* compatibility) { return compatibility->is_satisfiable(); });
    auto graph = kckp::Graph::create(satisfiable, std::move(adjacency), universal_partition_pairs);
    auto refined_domains = VariableDomainList(graph.get_layout().num_partitions);
    for (ygg::uint_t partition = 0; partition < graph.get_layout().num_partitions; ++partition)
        for (const auto vertex : graph.get_layout().vertex_partitions[partition])
            refined_domains[partition].objects.emplace_back(graph.get_layout().vertex_labels[vertex]);

    return PairwiseCompatibilityResult { std::move(refined_domains), std::move(graph) };
}

}

#endif
