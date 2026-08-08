#include "tyr/algorithms/kckp/kckp.hpp"

#include "analysis/static_literal_compatibility.hpp"

#include <gtest/gtest.h>
#include <initializer_list>
#include <span>
#include <utility>
#include <variant>
#include <vector>

namespace a = tyr::analysis;
namespace f = tyr::formalism;
namespace k = tyr::kckp;

namespace tyr::tests
{
namespace
{
using Object = ygg::Index<f::Object>;
using Extension = std::vector<ygg::uint_t>;
using Extensions = std::vector<Extension>;

struct FakeObjectView
{
    Object index;

    Object get_index() const noexcept { return index; }
};

struct FakePredicateView
{
    ygg::uint_t index;

    ygg::uint_t get_index() const noexcept { return index; }
};

struct FakeTermView
{
    std::variant<FakeObjectView, ygg::uint_t> variant;

    const auto& get_variant() const noexcept { return variant; }
};

struct FakeAtomView
{
    FakePredicateView predicate;
    std::vector<FakeTermView> terms;

    FakePredicateView get_predicate() const noexcept { return predicate; }
    const auto& get_terms() const noexcept { return terms; }
};

struct FakeLiteralView
{
    bool polarity;
    FakeAtomView atom;

    bool get_polarity() const noexcept { return polarity; }
    const FakeAtomView& get_atom() const noexcept { return atom; }
};

struct FakeGroundAtomView
{
    FakePredicateView predicate;
    std::vector<FakeObjectView> objects;

    FakePredicateView get_predicate() const noexcept { return predicate; }
    const auto& get_objects() const noexcept { return objects; }
};

FakeTermView parameter(ygg::uint_t index) { return FakeTermView { index }; }
FakeTermView constant(ygg::uint_t index) { return FakeTermView { FakeObjectView { Object(index) } }; }

FakeLiteralView literal(bool polarity, ygg::uint_t predicate, std::initializer_list<FakeTermView> terms)
{
    return FakeLiteralView { polarity, FakeAtomView { FakePredicateView { predicate }, terms } };
}

FakeGroundAtomView atom(ygg::uint_t predicate, std::initializer_list<ygg::uint_t> objects)
{
    auto result = FakeGroundAtomView { FakePredicateView { predicate }, {} };
    for (const auto object : objects)
        result.objects.push_back(FakeObjectView { Object(object) });
    return result;
}

a::VariableDomain domain(std::initializer_list<ygg::uint_t> values)
{
    auto result = a::VariableDomain {};
    for (const auto value : values)
        result.objects.emplace_back(value);
    return result;
}

class LabeledGraph
{
public:
    explicit LabeledGraph(k::Graph graph) : m_graph(std::move(graph)), m_domains(m_graph.get_layout().num_partitions)
    {
        const auto& layout = m_graph.get_layout();
        for (ygg::uint_t partition = 0; partition < layout.num_partitions; ++partition)
        {
            const auto& info = layout.info.infos[partition];
            auto& objects = m_domains[partition].objects;
            objects.reserve(info.num_bits);
            for (ygg::uint_t bit = 0; bit < info.num_bits; ++bit)
                objects.push_back(get_object(k::Vertex(info.bit_offset + bit)));
        }
    }

    const k::Graph& get_graph() const noexcept { return m_graph; }
    bool is_satisfiable() const noexcept { return m_graph.is_satisfiable(); }
    const a::VariableDomainList& get_domains() const noexcept { return m_domains; }

    Object get_object(k::Vertex vertex) const noexcept { return Object(m_graph.get_layout().vertex_labels[vertex.index]); }

