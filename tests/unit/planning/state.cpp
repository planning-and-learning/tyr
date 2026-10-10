#include "tyr/formalism/planning/parser.hpp"
#include "tyr/planning/planning.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/state_data.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_view.hpp"

#include <algorithm>
#include <barrier>
#include <cmath>
#include <concepts>
#include <deque>
#include <future>
#include <gtest/gtest.h>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <yggdrasil/containers/unique_object_pool.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace fp = tyr::formalism::planning;
namespace p = tyr::planning;

template<typename Kind>
concept StateIndexContract = std::constructible_from<ygg::Index<p::State<Kind>>, ygg::uint_t> && std::totally_ordered<ygg::Index<p::State<Kind>>>;

template<typename State>
class StateWithoutMetadata : public State
{
public:
    explicit StateWithoutMetadata(State state) : State(std::move(state)) {}

private:
    using KindType = void;
    using TaskType = void;
};

template<p::IterableViewStateConcept State>
struct StateFormattingView
{
    const State& state;

    template<tyr::formalism::FactKind F>
    auto get_atoms_view() const
    {
        return state.template get_atoms_view<F>();
    }
    auto get_fluent_facts_view() const { return state.get_fluent_facts_view(); }
    template<tyr::formalism::FactKind F>
    auto get_fterm_values_view() const
    {
        return state.template get_fterm_values_view<F>();
    }
};

template<tyr::TaskKind Kind>
class BuilderWithoutTaskType : public ygg::Builder<p::State<Kind>>
{
    using TaskType = void;
};

struct StateWithMutableTask : StateWithoutMetadata<p::StateView<tyr::GroundTag>>
{
    p::Task<tyr::GroundTag>& get_task() const;
};

struct StateWithTaskValue : StateWithoutMetadata<p::StateView<tyr::GroundTag>>
{
    p::Task<tyr::GroundTag> get_task() const;
};

struct StateWithMutableAtoms : StateWithoutMetadata<p::StateView<tyr::GroundTag>>
{
    template<tyr::formalism::FactKind F>
    auto get_atoms() { return p::StateView<tyr::GroundTag>::get_atoms<F>(); }
};

template<tyr::TaskKind Kind>
struct StateWithThrowingMove : p::BuilderStateView<Kind>
{
    explicit StateWithThrowingMove(p::BuilderStateView<Kind> state) : p::BuilderStateView<Kind>(state) {}
    StateWithThrowingMove(const StateWithThrowingMove&) = default;
    StateWithThrowingMove(StateWithThrowingMove&& other) : p::BuilderStateView<Kind>(other) { throw std::runtime_error("state move"); }
};

template<tyr::TaskKind Kind>
struct StateWithThrowingPack : p::StateView<Kind>
{
    explicit StateWithThrowingPack(p::StateView<Kind> state) : p::StateView<Kind>(std::move(state)) {}
    p::PackedStateView<Kind> pack() const { throw std::runtime_error("state pack"); }
};

using StateKinds = ygg::TypeList<tyr::GroundTag, tyr::LiftedTag>;
static_assert([]<typename... Kinds>(ygg::TypeList<Kinds...>) { return (StateIndexContract<Kinds> && ...); }(StateKinds {}));
static_assert([]<typename... Kinds>(ygg::TypeList<Kinds...>) { return (std::totally_ordered<p::PackedStateView<Kinds>> && ...); }(StateKinds {}));
static_assert(
    []<typename... Kinds>(ygg::TypeList<Kinds...>)
    {
        return ((p::StateViewConcept<p::StateView<Kinds>, Kinds> && p::StateViewConcept<p::BuilderStateView<Kinds>, Kinds>
                 && p::StateViewConcept<p::StateView<Kinds>&, Kinds> && p::StateViewConcept<const p::StateView<Kinds>&, Kinds>
                 && p::StateViewConcept<p::StateView<Kinds>&&, Kinds> && p::StateViewConcept<p::BuilderStateView<Kinds>&, Kinds>
                 && p::StateViewConcept<const p::BuilderStateView<Kinds>&, Kinds> && p::StateViewConcept<p::BuilderStateView<Kinds>&&, Kinds>
                 && !p::StateViewConcept<volatile p::StateView<Kinds>&, Kinds> && !p::StateViewConcept<volatile p::BuilderStateView<Kinds>&, Kinds>)
                && ...);
    }(StateKinds {}));

template<typename T>
concept HasStateIdentity =
    requires(const T& state) { state.get_index(); } || requires(const T& state) { state.pack(); } || requires(const T& state) { state.get_state_repository(); };

static_assert(
    []<typename... Kinds>(ygg::TypeList<Kinds...>)
    { return ((!HasStateIdentity<p::BuilderStateView<Kinds>> && !p::StateViewConcept<ygg::Builder<p::State<Kinds>>, Kinds>) && ...); }(StateKinds {}));
