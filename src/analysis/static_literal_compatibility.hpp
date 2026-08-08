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
#include "tyr/formalism/object_index.hpp"

#include <algorithm>
#include <boost/dynamic_bitset.hpp>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <limits>
#include <vector>
#include <yggdrasil/containers/variant.hpp>
#include <yggdrasil/core/types.hpp>

namespace tyr::analysis::detail
{

/**
 * Compiles fixed static literals into final per-rule unary and pair compatibility tables.
 * Static facts never change, so the literal mappings can be discarded after this one-time projection. Fluent consistency instead retains rule-to-literal
 * mappings and consults assignment sets shared by all rules, avoiding reconstruction of per-rule compatibility tables after every state change.
 */
class StaticLiteralCompatibility
{
private:
    using Object = ygg::Index<::tyr::formalism::Object>;
    using Bitset = boost::dynamic_bitset<>;

    static constexpr size_t invalid_bit = std::numeric_limits<size_t>::max();

    class BinaryCompatibility
    {
    public:
        BinaryCompatibility() = default;
        BinaryCompatibility(size_t first_size, size_t second_size) : m_first_size(first_size), m_second_size(second_size) {}

        bool constrained() const noexcept { return m_constrained; }

        void constrain()
        {
            if (!constrained())
            {
                m_constrained = true;
                m_rows.resize(m_first_size * ygg::BitsetSpan<uint64_t>::num_blocks(m_second_size));
            }
        }

        auto get_row(size_t first)
        {
            const auto num_blocks = ygg::BitsetSpan<uint64_t>::num_blocks(m_second_size);
            return ygg::BitsetSpan<uint64_t>(num_blocks == 0 ? m_rows.data() : m_rows.data() + first * num_blocks, m_second_size);
        }

        auto get_row(size_t first) const
        {
            const auto num_blocks = ygg::BitsetSpan<uint64_t>::num_blocks(m_second_size);
            return ygg::BitsetSpan<const uint64_t>(num_blocks == 0 ? m_rows.data() : m_rows.data() + first * num_blocks, m_second_size);
        }

        void set(size_t first, size_t second)
        {
            assert(constrained());
            get_row(first).set(second);
        }

        bool test(size_t first, size_t second) const noexcept { return !constrained() || get_row(first).test(second); }

        size_t count() const noexcept
        {
            auto result = size_t { 0 };
            for (size_t first = 0; first < m_first_size; ++first)
                result += get_row(first).count();
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
                get_row(first) &= support.get_row(first);
        }

        void subtract(const BinaryCompatibility& support)
        {
            assert(support.constrained());
            if (!constrained())
            {
                constrain();
                for (size_t first = 0; first < m_first_size; ++first)
                    get_row(first).set();
            }

            for (size_t first = 0; first < m_first_size; ++first)
                get_row(first) -= support.get_row(first);
        }

        template<typename Callback>
        void for_each(Callback&& callback) const
        {
            assert(constrained());
            for (size_t first = 0; first < m_first_size; ++first)
            {
                const auto row = get_row(first);
                for (auto second = row.find_first(); second != ygg::BitsetSpan<const uint64_t>::npos; second = row.find_next(second))
                    callback(first, second);
            }
        }

    private:
        size_t m_first_size { 0 };
        size_t m_second_size { 0 };
        bool m_constrained { false };
        std::vector<uint64_t> m_rows;
    };

    static size_t num_pairs(size_t arity) noexcept { return arity < 2 ? 0 : arity * (arity - 1) / 2; }

    static size_t pair_index(size_t arity, ygg::uint_t first, ygg::uint_t second) noexcept
    {
        assert(first < second && second < arity);
        return size_t(first) * (2 * arity - size_t(first) - 1) / 2 + size_t(second - first - 1);
    }

    static std::vector<BinaryCompatibility> make_binary_compatibilities(const VariableDomainList& domains)
    {
        auto result = std::vector<BinaryCompatibility> {};
        result.reserve(num_pairs(domains.size()));
        for (size_t first = 0; first < domains.size(); ++first)
            for (size_t second = first + 1; second < domains.size(); ++second)
                result.emplace_back(domains[first].objects.size(), domains[second].objects.size());
        return result;
    }

