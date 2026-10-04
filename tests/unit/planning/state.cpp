#include "tyr/formalism/planning/parser.hpp"
#include "tyr/planning/planning.hpp"
#include "tyr/planning/state_data.hpp"
#include "tyr/planning/state_index.hpp"
#include "tyr/planning/state_view.hpp"

#include <algorithm>
#include <barrier>
#include <concepts>
#include <deque>
#include <future>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
#include <yggdrasil/containers/unique_object_pool.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace fp = tyr::formalism::planning;
namespace p = tyr::planning;

template<typename Kind>
concept StateIndexContract = std::constructible_from<ygg::Index<p::State<Kind>>, ygg::uint_t> && std::totally_ordered<ygg::Index<p::State<Kind>>>;

using StateKinds = ygg::TypeList<tyr::GroundTag, tyr::LiftedTag>;
static_assert([]<typename... Kinds>(ygg::TypeList<Kinds...>) { return (StateIndexContract<Kinds> && ...); }(StateKinds {}));
static_assert([]<typename... Kinds>(ygg::TypeList<Kinds...>)
              { return (std::totally_ordered<p::PackedStateView<Kinds>> && ...); }(StateKinds {}));
static_assert([]<typename... Kinds>(ygg::TypeList<Kinds...>)
              { return ((p::StateViewConcept<p::StateView<Kinds>> && p::StateViewConcept<p::BuilderStateView<Kinds>>) && ...); }(StateKinds {}));

template<typename T>
concept HasStateIdentity = requires(const T& state) { state.get_index(); }
                          || requires(const T& state) { state.pack(); }
                          || requires(const T& state) { state.get_state_repository(); };

static_assert([]<typename... Kinds>(ygg::TypeList<Kinds...>)
              { return ((!HasStateIdentity<p::BuilderStateView<Kinds>> && !p::StateViewConcept<ygg::Builder<p::State<Kinds>>>) && ...); }(StateKinds {}));
static_assert(!p::StateViewConcept<int>);
static_assert(p::StateViewConcept<p::BuilderStateView<tyr::GroundTag>, tyr::GroundTag>);
static_assert(p::StateViewConcept<p::StateView<tyr::LiftedTag>, tyr::LiftedTag>);
static_assert(!p::StateViewConcept<p::BuilderStateView<tyr::GroundTag>, tyr::LiftedTag>);
static_assert(!p::StateViewConcept<p::StateView<tyr::LiftedTag>, tyr::GroundTag>);

template<typename T>
concept NodeState = requires { typename p::Node<T>; };

template<typename T>
concept Packable = requires(const T& value) { value.pack(); };

static_assert([]<typename... Kinds>(ygg::TypeList<Kinds...>)
              {
                  return ((p::NodeConcept<p::Node<p::StateView<Kinds>>> && std::totally_ordered<p::Node<p::StateView<Kinds>>>
                           && Packable<p::Node<p::StateView<Kinds>>> && Packable<p::LabeledNode<p::StateView<Kinds>>>)
                          && ...);
              }(StateKinds {}));
static_assert([]<typename... Kinds>(ygg::TypeList<Kinds...>)
              {
                  return ((p::NodeConcept<p::Node<p::BuilderStateView<Kinds>>> && !ygg::Identifiable<p::Node<p::BuilderStateView<Kinds>>>
                           && !std::equality_comparable<p::Node<p::BuilderStateView<Kinds>>> && !Packable<p::Node<p::BuilderStateView<Kinds>>>
                           && !Packable<p::LabeledNode<p::BuilderStateView<Kinds>>> && !NodeState<Kinds>)
                          && ...);
              }(StateKinds {}));
static_assert(!NodeState<int>);