static_assert(!p::StateViewConcept<int, tyr::GroundTag>);
static_assert(!p::StateViewConcept<const int&, tyr::GroundTag>);
static_assert(p::StateViewConcept<p::BuilderStateView<tyr::GroundTag>, tyr::GroundTag>);
static_assert(p::StateViewConcept<p::StateView<tyr::LiftedTag>, tyr::LiftedTag>);
static_assert(!p::StateViewConcept<p::BuilderStateView<tyr::GroundTag>, tyr::LiftedTag>);
static_assert(!p::StateViewConcept<p::StateView<tyr::LiftedTag>, tyr::GroundTag>);
static_assert(!p::StateViewConcept<const p::BuilderStateView<tyr::GroundTag>&, tyr::LiftedTag>);
static_assert(!p::StateViewConcept<const p::StateView<tyr::LiftedTag>&, tyr::GroundTag>);

static_assert(!p::StateViewConcept<p::StateView<tyr::GroundTag>, void>);
static_assert(!p::StateViewConcept<p::StateView<tyr::GroundTag>, int>);
static_assert(!p::StateViewConcept<StateWithMutableTask, tyr::GroundTag>);
static_assert(!p::StateViewConcept<StateWithTaskValue, tyr::GroundTag>);
static_assert(!p::IterableStateConcept<StateWithMutableAtoms&>);
static_assert(!p::StateViewConcept<StateWithMutableAtoms&, tyr::GroundTag>);
static_assert(
    []<typename... Kinds>(ygg::TypeList<Kinds...>)
    {
        return ((p::StateViewConcept<StateWithoutMetadata<p::StateView<Kinds>>, Kinds>
                 && p::StateViewConcept<const StateWithoutMetadata<p::StateView<Kinds>>&, Kinds>
                 && p::StateViewConcept<StateWithoutMetadata<p::BuilderStateView<Kinds>>&, Kinds>
                 && !p::StateViewConcept<volatile StateWithoutMetadata<p::StateView<Kinds>>&, Kinds>
                 && p::StateBuilderConcept<BuilderWithoutTaskType<Kinds>, Kinds> && p::StateBuilderConcept<BuilderWithoutTaskType<Kinds>&, Kinds>
                 && !p::StateBuilderConcept<const BuilderWithoutTaskType<Kinds>&, Kinds>
                 && !p::StateBuilderConcept<volatile BuilderWithoutTaskType<Kinds>&, Kinds>)
                && ...);
    }(StateKinds {}));

template<tyr::TaskKind Kind>
struct NodeWithoutMetadata
{
    StateWithoutMetadata<p::StateView<Kind>> get_state() const;
    ygg::float_t get_metric() const;
};

struct NodeWithMutableState : NodeWithoutMetadata<tyr::GroundTag>
{
    p::StateView<tyr::GroundTag> get_state();
};

static_assert(
    []<typename... Kinds>(ygg::TypeList<Kinds...>)
    {
        return ((p::NodeConcept<NodeWithoutMetadata<Kinds>, Kinds> && p::NodeConcept<NodeWithoutMetadata<Kinds>&, Kinds>
                 && p::NodeConcept<const NodeWithoutMetadata<Kinds>&, Kinds> && p::NodeConcept<NodeWithoutMetadata<Kinds>&&, Kinds>
                 && !p::NodeConcept<volatile NodeWithoutMetadata<Kinds>&, Kinds>)
                && ...);
    }(StateKinds {}));
template<tyr::TaskKind Kind>
consteval bool concrete_node_borrows_state()
{
    return requires(const p::Node<Kind>& node, const p::Node<Kind, p::BuilderStateView<Kind>>& builder_node) {
        { node.get_state() } -> std::same_as<const p::StateView<Kind>&>;
        { builder_node.get_state() } -> std::same_as<const p::BuilderStateView<Kind>&>;
    };
}

static_assert(concrete_node_borrows_state<tyr::GroundTag>());
static_assert(concrete_node_borrows_state<tyr::LiftedTag>());
static_assert(
    []<typename... Kinds>(ygg::TypeList<Kinds...>)
    {
        return ((std::is_nothrow_constructible_v<p::Node<Kinds>, p::StateView<Kinds>, ygg::float_t>
                 && std::is_nothrow_constructible_v<p::Node<Kinds, p::BuilderStateView<Kinds>>, p::BuilderStateView<Kinds>, ygg::float_t>
                 && !std::is_nothrow_constructible_v<p::Node<Kinds, StateWithThrowingMove<Kinds>>, const StateWithThrowingMove<Kinds>&, ygg::float_t>
                 && noexcept(std::declval<const p::Node<Kinds>&>().pack()) && noexcept(std::declval<const p::LabeledNode<Kinds>&>().pack())
                 && !noexcept(std::declval<const p::Node<Kinds, StateWithThrowingPack<Kinds>>&>().pack())
                 && !noexcept(std::declval<const p::LabeledNode<Kinds, StateWithThrowingPack<Kinds>>&>().pack()))
                && ...);
    }(StateKinds {}));