    k::Vertex find_vertex(ygg::uint_t partition, Object object) const noexcept
    {
        const auto& layout = m_graph.get_layout();
        if (partition >= layout.num_partitions)
            return {};

        const auto& info = layout.info.infos[partition];
        for (ygg::uint_t bit = 0; bit < info.num_bits; ++bit)
        {
            const auto vertex = k::Vertex(info.bit_offset + bit);
            if (get_object(vertex) == object)
                return vertex;
        }
        return {};
    }

private:
    k::Graph m_graph;
    a::VariableDomainList m_domains;
};

template<typename BinaryCompatible>
LabeledGraph make_graph(a::VariableDomainList domains, bool satisfiable, BinaryCompatible&& binary_compatible)
{
    auto partition_sizes = std::vector<size_t> {};
    auto vertex_labels = std::vector<ygg::uint_t> {};
    partition_sizes.reserve(domains.size());
    for (const auto& variable_domain : domains)
    {
        partition_sizes.push_back(variable_domain.objects.size());
        for (const auto object : variable_domain.objects)
            vertex_labels.push_back(ygg::uint_t(object));
    }

    const auto layout = k::create_graph_layout(partition_sizes, vertex_labels);
    auto adjacency = k::AdjacencyMatrix(layout);
    auto universal_partition_pairs = boost::dynamic_bitset<>(layout.num_partitions * layout.num_partitions);

    for (ygg::uint_t first_partition = 0; first_partition < layout.num_partitions; ++first_partition)
        for (ygg::uint_t second_partition = first_partition + 1; second_partition < layout.num_partitions; ++second_partition)
        {
            const auto& first_info = layout.info.infos[first_partition];
            const auto& second_info = layout.info.infos[second_partition];
            auto universal = true;
            for (ygg::uint_t first_bit = 0; first_bit < first_info.num_bits; ++first_bit)
                for (ygg::uint_t second_bit = 0; second_bit < second_info.num_bits; ++second_bit)
                {
                    if (!binary_compatible(first_partition,
                                           domains[first_partition].objects[first_bit],
                                           second_partition,
                                           domains[second_partition].objects[second_bit]))
                    {
                        universal = false;
                        continue;
                    }

                    const auto first_vertex = first_info.bit_offset + first_bit;
                    const auto second_vertex = second_info.bit_offset + second_bit;
                    adjacency.get_bitset(first_vertex, second_partition).set(second_bit);
                    adjacency.get_bitset(second_vertex, first_partition).set(first_bit);
                }

            if (universal)
            {
                universal_partition_pairs.set(size_t(first_partition) * layout.num_partitions + second_partition);
                universal_partition_pairs.set(size_t(second_partition) * layout.num_partitions + first_partition);
            }
        }

    return LabeledGraph(k::Graph::create(satisfiable, std::move(adjacency), std::move(universal_partition_pairs)));
}

Extensions enumerate(const LabeledGraph& graph, std::initializer_list<ygg::uint_t> prefix_values, k::Workspace& workspace)
{
    auto prefix = std::vector<k::Vertex> {};
    auto partition = ygg::uint_t { 0 };
    for (const auto value : prefix_values)
        prefix.push_back(graph.find_vertex(partition++, Object(value)));

    auto result = Extensions {};
    k::KCKP(graph.get_graph())
        .for_each_compatible_extension(std::span<const k::Vertex>(prefix),
                                       workspace,
                                       [&](std::span<const k::Vertex> extension)
                                       {
                                           auto values = Extension {};
                                           for (const auto vertex : extension)
                                               values.push_back(ygg::uint_t(graph.get_object(vertex)));
                                           result.push_back(std::move(values));
                                       });
    return result;
}

Extensions enumerate_cliques(const LabeledGraph& graph, k::Workspace& workspace)
{
    auto result = Extensions {};
    k::KCKP(graph.get_graph())
        .for_each_clique(
            [&](const auto& clique)
            {
                auto values = Extension {};
                for (const auto vertex : clique)
                    values.push_back(ygg::uint_t(graph.get_object(vertex)));
                result.push_back(std::move(values));
            },
            workspace);
    return result;
}
}

TEST(TyrKCKPGraphLayout, UsesVertexIndicesAsDefaultLabels)
{
    const auto layout = k::create_graph_layout(std::vector<size_t> { 2, 1 });

    EXPECT_EQ(layout.vertex_labels, (std::vector<ygg::uint_t> { 0, 1, 2 }));
}

TEST(TyrAnalysisStaticLiteralCompatibility, ProjectsSupportedStaticTuplesExactlyWhenArityAllows)
{
    using Compatibility = a::detail::StaticLiteralCompatibility;

    {
        const auto compatibility =
            Compatibility(std::vector { literal(true, 0, { parameter(0), constant(9), parameter(0), parameter(1) }) },
                          std::vector { atom(0, { 1, 9, 1, 2 }), atom(0, { 3, 8, 3, 4 }), atom(0, { 5, 9, 6, 7 }), atom(1, { 8, 9, 8, 9 }) },
                          a::VariableDomainList { domain({ 1, 3, 5 }), domain({ 2, 4, 7 }) },
                          10);

        ASSERT_TRUE(compatibility.is_satisfiable());
        EXPECT_TRUE(compatibility.unary_compatible(0, Object(1)));
        EXPECT_TRUE(compatibility.unary_compatible(1, Object(2)));
        EXPECT_FALSE(compatibility.unary_compatible(0, Object(3)));
        EXPECT_FALSE(compatibility.unary_compatible(0, Object(5)));
        EXPECT_TRUE(compatibility.binary_compatible(0, Object(1), 1, Object(2)));
        EXPECT_FALSE(compatibility.binary_compatible(0, Object(3), 1, Object(4)));
        EXPECT_FALSE(compatibility.binary_compatible(0, Object(5), 1, Object(7)));
    }

    {
        const auto negative_present =
            Compatibility(std::vector { literal(false, 0, { constant(9) }) }, std::vector { atom(0, { 9 }) }, a::VariableDomainList {}, 10);
        const auto negative_absent =
            Compatibility(std::vector { literal(false, 0, { constant(9) }) }, std::vector { atom(0, { 8 }) }, a::VariableDomainList {}, 10);
        const auto positive_present =
            Compatibility(std::vector { literal(true, 0, { constant(9) }) }, std::vector { atom(0, { 9 }) }, a::VariableDomainList {}, 10);
        const auto positive_absent =
            Compatibility(std::vector { literal(true, 0, { constant(9) }) }, std::vector { atom(0, { 8 }) }, a::VariableDomainList {}, 10);
        EXPECT_FALSE(negative_present.is_satisfiable());
        EXPECT_TRUE(negative_absent.is_satisfiable());
        EXPECT_TRUE(positive_present.is_satisfiable());
        EXPECT_FALSE(positive_absent.is_satisfiable());
    }

    {
        const auto compatibility = Compatibility(std::vector { literal(false, 0, { parameter(0), constant(9) }) },
                                                 std::vector { atom(0, { 1, 9 }), atom(0, { 2, 9 }), atom(0, { 3, 8 }), atom(1, { 4, 9 }) },
                                                 a::VariableDomainList { domain({ 1, 2, 3, 4 }) },
                                                 10);
        EXPECT_FALSE(compatibility.unary_compatible(0, Object(1)));
        EXPECT_FALSE(compatibility.unary_compatible(0, Object(2)));
        EXPECT_TRUE(compatibility.unary_compatible(0, Object(3)));
        EXPECT_TRUE(compatibility.unary_compatible(0, Object(4)));
    }

    {
        const auto compatibility = Compatibility(std::vector { literal(false, 0, { parameter(0), parameter(1), constant(9) }) },
                                                 std::vector { atom(0, { 1, 2, 9 }), atom(0, { 2, 1, 9 }), atom(0, { 1, 1, 8 }) },
                                                 a::VariableDomainList { domain({ 1, 2 }), domain({ 1, 2 }) },
                                                 10);
        EXPECT_TRUE(compatibility.unary_compatible(0, Object(1)));
        EXPECT_FALSE(compatibility.binary_compatible(0, Object(1), 1, Object(2)));
        EXPECT_FALSE(compatibility.binary_compatible(1, Object(1), 0, Object(2)));
        EXPECT_TRUE(compatibility.binary_compatible(0, Object(1), 1, Object(1)));
    }

    {
        const auto compatibility = Compatibility(std::vector { literal(false, 0, { parameter(0), parameter(1), parameter(2) }) },
                                                 std::vector { atom(0, { 1, 2, 3 }) },
                                                 a::VariableDomainList { domain({ 1 }), domain({ 2 }), domain({ 3 }) },
                                                 4);
        EXPECT_TRUE(compatibility.unary_compatible(0, Object(1)));
        EXPECT_TRUE(compatibility.binary_compatible(0, Object(1), 1, Object(2)));
        EXPECT_TRUE(compatibility.binary_compatible(0, Object(1), 2, Object(3)));
        EXPECT_TRUE(compatibility.binary_compatible(1, Object(2), 2, Object(3)));
    }

    {
        const auto compatibility =
            Compatibility(std::vector { literal(true, 0, { parameter(0), parameter(1) }), literal(true, 1, { parameter(0), parameter(1) }) },
                          std::vector { atom(0, { 2, 17 }), atom(0, { 31, 2 }), atom(1, { 2, 17 }), atom(1, { 31, 17 }) },
                          a::VariableDomainList { domain({ 2, 31 }), domain({ 2, 17 }) },
                          32);

        EXPECT_TRUE(compatibility.unary_compatible(0, Object(2)));
        EXPECT_TRUE(compatibility.unary_compatible(0, Object(31)));
        EXPECT_FALSE(compatibility.unary_compatible(0, Object(3)));
        EXPECT_TRUE(compatibility.unary_compatible(1, Object(17)));
        EXPECT_FALSE(compatibility.unary_compatible(1, Object(2)));
        EXPECT_TRUE(compatibility.binary_compatible(0, Object(2), 1, Object(17)));
        EXPECT_FALSE(compatibility.binary_compatible(0, Object(31), 1, Object(17)));
        EXPECT_TRUE(compatibility.binary_compatible(1, Object(17), 0, Object(2)));
    }

    {
        const auto compatibility =
            Compatibility(std::vector { literal(true, 0, { parameter(0) }) }, std::vector { atom(0, { 17 }) }, a::VariableDomainList { domain({ 17 }) }, 18);

        EXPECT_TRUE(compatibility.unary_compatible(1, Object(31)));
        EXPECT_TRUE(compatibility.binary_compatible(0, Object(17), 1, Object(31)));
        EXPECT_TRUE(compatibility.binary_compatible(2, Object(31), 1, Object(17)));
    }

    {
        const auto compatibility = Compatibility(std::vector<FakeLiteralView> {}, std::vector<FakeGroundAtomView> {}, a::VariableDomainList { domain({}) }, 0);
        EXPECT_TRUE(compatibility.is_satisfiable());
        EXPECT_FALSE(compatibility.unary_compatible(0, Object(0)));
    }
}

TEST(TyrKCKP, PlainCliqueEnumerationReturnsFullAssignments)
{
    const auto graph = make_graph(a::VariableDomainList { domain({ 0, 1 }), domain({ 2, 3 }) },
                                  true,
                                  [](auto, Object first, auto, Object second) { return ygg::uint_t(second) == ygg::uint_t(first) + 2; });

    auto workspace = k::Workspace {};
    EXPECT_EQ(enumerate_cliques(graph, workspace), (Extensions { { 0, 2 }, { 1, 3 } }));
}

TEST(TyrKCKP, ValidatesVertexPrefixes)
{
    const auto graph = make_graph(a::VariableDomainList { domain({ 0, 1 }), domain({ 2, 3 }) },
                                  true,
                                  [](auto, Object first, auto, Object second) { return ygg::uint_t(second) == ygg::uint_t(first) + 2; });
    const auto& raw_graph = graph.get_graph();
    auto workspace = k::Workspace {};
    const auto count_extensions = [&](std::initializer_list<k::Vertex> prefix)
    {
        auto count = size_t { 0 };
        k::KCKP(raw_graph).for_each_compatible_extension(std::span<const k::Vertex>(prefix.begin(), prefix.size()),
                                                         workspace,
                                                         [&](std::span<const k::Vertex>) { ++count; });
        return count;
    };

    EXPECT_EQ(count_extensions({ graph.find_vertex(0, Object(0)) }), 1);
    EXPECT_EQ(count_extensions({ graph.find_vertex(1, Object(2)) }), 0);
    EXPECT_EQ(count_extensions({ k::Vertex(raw_graph.get_layout().num_vertices) }), 0);
    EXPECT_EQ(count_extensions({ graph.find_vertex(0, Object(0)), graph.find_vertex(1, Object(3)) }), 0);
}

TEST(TyrKCKPGraph, ArcConsistencyAndSeededEnumerationCoverDegenerateCases)
{
    const auto graph = make_graph(a::VariableDomainList { domain({ 0, 1, 2 }), domain({ 3, 4, 5 }), domain({ 6, 7 }) },
                                  true,
                                  [](ygg::uint_t first_partition, Object first, ygg::uint_t second_partition, Object second)
                                  {
                                      const auto lhs = ygg::uint_t(first);
                                      const auto rhs = ygg::uint_t(second);
                                      if (first_partition == 0 && second_partition == 1)
                                          return (lhs == 0 && (rhs == 3 || rhs == 4)) || (lhs == 1 && rhs == 4) || (lhs == 2 && rhs == 5);
                                      if (first_partition == 1 && second_partition == 2)
                                          return (lhs == 3 && rhs == 6) || (lhs == 4 && (rhs == 6 || rhs == 7));
                                      return true;
                                  });

    ASSERT_TRUE(graph.is_satisfiable());
    EXPECT_EQ(graph.get_domains()[0].objects, (std::vector<Object> { Object(0), Object(1) }));
    EXPECT_EQ(graph.get_domains()[1].objects, (std::vector<Object> { Object(3), Object(4) }));
    EXPECT_EQ(graph.get_domains()[2].objects, (std::vector<Object> { Object(6), Object(7) }));

    auto workspace = k::Workspace {};
    const auto all = Extensions { { 0, 3, 6 }, { 0, 4, 6 }, { 0, 4, 7 }, { 1, 4, 6 }, { 1, 4, 7 } };
    EXPECT_EQ(enumerate(graph, {}, workspace), all);
    EXPECT_EQ(enumerate(graph, { 0 }, workspace), (Extensions { { 3, 6 }, { 4, 6 }, { 4, 7 } }));
    EXPECT_TRUE(enumerate(graph, { 2 }, workspace).empty());
    EXPECT_TRUE(enumerate(graph, { 1, 3 }, workspace).empty());

    EXPECT_EQ(enumerate(graph, { 0, 3, 6 }, workspace), (Extensions { {} }));
    EXPECT_TRUE(enumerate(graph, { 1, 3, 6 }, workspace).empty());

    const auto unary = make_graph(a::VariableDomainList { domain({ 9 }) }, true, [](auto, auto, auto, auto) { return true; });
    EXPECT_EQ(enumerate(unary, {}, workspace), (Extensions { { 9 } }));
    EXPECT_EQ(enumerate(graph, {}, workspace), all);

    const auto satisfiable_nullary = make_graph(a::VariableDomainList {}, true, [](auto, auto, auto, auto) { return true; });
    const auto dead_nullary = make_graph(a::VariableDomainList {}, false, [](auto, auto, auto, auto) { return true; });
    EXPECT_EQ(enumerate(satisfiable_nullary, {}, workspace), (Extensions { {} }));
    EXPECT_TRUE(enumerate(dead_nullary, {}, workspace).empty());

    const auto dead_unary = make_graph(a::VariableDomainList { domain({ 8, 9 }) }, false, [](auto, auto, auto, auto) { return true; });
    EXPECT_TRUE(dead_unary.get_domains()[0].objects.empty());
}

TEST(TyrKCKPGraph, IncrementalSupportsPropagateAsymmetricCascadeAndPreserveCompactionOrder)
{
    const auto graph = make_graph(a::VariableDomainList { domain({ 3, 1, 2, 0 }), domain({ 11, 10 }), domain({ 22, 20, 21 }), domain({ 30 }) },
                                  true,
                                  [](ygg::uint_t first_partition, Object first, ygg::uint_t second_partition, Object second)
                                  {
                                      const auto lhs = ygg::uint_t(first);
                                      const auto rhs = ygg::uint_t(second);
                                      if (first_partition == 0 && second_partition == 1)
                                          return (rhs == 10 && (lhs == 1 || lhs == 0)) || (rhs == 11 && (lhs == 3 || lhs == 2));
                                      if (first_partition == 1 && second_partition == 2)
                                          return (lhs == 10 && (rhs == 20 || rhs == 21)) || (lhs == 11 && (rhs == 21 || rhs == 22));
                                      if (first_partition == 2 && second_partition == 3)
                                          return lhs == 20 && rhs == 30;
                                      return true;
                                  });

    ASSERT_TRUE(graph.is_satisfiable());
    EXPECT_EQ(graph.get_domains()[0].objects, (std::vector<Object> { Object(1), Object(0) }));
    EXPECT_EQ(graph.get_domains()[1].objects, (std::vector<Object> { Object(10) }));
    EXPECT_EQ(graph.get_domains()[2].objects, (std::vector<Object> { Object(20) }));
    EXPECT_EQ(graph.get_domains()[3].objects, (std::vector<Object> { Object(30) }));

    auto workspace = k::Workspace {};
    EXPECT_EQ(enumerate(graph, {}, workspace), (Extensions { { 1, 10, 20, 30 }, { 0, 10, 20, 30 } }));
}

TEST(TyrKCKPGraph, EmptyPartitionMakesGraphUnsatisfiable)
{
    const auto graph = make_graph(a::VariableDomainList { domain({ 0, 1 }), domain({}), domain({ 2 }) }, true, [](auto, auto, auto, auto) { return true; });

    EXPECT_FALSE(graph.is_satisfiable());
    for (const auto& variable_domain : graph.get_domains())
        EXPECT_TRUE(variable_domain.objects.empty());

    auto workspace = k::Workspace {};
    EXPECT_TRUE(enumerate(graph, {}, workspace).empty());
}

TEST(TyrKCKPGraph, ArcConsistencyDoesNotImplyClique)
{
    const auto graph = make_graph(a::VariableDomainList { domain({ 0, 1 }), domain({ 2, 3 }), domain({ 4, 5 }) },
                                  true,
                                  [](ygg::uint_t first_partition, Object first, ygg::uint_t second_partition, Object second)
                                  {
                                      const auto lhs = ygg::uint_t(first);
                                      const auto rhs = ygg::uint_t(second);
                                      if (first_partition == 0 && second_partition == 1)
                                          return (lhs == 0 && rhs == 2) || (lhs == 1 && rhs == 3);
                                      if (first_partition == 1 && second_partition == 2)
                                          return (lhs == 2 && rhs == 4) || (lhs == 3 && rhs == 5);
                                      return (lhs == 0 && rhs == 5) || (lhs == 1 && rhs == 4);
                                  });

    ASSERT_TRUE(graph.is_satisfiable());
    EXPECT_EQ(graph.get_domains()[0].objects, (std::vector<Object> { Object(0), Object(1) }));
    EXPECT_EQ(graph.get_domains()[1].objects, (std::vector<Object> { Object(2), Object(3) }));
    EXPECT_EQ(graph.get_domains()[2].objects, (std::vector<Object> { Object(4), Object(5) }));

    auto workspace = k::Workspace {};
    EXPECT_TRUE(enumerate(graph, {}, workspace).empty());
}

TEST(TyrKCKPGraph, SupportAcrossBitsetBlockBoundaryIsPreserved)
{
    auto target_domain = a::VariableDomain {};
    for (ygg::uint_t object = 1; object <= 65; ++object)
        target_domain.objects.emplace_back(object);

    auto domains = a::VariableDomainList {};
    domains.push_back(domain({ 0 }));
    domains.push_back(std::move(target_domain));
    const auto graph = make_graph(std::move(domains), true, [](auto, auto, auto, Object target) { return ygg::uint_t(target) == 65; });

    ASSERT_TRUE(graph.is_satisfiable());
    EXPECT_EQ(graph.get_domains()[0].objects, (std::vector<Object> { Object(0) }));
    EXPECT_EQ(graph.get_domains()[1].objects, (std::vector<Object> { Object(65) }));

    auto workspace = k::Workspace {};
    EXPECT_EQ(enumerate(graph, {}, workspace), (Extensions { { 0, 65 } }));
}

}