    size_t object_bit(ygg::uint_t parameter, Object object) const noexcept
    {
        if (parameter >= m_object_to_bit.size())
            return invalid_bit;

        const auto object_index = size_t(ygg::uint_t(object));
        return object_index < m_object_to_bit[parameter].size() ? m_object_to_bit[parameter][object_index] : invalid_bit;
    }

public:
    template<typename LiteralRange, typename AtomRange>
    StaticLiteralCompatibility(LiteralRange literals, AtomRange atoms, const VariableDomainList& domains, size_t num_objects) :
        m_satisfiable(true),
        m_domains(domains),
        m_object_to_bit(domains.size(), std::vector<size_t>(num_objects, invalid_bit)),
        m_unary_compatible(domains.size()),
        m_binary_compatible(make_binary_compatibilities(domains))
    {
        const auto arity = domains.size();
        for (size_t parameter = 0; parameter < arity; ++parameter)
        {
            m_unary_compatible[parameter].resize(domains[parameter].objects.size());
            m_unary_compatible[parameter].set();
            for (size_t bit = 0; bit < domains[parameter].objects.size(); ++bit)
            {
                const auto object = size_t(ygg::uint_t(domains[parameter].objects[bit]));
                assert(object < num_objects);
                if (object < num_objects)
                {
                    assert(m_object_to_bit[parameter][object] == invalid_bit);
                    m_object_to_bit[parameter][object] = bit;
                }
            }
        }

        for (const auto& literal : literals)
        {
            const auto& terms = literal.get_atom().get_terms();
            auto parameters = std::vector<ygg::uint_t> {};
            parameters.reserve(terms.size());
            for (const auto term : terms)
                visit(
                    [&](auto value)
                    {
                        if constexpr (!requires { value.get_index(); })
                        {
                            const auto parameter = ygg::uint_t(value);
                            assert(parameter < arity);
                            parameters.push_back(parameter);
                        }
                    },
                    term.get_variant());

            std::sort(parameters.begin(), parameters.end());
            parameters.erase(std::unique(parameters.begin(), parameters.end()), parameters.end());

            auto unary_supports = std::vector<Bitset>(arity);
            auto binary_supports = make_binary_compatibilities(domains);
            for (const auto parameter : parameters)
                unary_supports[parameter].resize(domains[parameter].objects.size());
            for (size_t first = 0; first < parameters.size(); ++first)
                for (size_t second = first + 1; second < parameters.size(); ++second)
                    binary_supports[pair_index(arity, parameters[first], parameters[second])].constrain();

            auto assignment = std::vector<Object>(arity);
            auto assigned = std::vector<bool>(arity);
            auto has_matching_atom = false;
            for (const auto& atom : atoms)
            {
                if (atom.get_predicate().get_index() != literal.get_atom().get_predicate().get_index())
                    continue;

                std::fill(assigned.begin(), assigned.end(), false);
                auto matches = true;
                auto position = size_t { 0 };
                for (const auto object : atom.get_objects())
                {
                    if (position == terms.size())
                    {
                        matches = false;
                        break;
                    }

                    const auto object_index = object.get_index();
                    visit(
                        [&](auto value)
                        {
                            if constexpr (requires { value.get_index(); })
                            {
                                matches = matches && value.get_index() == object_index;
                            }
                            else
                            {
                                const auto parameter = ygg::uint_t(value);
                                if (assigned[parameter])
                                    matches = matches && assignment[parameter] == object_index;
                                else
                                {
                                    assignment[parameter] = object_index;
                                    assigned[parameter] = true;
                                }
                            }
                        },
                        terms[position++].get_variant());
                }
                if (!matches || position != terms.size())
                    continue;

                has_matching_atom = true;
                for (const auto parameter : parameters)
                {
                    const auto bit = object_bit(parameter, assignment[parameter]);
                    if (bit != invalid_bit)
                        unary_supports[parameter].set(bit);
                }
                for (size_t first = 0; first < parameters.size(); ++first)
                    for (size_t second = first + 1; second < parameters.size(); ++second)
                    {
                        const auto first_parameter = parameters[first];
                        const auto second_parameter = parameters[second];
                        const auto first_bit = object_bit(first_parameter, assignment[first_parameter]);
                        const auto second_bit = object_bit(second_parameter, assignment[second_parameter]);
                        if (first_bit != invalid_bit && second_bit != invalid_bit)
                            binary_supports[pair_index(arity, first_parameter, second_parameter)].set(first_bit, second_bit);
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

    bool is_satisfiable() const noexcept { return m_satisfiable; }

    bool is_binary_constrained(ygg::uint_t first_parameter, ygg::uint_t second_parameter) const noexcept
    {
        if (first_parameter >= m_unary_compatible.size() || second_parameter >= m_unary_compatible.size() || first_parameter == second_parameter)
            return false;
        if (first_parameter > second_parameter)
            std::swap(first_parameter, second_parameter);
        return m_binary_compatible[pair_index(m_unary_compatible.size(), first_parameter, second_parameter)].constrained();
    }

    size_t num_binary_compatible(ygg::uint_t first_parameter, ygg::uint_t second_parameter) const noexcept
    {
        assert(first_parameter < second_parameter);
        return m_binary_compatible[pair_index(m_unary_compatible.size(), first_parameter, second_parameter)].count();
    }

    template<typename Callback>
    void for_each_binary_compatible(ygg::uint_t first_parameter, ygg::uint_t second_parameter, Callback&& callback) const
    {
        assert(first_parameter < second_parameter);
        const auto& compatible = m_binary_compatible[pair_index(m_unary_compatible.size(), first_parameter, second_parameter)];
        compatible.for_each([&](const auto first, const auto second)
                            { callback(m_domains[first_parameter].objects[first], m_domains[second_parameter].objects[second]); });
    }

    bool unary_compatible(ygg::uint_t parameter, Object object) const noexcept
    {
        if (parameter >= m_unary_compatible.size())
            return true;

        const auto bit = object_bit(parameter, object);
        return bit != invalid_bit && m_unary_compatible[parameter].test(bit);
    }

    bool binary_compatible(ygg::uint_t first_parameter, Object first_object, ygg::uint_t second_parameter, Object second_object) const noexcept
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
        if (first_bit == invalid_bit || second_bit == invalid_bit)
            return false;

        return m_binary_compatible[pair_index(m_unary_compatible.size(), first_parameter, second_parameter)].test(first_bit, second_bit);
    }

private:
    bool m_satisfiable;
    VariableDomainList m_domains;
    std::vector<std::vector<size_t>> m_object_to_bit;
    std::vector<Bitset> m_unary_compatible;
    std::vector<BinaryCompatibility> m_binary_compatible;
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

    auto partition_sizes = std::vector<size_t> {};
    partition_sizes.reserve(domains.size());
    auto vertex_labels = std::vector<ygg::uint_t> {};
    for (const auto& domain : domains)
    {
        partition_sizes.push_back(domain.objects.size());
        for (const auto object : domain.objects)
            vertex_labels.push_back(ygg::uint_t(object));
    }

    const auto layout = kckp::create_graph_layout(partition_sizes, vertex_labels);
    auto adjacency = kckp::AdjacencyMatrix(layout);
    auto universal_partition_pairs = boost::dynamic_bitset<>(layout.num_partitions * layout.num_partitions);
    auto object_to_bit =
        std::vector<std::vector<ygg::uint_t>>(layout.num_partitions, std::vector<ygg::uint_t>(num_objects, std::numeric_limits<ygg::uint_t>::max()));
    for (ygg::uint_t partition = 0; partition < layout.num_partitions; ++partition)
        for (ygg::uint_t bit = 0; bit < domains[partition].objects.size(); ++bit)
            object_to_bit[partition][ygg::uint_t(domains[partition].objects[bit])] = bit;

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
                universal_partition_pairs.set(size_t(first_partition) * layout.num_partitions + second_partition);
                universal_partition_pairs.set(size_t(second_partition) * layout.num_partitions + first_partition);
                continue;
            }

            auto num_compatible = size_t { 0 };
            seed->for_each_binary_compatible(
                first_partition,
                second_partition,
                [&](const auto first_object, const auto second_object)
                {
                    const auto first_bit = object_to_bit[first_partition][ygg::uint_t(first_object)];
                    const auto second_bit = object_to_bit[second_partition][ygg::uint_t(second_object)];
                    if (first_bit == std::numeric_limits<ygg::uint_t>::max() || second_bit == std::numeric_limits<ygg::uint_t>::max()
                        || !std::ranges::all_of(compatibilities,
                                                [&](const auto* compatibility)
                                                { return compatibility->binary_compatible(first_partition, first_object, second_partition, second_object); }))
                        return;

                    const auto first_vertex = layout.info.infos[first_partition].bit_offset + first_bit;
                    const auto second_vertex = layout.info.infos[second_partition].bit_offset + second_bit;
                    adjacency.get_bitset(first_vertex, second_partition).set(second_bit);
                    adjacency.get_bitset(second_vertex, first_partition).set(first_bit);
                    ++num_compatible;
                });

            if (num_compatible == domains[first_partition].objects.size() * domains[second_partition].objects.size())
            {
                universal_partition_pairs.set(size_t(first_partition) * layout.num_partitions + second_partition);
                universal_partition_pairs.set(size_t(second_partition) * layout.num_partitions + first_partition);
            }
        }

    const auto satisfiable = std::ranges::all_of(compatibilities, [](const auto* compatibility) { return compatibility->is_satisfiable(); });
    auto graph = kckp::Graph::create(satisfiable, std::move(adjacency), std::move(universal_partition_pairs));
    auto refined_domains = VariableDomainList(graph.get_layout().num_partitions);
    for (ygg::uint_t partition = 0; partition < graph.get_layout().num_partitions; ++partition)
        for (const auto vertex : graph.get_layout().vertex_partitions[partition])
            refined_domains[partition].objects.emplace_back(graph.get_layout().vertex_labels[vertex]);

    return PairwiseCompatibilityResult { std::move(refined_domains), std::move(graph) };
}

}

#endif