static_assert(!p::NodeConcept<NodeWithoutMetadata<tyr::GroundTag>, tyr::LiftedTag>);
static_assert(!p::NodeConcept<NodeWithoutMetadata<tyr::GroundTag>, void>);
static_assert(!p::NodeConcept<NodeWithMutableState, tyr::GroundTag>);
static_assert(!p::NodeConcept<NodeWithMutableState&, tyr::GroundTag>);

template<typename Kind, typename State>
concept NodeState = requires { typename p::Node<Kind, State>; };

template<typename Kind, typename State>
concept LabeledNodeState = requires { typename p::LabeledNode<Kind, State>; };

template<typename T>
concept Packable = requires(const T& value) { value.pack(); };

static_assert(
    []<typename... Kinds>(ygg::TypeList<Kinds...>)
    {
        return ((p::NodeConcept<p::Node<Kinds>, Kinds> && std::totally_ordered<p::Node<Kinds>> && Packable<p::Node<Kinds>> && Packable<p::LabeledNode<Kinds>>)
                && ...);
    }(StateKinds {}));
static_assert(
    []<typename... Kinds>(ygg::TypeList<Kinds...>)
    {
        return ((p::NodeConcept<p::Node<Kinds, p::BuilderStateView<Kinds>>, Kinds> && !ygg::Identifiable<p::Node<Kinds, p::BuilderStateView<Kinds>>>
                 && !std::equality_comparable<p::Node<Kinds, p::BuilderStateView<Kinds>>> && !Packable<p::Node<Kinds, p::BuilderStateView<Kinds>>>
                 && !Packable<p::LabeledNode<Kinds, p::BuilderStateView<Kinds>>> && !NodeState<Kinds, Kinds>)
                && ...);
    }(StateKinds {}));
static_assert(!NodeState<int, int>);
static_assert(!NodeState<tyr::GroundTag, p::StateView<tyr::LiftedTag>>);
static_assert(!NodeState<tyr::LiftedTag, p::BuilderStateView<tyr::GroundTag>>);
static_assert(!LabeledNodeState<tyr::GroundTag, p::StateView<tyr::LiftedTag>>);
static_assert(!LabeledNodeState<tyr::LiftedTag, p::BuilderStateView<tyr::GroundTag>>);
static_assert(!p::NodeConcept<p::Node<tyr::GroundTag>, tyr::LiftedTag>);
static_assert(!p::NodeConcept<int, tyr::GroundTag>);