namespace tyr::tests
{
namespace
{
template<TaskKind Kind>
void expect_packed_state_identity()
{
    using Data = ygg::Data<p::State<Kind>>;
    using Index = ygg::Index<p::State<Kind>>;
    auto facts = p::FactPackedStorage<Kind, p::StateStoragePolicyTag> {};
    auto derived = p::AtomPackedStorage<Kind, p::StateStoragePolicyTag> {};
    auto numeric = p::NumericPackedStorage<Kind, p::StateStoragePolicyTag> {};
    const auto base = Data(Index(0), facts, derived, numeric);
    const auto hash = ygg::Hash<Data> {};
    derived.index = { 1 };
    const auto extended = Data(Index(1), facts, derived, numeric);
    EXPECT_NE(base.template get_atoms<formalism::DerivedTag>(), extended.template get_atoms<formalism::DerivedTag>());
    EXPECT_EQ(base, extended);
    EXPECT_EQ(hash(base), hash(extended));

    facts.index = { 1 };
    const auto changed_facts = Data(Index(2), facts, derived, numeric);
    EXPECT_NE(base, changed_facts);
    numeric.index = { 1 };
    const auto changed_numeric = Data(Index(3), base.template get_atoms<formalism::FluentTag>(), derived, numeric);
    EXPECT_NE(base, changed_numeric);
}

template<TaskKind Kind>
void expect_state_builder_identity()
{
    using Builder = ygg::Builder<p::State<Kind>>;
    auto base = Builder {};
    if constexpr (std::same_as<Kind, GroundTag>)
        base.template get_atoms<formalism::FluentTag>().values = { 1, 0 };
    else
    {
        base.template get_atoms<formalism::FluentTag>().indices.resize(2);
        base.template get_atoms<formalism::FluentTag>().indices.set(0);
    }
    base.get_numeric_variables().values = { 3.0 };
    auto other = base;
    base.set(ygg::Index<p::State<Kind>>(0));
    other.set(ygg::Index<p::State<Kind>>(1));
    other.template get_atoms<formalism::DerivedTag>().indices.resize(3, true);
    EXPECT_TRUE(ygg::EqualTo<Builder> {}(base, other));
    EXPECT_EQ(ygg::Hash<Builder> {}(base), ygg::Hash<Builder> {}(other));

    if constexpr (std::same_as<Kind, GroundTag>)
        other.template get_atoms<formalism::FluentTag>().values.front() = 0;
    else
        other.template get_atoms<formalism::FluentTag>().indices.flip(0);
    EXPECT_FALSE(ygg::EqualTo<Builder> {}(base, other));
    other = base;
    other.get_numeric_variables().values.front() = 4.0;
    EXPECT_FALSE(ygg::EqualTo<Builder> {}(base, other));
}

template<TaskKind Kind>
auto derived_names(const p::StateView<Kind>& state)
{
    auto result = std::vector<std::string> {};
    for (const auto atom : state.get_derived_atoms_view())
        result.push_back(atom.get_predicate().get_name().str());
    std::ranges::sort(result);
    return result;
}

template<TaskKind Kind>
void expect_borrowed_builder_view(const p::TaskPtr<Kind>& task, const p::StateView<Kind>& registered, fp::ActionBindingView label)
{
    const auto num_states = registered.get_state_repository()->num_states();
    auto pool = ygg::UniqueObjectPool<ygg::Builder<p::State<Kind>>> {};
    auto owned = pool.get_or_allocate();
    *owned = registered.get_state_builder();
    const auto state = ygg::make_view(*owned, *task);
    static_assert(std::same_as<decltype(state), const p::BuilderStateView<Kind>>);
    const auto node = p::Node(state, 3);
    using BorrowedNode = std::remove_cvref_t<decltype(node)>;
    static_assert(std::same_as<typename BorrowedNode::StateType, p::BuilderStateView<Kind>>);
    static_assert(std::same_as<typename BorrowedNode::KindType, Kind>);
    static_assert(std::same_as<typename BorrowedNode::TaskType, p::Task<Kind>>);
    const auto labeled = p::LabeledNode { label, node };
    const auto nodes = p::NodeList<p::BuilderStateView<Kind>> { node };
    const auto labeled_nodes = p::LabeledNodeList<p::BuilderStateView<Kind>> { labeled };
    EXPECT_EQ(&nodes.front().get_state().get_state_builder(), owned.get());
    EXPECT_EQ(&labeled_nodes.front().node.get_state().get_state_builder(), owned.get());
    EXPECT_EQ(nodes.front().get_metric(), 3);
    EXPECT_EQ(labeled_nodes.front().node.get_metric(), 3);
    EXPECT_EQ(labeled_nodes.front().label, label);
    const auto formatted_node = fmt::format("{}", node);
    EXPECT_NE(formatted_node.find("metric value = 3"), std::string::npos);
    EXPECT_EQ(formatted_node.find("index = "), std::string::npos);
    EXPECT_NE(fmt::format("{}", labeled).find(label.get_relation().get_name().str()), std::string::npos);
    EXPECT_EQ(&state.get_handle(), owned.get());
    EXPECT_EQ(&state.get_state_builder(), owned.get());
    EXPECT_EQ(&state.get_context(), task.get());
    EXPECT_EQ(&state.get_task(), &registered.get_task());
    EXPECT_EQ(state.get_repository(), registered.get_repository());
    EXPECT_TRUE(std::ranges::equal(state.get_static_atoms(), registered.get_static_atoms()));
    EXPECT_TRUE(std::ranges::equal(state.get_fluent_facts(), registered.get_fluent_facts()));
    EXPECT_TRUE(std::ranges::equal(state.get_derived_atoms(), registered.get_derived_atoms()));
    EXPECT_TRUE(std::ranges::equal(state.get_static_fterm_values(), registered.get_static_fterm_values()));
    EXPECT_TRUE(std::ranges::equal(state.get_fluent_fterm_values(), registered.get_fluent_fterm_values()));
    EXPECT_TRUE(std::ranges::equal(state.get_static_atoms_view(), registered.get_static_atoms_view()));
    EXPECT_TRUE(std::ranges::equal(state.get_fluent_facts_view(), registered.get_fluent_facts_view()));
    EXPECT_TRUE(std::ranges::equal(state.get_fluent_atoms_view(), registered.get_fluent_atoms_view()));
    EXPECT_TRUE(std::ranges::equal(state.get_derived_atoms_view(), registered.get_derived_atoms_view()));
    EXPECT_TRUE(std::ranges::equal(p::get_atoms_view<formalism::StaticTag>(state), p::get_atoms_view<formalism::StaticTag>(registered)));
    EXPECT_TRUE(std::ranges::equal(p::get_atoms_view<formalism::FluentTag>(state), p::get_atoms_view<formalism::FluentTag>(registered)));
    EXPECT_TRUE(std::ranges::equal(p::get_atoms_view<formalism::DerivedTag>(state), p::get_atoms_view<formalism::DerivedTag>(registered)));
    EXPECT_TRUE(std::ranges::equal(state.get_static_fterm_values_view(), registered.get_static_fterm_values_view()));
    EXPECT_TRUE(std::ranges::equal(state.get_fluent_fterm_values_view(), registered.get_fluent_fterm_values_view()));
    for (const auto atom : registered.get_static_atoms_view())
    {
        EXPECT_EQ(state.test(atom.get_index()), registered.test(atom));
        EXPECT_EQ(state.test(atom), registered.test(atom));
    }
    for (const auto fact : registered.get_fluent_facts_view())
    {
        EXPECT_EQ(state.get(fact.get_variable().get_index()), fact.get_value());
        EXPECT_EQ(state.get(fact.get_variable()), fact.get_value());
    }
    for (const auto atom : registered.get_derived_atoms_view())
    {
        EXPECT_EQ(state.test(atom.get_index()), registered.test(atom));
        EXPECT_EQ(state.test(atom), registered.test(atom));
    }
    for (const auto& [fterm, value] : registered.get_static_fterm_values_view())
    {
        EXPECT_EQ(state.get(fterm.get_index()), value);
        EXPECT_EQ(state.get(fterm), value);
    }
    for (const auto& [fterm, value] : registered.get_fluent_fterm_values_view())
    {
        EXPECT_EQ(state.get(fterm.get_index()), value);
        EXPECT_EQ(state.get(fterm), value);
    }

    auto moved = std::move(owned);
    EXPECT_FALSE(owned);
    EXPECT_EQ(&state.get_state_builder(), moved.get());
    const auto numeric = registered.get_fluent_fterm_values_view();
    ASSERT_NE(numeric.begin(), numeric.end());
    const auto [fterm, value] = *numeric.begin();
    moved->set(fterm, value + 1);
    EXPECT_EQ(state.get(fterm), value + 1);
    EXPECT_EQ(node.get_state().get(fterm), value + 1);
    EXPECT_EQ(nodes.front().get_state().get(fterm), value + 1);
    EXPECT_EQ(labeled_nodes.front().node.get_state().get(fterm), value + 1);
    EXPECT_EQ(registered.get(fterm), value);
    EXPECT_EQ(registered.get_state_repository()->num_states(), num_states);
    if constexpr (std::same_as<Kind, GroundTag>)
    {
        auto& values = moved->template get_atoms<formalism::FluentTag>().values;
        std::ranges::fill(values, ygg::uint_t(0));
        const auto atoms = p::get_atoms_view<formalism::FluentTag>(ygg::make_view(*moved, *task));
        EXPECT_TRUE(atoms.begin() == atoms.end());
    }
}

template<TaskKind Kind>
void expect_registered_closures_and_transition_costs(const p::TaskPtr<Kind>& task, bool concurrent)
{
    SCOPED_TRACE(concurrent ? "concurrent storage" : "sequential storage");
    const auto context = ygg::ExecutionContext::create(1);
    auto axioms = p::AxiomEvaluatorFactory<Kind>().create(task, context);
    auto generator = p::SuccessorGeneratorFactory<Kind>().create(task, context);
    auto factory = p::StateRepositoryFactory<Kind> {};
    auto repository = concurrent ? factory.create_concurrent(task) : factory.create(task);
    std::weak_ptr<p::StateRepository<Kind>> owner;
    {
        const ygg::Builder<p::State<Kind>>* released_builder = nullptr;
        const auto packed = [&]
        {
            auto temporary_repository = factory.create(task);
            owner = temporary_repository;
            const auto state = temporary_repository->get_initial_state(*axioms);
            released_builder = &state.get_state_builder();
            return state.pack();
        }();
        EXPECT_FALSE(owner.expired());
        auto recycled_builder = packed.get_state_repository()->get_state_builder();
        EXPECT_EQ(recycled_builder.get(), released_builder);
        recycled_builder = {};
        EXPECT_EQ(packed.unpack().pack(), packed);
        EXPECT_EQ(packed.get_state_repository()->num_states(), 1);
    }
    EXPECT_TRUE(owner.expired());

    const auto initial = generator->get_initial_node(*repository, *axioms);
    const auto successors = generator->get_labeled_successor_nodes(initial, *repository, *axioms);
    ASSERT_EQ(successors.size(), 2);
    const auto cheap = std::ranges::find_if(successors, [](const auto& next) { return next.label.get_relation().get_name().str() == "cheap"; });
    const auto expensive = std::ranges::find_if(successors, [](const auto& next) { return next.label.get_relation().get_name().str() == "expensive"; });
    ASSERT_NE(cheap, successors.end());
    ASSERT_NE(expensive, successors.end());
    EXPECT_EQ(cheap->node.get_state(), expensive->node.get_state());
    EXPECT_EQ(cheap->node.get_metric(), 1);
    EXPECT_EQ(expensive->node.get_metric(), 7);
    const auto packed = cheap->pack();
    EXPECT_EQ(packed.label, cheap->label);
    EXPECT_EQ(packed.node.get_state(), cheap->node.get_state().pack());
    EXPECT_EQ(packed.node.get_metric(), cheap->node.get_metric());
    EXPECT_EQ(packed.node, cheap->node.pack());
    EXPECT_NE(packed.node, expensive->node.pack());
    EXPECT_EQ(packed.unpack().label, cheap->label);
    EXPECT_EQ(packed.unpack().node, cheap->node);

    EXPECT_EQ(repository->num_states(), 2);
    EXPECT_EQ(derived_names(cheap->node.get_state()), (std::vector<std::string> { "live" }));
    EXPECT_EQ(derived_names(expensive->node.get_state()), derived_names(cheap->node.get_state()));
    expect_borrowed_builder_view(task, cheap->node.get_state(), cheap->label);

    const auto expect_duplicate = [&](const auto& state)
    {
        const auto packed = state.pack();
        const auto unpacked = packed.unpack();
        EXPECT_EQ(packed.get_index(), state.get_index());
        EXPECT_EQ(packed.get_state_repository(), repository);
        EXPECT_EQ(unpacked, state);
        EXPECT_EQ(unpacked.pack(), packed);
        EXPECT_TRUE(std::ranges::equal(unpacked.get_fluent_facts(), state.get_fluent_facts()));
        EXPECT_EQ(derived_names(unpacked), derived_names(state));
        EXPECT_TRUE(std::ranges::equal(unpacked.get_fluent_fterm_values(), state.get_fluent_fterm_values()));

        auto builder = repository->get_state_builder();
        builder->assign_unextended_part(state.get_state_builder());
        const auto derived = builder->get_derived_atoms();
        EXPECT_EQ(derived.begin(), derived.end());
        const auto duplicate = repository->register_state(*axioms, std::move(builder));
        EXPECT_EQ(duplicate, state);
        EXPECT_EQ(derived_names(duplicate), derived_names(state));
        EXPECT_EQ(derived_names(repository->get_registered_state(duplicate.get_index())), derived_names(state));
    };
    expect_duplicate(cheap->node.get_state());

    const auto bindings = generator->get_applicable_action_bindings(cheap->node);
    const auto raise = std::ranges::find_if(bindings, [](const auto binding) { return binding.get_relation().get_name().str() == "raise"; });
    const auto disable = std::ranges::find_if(bindings, [](const auto binding) { return binding.get_relation().get_name().str() == "disable"; });
    ASSERT_NE(raise, bindings.end());
    ASSERT_NE(disable, bindings.end());
    const auto raised = [&]
    {
        if (!concurrent)
            return generator->get_successor_node(cheap->node, *raise, *repository, *axioms);

        const auto worker_context = ygg::ExecutionContext::create(1);
        auto worker_axioms = axioms->make_worker(worker_context);
        auto worker_generator = generator->make_worker(worker_context);
        auto worker_repository = repository->make_worker();
        auto start = std::barrier(2);
        auto first = std::async(std::launch::async,
                                [&]
                                {
                                    start.arrive_and_wait();
                                    return generator->get_successor_node(cheap->node, *raise, *repository, *axioms);
                                });
        auto second = std::async(std::launch::async,
                                 [&]
                                 {
                                     start.arrive_and_wait();
                                     return worker_generator->get_successor_node(cheap->node, *raise, *worker_repository, *worker_axioms);
                                 });
        const auto first_node = first.get();
        const auto second_node = second.get();
        EXPECT_EQ(first_node, second_node);
        EXPECT_EQ(derived_names(first_node.get_state()), derived_names(second_node.get_state()));
        return first_node;
    }();
    EXPECT_NE(raised.get_state(), cheap->node.get_state());
    EXPECT_EQ(derived_names(raised.get_state()), (std::vector<std::string> { "live", "ready" }));
    EXPECT_EQ(repository->num_states(), 3);
    {
        auto pool = ygg::UniqueObjectPool<ygg::Builder<p::State<Kind>>> {};
        auto owned = pool.get_or_allocate();
        *owned = cheap->node.get_state().get_state_builder();
        owned->set(ygg::Index<p::State<Kind>> {});
        const auto borrowed = p::Node(ygg::make_view(*owned, *task), cheap->node.get_metric());
        EXPECT_EQ(generator->get_applicable_action_bindings(borrowed), bindings);
        const auto expect_same_node = [](const auto& actual, const auto& expected)
        {
            EXPECT_EQ(actual.get_metric(), expected.get_metric());
            EXPECT_TRUE(std::ranges::equal(actual.get_state().get_fluent_facts(), expected.get_state().get_fluent_facts()));
            EXPECT_TRUE(std::ranges::equal(actual.get_state().get_derived_atoms(), expected.get_state().get_derived_atoms()));
            EXPECT_TRUE(std::ranges::equal(actual.get_state().get_fluent_fterm_values(), expected.get_state().get_fluent_fterm_values()));
        };
        auto generated = pool.get_or_allocate();
        const auto successor = generator->get_successor_node(borrowed, *raise, *generated, *axioms);
        static_assert(std::same_as<decltype(successor), const p::Node<p::BuilderStateView<Kind>>>);
        EXPECT_EQ(&successor.get_state().get_state_builder(), generated.get());
        expect_same_node(successor, raised);
        EXPECT_TRUE(generated->get_index().is_max());
        if constexpr (std::same_as<Kind, GroundTag>)
            expect_same_node(generator->get_successor_node(borrowed, generator->ground_action(*raise), *generated, *axioms), raised);

        auto reference_repository = factory.create(task);
        const auto expected = generator->get_labeled_successor_nodes(cheap->node, *reference_repository, *axioms);
        auto states = std::deque<ygg::Builder<p::State<Kind>>> {};
        const auto labeled = generator->get_labeled_successor_nodes(borrowed, states, *axioms);
        const auto nodes = generator->get_successor_nodes(borrowed, states, *axioms);
        static_assert(std::same_as<decltype(labeled), const p::LabeledNodeList<p::BuilderStateView<Kind>>>);
        static_assert(std::same_as<decltype(nodes), const p::NodeList<p::BuilderStateView<Kind>>>);
        ASSERT_EQ(labeled.size(), 2);
        ASSERT_EQ(labeled.size(), expected.size());
        ASSERT_EQ(nodes.size(), expected.size());
        EXPECT_NE(&labeled[0].node.get_state().get_state_builder(), &labeled[1].node.get_state().get_state_builder());
        for (size_t i = 0; i < expected.size(); ++i)
        {
            EXPECT_EQ(labeled[i].label, expected[i].label);
            expect_same_node(labeled[i].node, expected[i].node);
            expect_same_node(nodes[i], expected[i].node);
        }
        const auto retained = std::ranges::find_if(labeled, [&](const auto& node) { return node.label == *raise; });
        ASSERT_NE(retained, labeled.end());
        const auto old_size = states.size();
        const auto grandchildren = generator->get_labeled_successor_nodes(retained->node, states, *axioms);
        const auto expected_grandchildren = generator->get_labeled_successor_nodes(raised, *reference_repository, *axioms);
        ASSERT_EQ(grandchildren.size(), expected_grandchildren.size());
        EXPECT_EQ(states.size(), old_size + grandchildren.size());
        for (size_t i = 0; i < grandchildren.size(); ++i)
        {
            EXPECT_EQ(grandchildren[i].label, expected_grandchildren[i].label);
            expect_same_node(grandchildren[i].node, expected_grandchildren[i].node);
        }
        for (size_t i = 0; i < expected.size(); ++i)
        {
            expect_same_node(labeled[i].node, expected[i].node);
            expect_same_node(nodes[i], expected[i].node);
        }

        size_t visited = 0;
        EXPECT_TRUE(generator->for_each_labeled_successor_node(borrowed,
                                                               *generated,
                                                               *axioms,
                                                               [&](auto next)
                                                               {
                                                                   static_assert(std::same_as<decltype(next), p::LabeledNode<p::BuilderStateView<Kind>>>);
                                                                   EXPECT_EQ(&next.node.get_state().get_state_builder(), generated.get());
                                                                   EXPECT_EQ(next.label, expected.at(visited).label);
                                                                   expect_same_node(next.node, expected.at(visited++).node);
                                                                   return true;
                                                               }));
        EXPECT_EQ(visited, expected.size());
        visited = 0;
        EXPECT_FALSE(generator->for_each_successor_node(borrowed,
                                                        *generated,
                                                        *axioms,
                                                        [&](auto next)
                                                        {
                                                            static_assert(std::same_as<decltype(next), p::Node<p::BuilderStateView<Kind>>>);
                                                            expect_same_node(next, expected.at(visited++).node);
                                                            return false;
                                                        }));
        EXPECT_EQ(visited, 1);
        EXPECT_EQ(repository->num_states(), 3);

        const auto stop = [](auto) { return false; };
        EXPECT_THROW(generator->get_successor_node(borrowed, *raise, *owned, *axioms), std::invalid_argument);
        EXPECT_THROW(generator->for_each_successor_node(borrowed, *owned, *axioms, stop), std::invalid_argument);
        EXPECT_THROW(generator->for_each_labeled_successor_node(borrowed, *owned, *axioms, stop), std::invalid_argument);
        EXPECT_THROW(generator->for_each_successor_node(borrowed, raise->get_relation(), *owned, *axioms, stop), std::invalid_argument);
        EXPECT_THROW(generator->for_each_labeled_successor_node(borrowed, raise->get_relation(), *owned, *axioms, stop), std::invalid_argument);
        expect_same_node(borrowed, cheap->node);

        EXPECT_EQ(generator->get_packed_successor_node(borrowed, *raise, *repository, *axioms), raised.pack());
        if constexpr (std::same_as<Kind, GroundTag>)
            EXPECT_EQ(generator->get_packed_successor_node(borrowed, generator->ground_action(*raise), *repository, *axioms), raised.pack());
        // Ground axiom evaluation expects the capacity normally prepared by the state repository.
        if constexpr (std::same_as<Kind, GroundTag>)
            generated->resize_derived_atoms(task->get_task().template get_atoms<formalism::DerivedTag>().size());
        generator->generate_successor_state(borrowed, *raise, *generated);
        axioms->compute_extended_state(*generated);
        const auto generated_view = ygg::make_view(*generated, *task);
        EXPECT_TRUE(std::ranges::equal(generated_view.get_fluent_facts(), raised.get_state().get_fluent_facts()));
        EXPECT_TRUE(std::ranges::equal(generated_view.get_derived_atoms(), raised.get_state().get_derived_atoms()));
        EXPECT_TRUE(std::ranges::equal(generated_view.get_fluent_fterm_values(), raised.get_state().get_fluent_fterm_values()));
        EXPECT_TRUE(owned->get_index().is_max());
        EXPECT_EQ(repository->num_states(), 3);
    }
    expect_duplicate(raised.get_state());

    const auto plan = p::Plan<Kind>(initial, { *cheap, { *raise, raised } });
    const auto packed_plan = plan.pack();
    const auto unpacked_plan = packed_plan.unpack();
    EXPECT_EQ(packed_plan.get_start_node(), initial.pack());
    EXPECT_EQ(unpacked_plan.get_start_node(), initial);
    EXPECT_FALSE(packed_plan.empty());
    EXPECT_EQ(packed_plan.get_length(), 2);
    EXPECT_EQ(packed_plan.get_cost(), raised.get_metric());
    EXPECT_EQ(unpacked_plan.get_length(), plan.get_length());
    EXPECT_EQ(unpacked_plan.get_cost(), plan.get_cost());
    for (size_t i = 0; i < plan.get_length(); ++i)
    {
        EXPECT_EQ(packed_plan.get_labeled_succ_nodes()[i].label, plan.get_labeled_succ_nodes()[i].label);
        EXPECT_EQ(packed_plan.get_labeled_succ_nodes()[i].node, plan.get_labeled_succ_nodes()[i].node.pack());
        EXPECT_EQ(unpacked_plan.get_labeled_succ_nodes()[i].label, plan.get_labeled_succ_nodes()[i].label);
        EXPECT_EQ(unpacked_plan.get_labeled_succ_nodes()[i].node, plan.get_labeled_succ_nodes()[i].node);
    }
    const auto empty_plan = p::Plan<Kind>(raised).pack();
    EXPECT_TRUE(empty_plan.empty());
    EXPECT_EQ(empty_plan.get_length(), 0);
    EXPECT_EQ(empty_plan.get_cost(), 0);
    EXPECT_EQ(empty_plan.get_start_node(), raised.pack());
    EXPECT_TRUE(empty_plan.unpack().empty());
    EXPECT_EQ(empty_plan.unpack().get_start_node(), raised);
    EXPECT_EQ(empty_plan.unpack().get_cost(), 0);

    const auto disabled = generator->get_successor_node(raised, *disable, *repository, *axioms);
    EXPECT_NE(disabled.get_state(), raised.get_state());
    EXPECT_NE(disabled.get_state(), initial.get_state());
    EXPECT_TRUE(derived_names(disabled.get_state()).empty());
    EXPECT_EQ(repository->num_states(), 4);
    expect_duplicate(disabled.get_state());
    expect_duplicate(cheap->node.get_state());
    EXPECT_EQ(repository->num_states(), 4);
}
}

TEST(TyrPlanningStateTest, BuilderIdentityUsesOnlyFluentFactsAndNumericValues)
{
    expect_state_builder_identity<GroundTag>();
    expect_state_builder_identity<LiftedTag>();
}

TEST(TyrPlanningStateTest, DuplicateStatesRetainClosuresAndIndependentTransitionCosts)
{
    expect_packed_state_identity<LiftedTag>();
    expect_packed_state_identity<GroundTag>();
    const auto lifted_task = p::Task<LiftedTag>::create(fp::Parser(R"(
(define (domain state-closure)
  (:requirements :adl :numeric-fluents :action-costs :derived-predicates)
  (:predicates (enabled) (on) (live) (ready))
  (:functions (step) (value) (total-cost))
  (:derived (live) (on))
  (:derived (ready) (and (live) (> (value) 0)))
  (:action cheap :parameters () :precondition (and (enabled) (not (on)))
    :effect (and (on) (increase (total-cost) 1)))
  (:action expensive :parameters () :precondition (and (enabled) (not (on)))
    :effect (and (on) (increase (total-cost) 7)))
  (:action raise :parameters () :precondition (on)
    :effect (and (increase (value) (step)) (increase (total-cost) 1)))
  (:action disable :parameters () :precondition (on)
    :effect (and (not (on)) (increase (total-cost) 1))))
)",
                                                                 "state-closure-domain.pddl")
                                                          .parse_task(R"(
(define (problem state-closure-problem)
  (:domain state-closure)
  (:init (enabled) (= (step) 1) (= (value) 0) (= (total-cost) 0))
  (:goal (ready))
  (:metric minimize (total-cost)))
)",
                                                                      "state-closure-problem.pddl"));
    const auto ground_task = lifted_task->instantiate_ground_task(*ygg::ExecutionContext::create(1)).task;
    ASSERT_TRUE(ground_task);
    for (const auto concurrent : { false, true })
    {
        expect_registered_closures_and_transition_costs(lifted_task, concurrent);
        expect_registered_closures_and_transition_costs(ground_task, concurrent);
    }
}
}