namespace tyr::tests
{
namespace
{
template<TaskKind Kind>
void expect_packed_state_identity()
{
    using Data = ygg::Data<p::State<Kind>>;
    auto facts = p::FactPackedStorage<Kind, p::StateStoragePolicyTag> {};
    auto derived = p::AtomPackedStorage<Kind, p::StateStoragePolicyTag> {};
    auto numeric = p::NumericPackedStorage<Kind, p::StateStoragePolicyTag> {};
    const auto base = Data(facts, derived, numeric);
    const auto hash = ygg::Hash<Data> {};
    derived.index = { 1 };
    const auto extended = Data(facts, derived, numeric);
    EXPECT_NE(base.template get_atom_storage<formalism::DerivedTag>(), extended.template get_atom_storage<formalism::DerivedTag>());
    EXPECT_EQ(base, extended);
    EXPECT_EQ(hash(base), hash(extended));

    facts.index = { 1 };
    const auto changed_facts = Data(facts, derived, numeric);
    EXPECT_NE(base, changed_facts);
    numeric.index = { 1 };
    const auto changed_numeric = Data(base.template get_atom_storage<formalism::FluentTag>(), derived, numeric);
    EXPECT_NE(base, changed_numeric);
}

template<TaskKind Kind>
void expect_state_builder_identity()
{
    using Builder = ygg::Builder<p::State<Kind>>;
    auto base = Builder {};
    if constexpr (std::same_as<Kind, GroundTag>)
        base.template get_atom_storage<formalism::FluentTag>().values = { 1, 0 };
    else
    {
        base.template get_atom_storage<formalism::FluentTag>().indices.resize(2);
        base.template get_atom_storage<formalism::FluentTag>().indices.set(0);
    }
    base.get_numeric_variables().values = { 3.0 };
    auto other = base;
    base.set(ygg::Index<p::State<Kind>>(0));
    other.set(ygg::Index<p::State<Kind>>(1));
    other.template get_atom_storage<formalism::DerivedTag>().indices.resize(3, true);
    EXPECT_TRUE(ygg::EqualTo<Builder> {}(base, other));
    EXPECT_EQ(ygg::Hash<Builder> {}(base), ygg::Hash<Builder> {}(other));

    if constexpr (std::same_as<Kind, GroundTag>)
        other.template get_atom_storage<formalism::FluentTag>().values.front() = 0;
    else
        other.template get_atom_storage<formalism::FluentTag>().indices.flip(0);
    EXPECT_FALSE(ygg::EqualTo<Builder> {}(base, other));
    other = base;
    other.get_numeric_variables().values.front() = 4.0;
    EXPECT_FALSE(ygg::EqualTo<Builder> {}(base, other));
}

template<TaskKind Kind>
void expect_state_builder_lifecycle()
{
    using Builder = ygg::Builder<p::State<Kind>>;
    using Fact = ygg::Data<fp::FDRFact<formalism::FluentTag>>;
    using Atom = ygg::Index<fp::Atom<GroundTag, formalism::DerivedTag>>;
    using Numeric = ygg::Index<fp::FunctionTerm<GroundTag, formalism::FluentTag>>;
    const auto variable = ygg::Index<fp::FDRVariable<formalism::FluentTag>>(0);
    auto source = Builder {};
    auto target = Builder {};
    if constexpr (std::same_as<Kind, GroundTag>)
    {
        source.resize_fluent_facts(1);
        target.resize_fluent_facts(1);
        source.resize_derived_atoms(2);
        target.resize_derived_atoms(2);
    }
    source.set(ygg::Index<p::State<Kind>>(7));
    target.set(ygg::Index<p::State<Kind>>(8));
    source.set(Fact(variable, fp::FDRValue(1)));
    source.set(Numeric(2), 3);
    source.set(Atom(0));
    target.set(Atom(1));
    EXPECT_TRUE(std::isnan(source.get(Numeric(0))));
    EXPECT_TRUE(std::isnan(source.get(Numeric(99))));
    source.set(Numeric(1), std::numeric_limits<ygg::float_t>::quiet_NaN());
    EXPECT_TRUE(std::isnan(source.get(Numeric(1))));

    target.assign_unextended_part(source);
    EXPECT_EQ(target.get_index(), ygg::Index<p::State<Kind>>(8));
    EXPECT_EQ(target.get(variable), fp::FDRValue(1));
    EXPECT_EQ(target.get(Numeric(2)), 3);
    EXPECT_TRUE(std::isnan(target.get(Numeric(0))));
    EXPECT_FALSE(target.test(Atom(0)));
    EXPECT_TRUE(target.test(Atom(1)));

    source.set(Fact(variable, fp::FDRValue::none()));
    source.set(Numeric(2), 4);
    using std::swap;
    swap(source, target);
    EXPECT_EQ(source.get_index(), ygg::Index<p::State<Kind>>(8));
    EXPECT_EQ(source.get(variable), fp::FDRValue(1));
    EXPECT_EQ(source.get(Numeric(2)), 3);
    EXPECT_FALSE(source.test(Atom(0)));
    EXPECT_TRUE(source.test(Atom(1)));
    EXPECT_EQ(target.get_index(), ygg::Index<p::State<Kind>>(7));
    EXPECT_EQ(target.get(variable), fp::FDRValue::none());
    EXPECT_EQ(target.get(Numeric(2)), 4);
    EXPECT_TRUE(target.test(Atom(0)));
    EXPECT_FALSE(target.test(Atom(1)));

    source.clear_extended_part();
    EXPECT_TRUE(source.template get_atom_storage<formalism::DerivedTag>().indices.empty());
    EXPECT_EQ(source.get_index(), ygg::Index<p::State<Kind>>(8));
    EXPECT_EQ(source.get(variable), fp::FDRValue(1));
    EXPECT_EQ(source.get(Numeric(2)), 3);
    target.clear_unextended_part();
    EXPECT_TRUE(target.get_fluent_facts().begin() == target.get_fluent_facts().end());
    EXPECT_TRUE(target.get_numeric_variables().values.empty());
    EXPECT_EQ(target.get_index(), ygg::Index<p::State<Kind>>(7));
    EXPECT_TRUE(target.test(Atom(0)));

    target.clear();
    EXPECT_TRUE(target.get_index().is_max());
    EXPECT_TRUE(target.get_fluent_facts().begin() == target.get_fluent_facts().end());
    EXPECT_TRUE(target.template get_atom_storage<formalism::DerivedTag>().indices.empty());
    EXPECT_TRUE(target.get_numeric_variables().values.empty());
}

template<TaskKind Kind>
auto derived_names(const p::StateView<Kind>& state)
{
    auto result = std::vector<std::string> {};
    for (const auto atom : state.template get_atoms_view<formalism::DerivedTag>())
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
    EXPECT_TRUE(std::ranges::equal(owned->template get_atoms_view<formalism::FluentTag>(*task->get_repository()),
                                   registered.template get_atoms_view<formalism::FluentTag>()));
    EXPECT_TRUE(std::ranges::equal(owned->template get_atoms_view<formalism::DerivedTag>(*task->get_repository()),
                                   registered.template get_atoms_view<formalism::DerivedTag>()));
    EXPECT_TRUE(std::ranges::equal(owned->template get_fterm_values_view<formalism::FluentTag>(*task->get_repository()),
                                   registered.template get_fterm_values_view<formalism::FluentTag>()));
    static_assert(std::same_as<decltype(state), const p::BuilderStateView<Kind>>);
    const auto formatting_view = StateFormattingView<p::BuilderStateView<Kind>> { state };
    static_assert(p::IterableViewStateConcept<decltype(formatting_view)>);
    static_assert(!p::StateViewConcept<decltype(formatting_view), Kind>);
    EXPECT_EQ(fmt::format("{}", formatting_view), fmt::format("{}", state));
    const auto markerless_state = StateWithoutMetadata(state);
    const auto markerless_node = p::Node<Kind, StateWithoutMetadata<p::BuilderStateView<Kind>>>(markerless_state, 3);
    EXPECT_EQ(&markerless_node.get_state().get_task(), task.get());
    EXPECT_TRUE(std::ranges::equal(markerless_state.template get_atoms_view<formalism::FluentTag>(), state.template get_atoms_view<formalism::FluentTag>()));
    const auto node = p::Node<Kind, p::BuilderStateView<Kind>>(state, 3);
    using BorrowedNode = std::remove_cvref_t<decltype(node)>;
    static_assert(std::same_as<typename BorrowedNode::StateType, p::BuilderStateView<Kind>>);
    const auto labeled = p::LabeledNode<Kind, p::BuilderStateView<Kind>> { label, node };
    const auto nodes = p::NodeList<Kind, p::BuilderStateView<Kind>> { node };
    const auto labeled_nodes = p::LabeledNodeList<Kind, p::BuilderStateView<Kind>> { labeled };

    auto binding_data = ygg::Data<formalism::RelationBinding<fp::Action<LiftedTag>>> {};
    binding_data.relation = label.get_relation().get_index();
    for (const auto object : label.get_objects())
        binding_data.objects.push_back(object.get_index());
    const auto borrowed_label = ygg::make_view(binding_data, label.get_context());
    const auto borrowed_labeled = p::LabeledNode<Kind, p::BuilderStateView<Kind>, fp::ActionBindingDataView> { borrowed_label, node };
    const auto indexed_labeled = p::LabeledNode<Kind> { label, p::Node<Kind>(registered, 3) };
    const auto borrowed_indexed_labeled = p::LabeledNode<Kind, p::StateView<Kind>, fp::ActionBindingDataView> { borrowed_label, indexed_labeled.node };
    static_assert(Packable<decltype(indexed_labeled)>);
    static_assert(!Packable<decltype(borrowed_labeled)> && !Packable<decltype(borrowed_indexed_labeled)>);
    const auto borrowed_labeled_nodes = p::LabeledNodeList<Kind, p::BuilderStateView<Kind>, fp::ActionBindingDataView> { borrowed_labeled };
    EXPECT_EQ(&borrowed_labeled_nodes.front().label.get_data(), &binding_data);
    EXPECT_EQ(&borrowed_indexed_labeled.label.get_data(), &binding_data);
    EXPECT_EQ(indexed_labeled.pack().unpack().label, label);
    const auto throwing_move = StateWithThrowingMove<Kind>(state);
    EXPECT_THROW((p::Node<Kind, StateWithThrowingMove<Kind>>(throwing_move, 0)), std::runtime_error);
    const auto throwing_node = p::Node<Kind, StateWithThrowingPack<Kind>>(StateWithThrowingPack<Kind>(registered), 0);
    EXPECT_THROW(throwing_node.pack(), std::runtime_error);
    const auto throwing_labeled = p::LabeledNode<Kind, StateWithThrowingPack<Kind>> { label, throwing_node };
    EXPECT_THROW(throwing_labeled.pack(), std::runtime_error);
    EXPECT_EQ(fmt::format("{}", borrowed_labeled), fmt::format("{}", labeled));
    EXPECT_EQ(fmt::format("{}", borrowed_indexed_labeled), fmt::format("{}", indexed_labeled));

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
    EXPECT_EQ(state.get_formalism_repository(), registered.get_formalism_repository());
    EXPECT_TRUE(std::ranges::equal(state.template get_atoms<::tyr::formalism::StaticTag>(), registered.template get_atoms<::tyr::formalism::StaticTag>()));
    EXPECT_TRUE(std::ranges::equal(state.get_fluent_facts(), registered.get_fluent_facts()));
    EXPECT_TRUE(std::ranges::equal(state.template get_atoms<::tyr::formalism::DerivedTag>(), registered.template get_atoms<::tyr::formalism::DerivedTag>()));
    EXPECT_TRUE(std::ranges::equal(state.template get_fterm_values<::tyr::formalism::StaticTag>(),
                                   registered.template get_fterm_values<::tyr::formalism::StaticTag>()));
    EXPECT_TRUE(std::ranges::equal(state.template get_fterm_values<::tyr::formalism::FluentTag>(),
                                   registered.template get_fterm_values<::tyr::formalism::FluentTag>()));
    EXPECT_TRUE(std::ranges::equal(state.template get_atoms_view<formalism::StaticTag>(), registered.template get_atoms_view<formalism::StaticTag>()));
    EXPECT_TRUE(std::ranges::equal(state.get_fluent_facts_view(), registered.get_fluent_facts_view()));
    EXPECT_TRUE(std::ranges::equal(state.template get_atoms_view<formalism::FluentTag>(), registered.template get_atoms_view<formalism::FluentTag>()));
    EXPECT_TRUE(std::ranges::equal(state.template get_atoms_view<formalism::DerivedTag>(), registered.template get_atoms_view<formalism::DerivedTag>()));
    const auto check_filtered = [&]<formalism::FactKind F>()
    {
        const auto indices = registered.template get_atoms_view<F>() | std::views::transform([](auto atom) { return atom.get_index(); });
        EXPECT_TRUE(std::ranges::equal(registered.template get_atoms<F>(), indices));
        EXPECT_TRUE(std::ranges::equal(state.template get_atoms<F>(), indices));
        if constexpr (!std::same_as<F, formalism::StaticTag>)
        {
            EXPECT_TRUE(std::ranges::equal(owned->template get_atoms<F>(*task->get_repository()), indices));
        }
        for (const auto atom : registered.template get_atoms_view<F>())
        {
            auto expected = std::vector<fp::AtomView<GroundTag, F>> {};
            for (const auto candidate : registered.template get_atoms_view<F>())
                if (candidate.get_predicate() == atom.get_predicate())
                    expected.push_back(candidate);
            // The temporary state and predicate wrappers expire before traversal.
            auto borrowed = ygg::make_view(*owned, *task).get_atoms_view(atom.get_predicate());
            auto indexed = p::StateView<Kind>(registered).get_atoms_view(atom.get_predicate());
            EXPECT_TRUE(std::ranges::equal(borrowed, expected));
            EXPECT_TRUE(std::ranges::equal(indexed, expected));
        }
        auto foreign_repository = task->get_domain().get_repository_factory()->create();
        auto foreign_data = ygg::Data<formalism::Predicate<F>>(std::string("foreign"), 0);
        const auto foreign = fp::insert(foreign_repository, foreign_data).first;
        auto absent = state.get_atoms_view(foreign);
        EXPECT_TRUE(absent.begin() == absent.end());
    };
    check_filtered.template operator()<formalism::StaticTag>();
    check_filtered.template operator()<formalism::FluentTag>();
    check_filtered.template operator()<formalism::DerivedTag>();
    EXPECT_TRUE(std::ranges::equal(state.template get_fterm_values_view<::tyr::formalism::StaticTag>(),
                                   registered.template get_fterm_values_view<::tyr::formalism::StaticTag>()));
    EXPECT_TRUE(std::ranges::equal(state.template get_fterm_values_view<::tyr::formalism::FluentTag>(),
                                   registered.template get_fterm_values_view<::tyr::formalism::FluentTag>()));
    for (const auto atom : registered.template get_atoms_view<formalism::StaticTag>())
    {
        EXPECT_EQ(state.test(atom.get_index()), registered.test(atom));
        EXPECT_EQ(state.test(atom), registered.test(atom));
    }
    for (const auto fact : registered.get_fluent_facts_view())
    {
        EXPECT_EQ(state.get(fact.get_variable().get_index()), fact.get_value());
        EXPECT_EQ(state.get(fact.get_variable()), fact.get_value());
        EXPECT_TRUE(state.test(*fact.get_atom()));
        EXPECT_TRUE(registered.test(*fact.get_atom()));
        EXPECT_EQ(fact.get_atom_index(), fact.get_atom()->get_index());
        const auto unassigned = ygg::make_view(ygg::Data<fp::FDRFact<formalism::FluentTag>>(fact.get_variable().get_index(), fp::FDRValue::none()),
                                               fact.get_context());
        EXPECT_FALSE(unassigned.get_atom_index());
        EXPECT_FALSE(unassigned.get_atom());
    }
    for (const auto atom : registered.template get_atoms_view<formalism::DerivedTag>())
    {
        EXPECT_EQ(state.test(atom.get_index()), registered.test(atom));
        EXPECT_EQ(state.test(atom), registered.test(atom));
    }
    for (const auto& [fterm, value] : registered.template get_fterm_values_view<::tyr::formalism::StaticTag>())
    {
        EXPECT_EQ(state.get(fterm.get_index()), value);
        EXPECT_EQ(state.get(fterm), value);
    }
    for (const auto& [fterm, value] : registered.template get_fterm_values_view<::tyr::formalism::FluentTag>())
    {
        EXPECT_EQ(state.get(fterm.get_index()), value);
        EXPECT_EQ(state.get(fterm), value);
    }

    const auto num_variables = task->get_fdr_context()->get_variables().size();
    auto predicate_data = ygg::Data<formalism::Predicate<formalism::FluentTag>>(std::string("unregistered"), 0);
    auto atom_binding_data = ygg::Data<formalism::RelationBinding<formalism::Predicate<formalism::FluentTag>>> {};
    atom_binding_data.relation = fp::insert(*task->get_repository(), predicate_data).first.get_index();
    auto atom_data = ygg::Data<fp::Atom<GroundTag, formalism::FluentTag>>(fp::insert(*task->get_repository(), atom_binding_data).first.get_index());
    const auto absent_atom = fp::insert(*task->get_repository(), atom_data).first;
    EXPECT_FALSE(state.test(absent_atom));
    EXPECT_FALSE(registered.test(absent_atom));
    EXPECT_EQ(task->get_fdr_context()->get_variables().size(), num_variables);

    auto moved = std::move(owned);
    EXPECT_FALSE(owned);
    EXPECT_EQ(&state.get_state_builder(), moved.get());
    const auto numeric = registered.template get_fterm_values_view<::tyr::formalism::FluentTag>();
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
        auto& values = moved->template get_atom_storage<formalism::FluentTag>().values;
        std::ranges::fill(values, ygg::uint_t(0));
    }
    else
        moved->template get_atom_storage<formalism::FluentTag>().indices.clear();
    const auto atoms = moved->template get_atoms_view<formalism::FluentTag>(*task->get_repository());
    EXPECT_TRUE(atoms.begin() == atoms.end());
    const auto indices = moved->template get_atoms<formalism::FluentTag>(*task->get_repository());
    EXPECT_TRUE(indices.begin() == indices.end());
    for (const auto atom : registered.template get_atoms_view<formalism::FluentTag>())
    {
        auto filtered = state.get_atoms_view(atom.get_predicate());
        EXPECT_TRUE(filtered.begin() == filtered.end());
        EXPECT_FALSE(state.test(atom));
        EXPECT_TRUE(registered.test(atom));
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
        EXPECT_TRUE(std::ranges::equal(unpacked.template get_fterm_values<::tyr::formalism::FluentTag>(),
                                       state.template get_fterm_values<::tyr::formalism::FluentTag>()));

        auto builder = repository->get_state_builder();
        builder->assign_unextended_part(state.get_state_builder());
        const auto derived = builder->template get_atoms<formalism::DerivedTag>(*task->get_repository());
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
        const auto borrowed = p::Node<Kind, p::BuilderStateView<Kind>>(ygg::make_view(*owned, *task), cheap->node.get_metric());
        EXPECT_EQ(generator->get_applicable_action_bindings(borrowed), bindings);
        const auto expect_same_node = [](const auto& actual, const auto& expected)
        {
            EXPECT_EQ(actual.get_metric(), expected.get_metric());
            EXPECT_TRUE(std::ranges::equal(actual.get_state().get_fluent_facts(), expected.get_state().get_fluent_facts()));
            EXPECT_TRUE(std::ranges::equal(actual.get_state().template get_atoms<::tyr::formalism::DerivedTag>(), expected.get_state().template get_atoms<::tyr::formalism::DerivedTag>()));
            EXPECT_TRUE(std::ranges::equal(actual.get_state().template get_fterm_values<::tyr::formalism::FluentTag>(),
                                           expected.get_state().template get_fterm_values<::tyr::formalism::FluentTag>()));
        };
        auto generated = pool.get_or_allocate();
        const auto successor = generator->get_successor_node(borrowed, *raise, *generated, *axioms);
        static_assert(std::same_as<decltype(successor), const p::Node<Kind, p::BuilderStateView<Kind>>>);
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
        static_assert(std::same_as<decltype(labeled), const p::LabeledNodeList<Kind, p::BuilderStateView<Kind>>>);
        static_assert(std::same_as<decltype(nodes), const p::NodeList<Kind, p::BuilderStateView<Kind>>>);
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
                                                                   static_assert(std::same_as<decltype(next), p::LabeledNode<Kind, p::BuilderStateView<Kind>>>);
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
                                                            static_assert(std::same_as<decltype(next), p::Node<Kind, p::BuilderStateView<Kind>>>);
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
        {
            EXPECT_EQ(generator->get_packed_successor_node(borrowed, generator->ground_action(*raise), *repository, *axioms), raised.pack());
        }
        // Ground axiom evaluation expects the capacity normally prepared by the state repository.
        if constexpr (std::same_as<Kind, GroundTag>)
            generated->resize_derived_atoms(task->get_task().template get_atoms<formalism::DerivedTag>().size());
        generator->generate_successor_state(borrowed, *raise, *generated);
        axioms->compute_extended_state(*generated);
        const auto generated_view = ygg::make_view(*generated, *task);
        EXPECT_TRUE(std::ranges::equal(generated_view.get_fluent_facts(), raised.get_state().get_fluent_facts()));
        EXPECT_TRUE(std::ranges::equal(generated_view.template get_atoms<::tyr::formalism::DerivedTag>(), raised.get_state().template get_atoms<::tyr::formalism::DerivedTag>()));
        EXPECT_TRUE(std::ranges::equal(generated_view.template get_fterm_values<::tyr::formalism::FluentTag>(),
                                       raised.get_state().template get_fterm_values<::tyr::formalism::FluentTag>()));
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

TEST(TyrPlanningStateTest, BuilderPartsRetainIndependentDataAndRegistration)
{
    expect_state_builder_lifecycle<GroundTag>();
    expect_state_builder_lifecycle<LiftedTag>();
}

TEST(TyrPlanningStateTest, GroundBuilderRetainsNonbinaryFactsAndPresizedDerivedAtoms)
{
    auto builder = ygg::Builder<p::State<GroundTag>> {};
    const auto variable = ygg::Index<fp::FDRVariable<formalism::FluentTag>>(1);
    const auto derived = ygg::Index<fp::Atom<GroundTag, formalism::DerivedTag>>(64);
    builder.resize_fluent_facts(2);
    builder.resize_derived_atoms(130);
    EXPECT_EQ(builder.get(variable), fp::FDRValue::none());
    EXPECT_FALSE(builder.test(derived));
    builder.set(ygg::Data<fp::FDRFact<formalism::FluentTag>>(variable, fp::FDRValue(37)));
    builder.set(derived);
    EXPECT_EQ(builder.get(variable), fp::FDRValue(37));
    EXPECT_TRUE(builder.test(derived));
    EXPECT_FALSE(builder.test(ygg::Index<fp::Atom<GroundTag, formalism::DerivedTag>>(129)));
    EXPECT_EQ(builder.get_atom_storage<formalism::DerivedTag>().indices.size(), 130);
}

TEST(TyrPlanningStateTest, LiftedBuilderGrowsBinaryStorageAndTrimsClearedFacts)
{
    using Fact = ygg::Data<fp::FDRFact<formalism::FluentTag>>;
    auto builder = ygg::Builder<p::State<LiftedTag>> {};
    const auto low = ygg::Index<fp::FDRVariable<formalism::FluentTag>>(1);
    const auto high = ygg::Index<fp::FDRVariable<formalism::FluentTag>>(130);
    const auto derived = ygg::Index<fp::Atom<GroundTag, formalism::DerivedTag>>(130);
    EXPECT_EQ(builder.get(high), fp::FDRValue::none());
    EXPECT_FALSE(builder.test(derived));
    builder.set(Fact(high, fp::FDRValue::none()));
    EXPECT_TRUE(builder.get_atom_storage<formalism::FluentTag>().indices.empty());
    builder.set(Fact(low, fp::FDRValue(1)));
    builder.set(Fact(high, fp::FDRValue(1)));
    builder.set(derived);
    EXPECT_EQ(builder.get(high), fp::FDRValue(1));
    EXPECT_TRUE(builder.test(derived));
    EXPECT_FALSE(builder.test(ygg::Index<fp::Atom<GroundTag, formalism::DerivedTag>>(131)));
    builder.set(Fact(high, fp::FDRValue::none()));
    EXPECT_EQ(builder.get(high), fp::FDRValue::none());
    EXPECT_EQ(builder.get(low), fp::FDRValue(1));
    EXPECT_EQ(builder.get_atom_storage<formalism::FluentTag>().indices.size(), 2);
    builder.set(Fact(low, fp::FDRValue::none()));
    EXPECT_TRUE(builder.get_atom_storage<formalism::FluentTag>().indices.empty());
    EXPECT_TRUE(builder.test(derived));
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
