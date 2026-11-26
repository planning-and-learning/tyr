# tyr: state representation and successor generation for loki 1.15's formalisms

## Context

loki 1.15 parses nondeterministic (`oneof`) and probabilistic effects (PPDDL: rewards, probabilistic initial states),
durative actions and timed initial literals (PDDL 2.1/2.2), PDDL 3 constraints and preferences, and trajectory formulas
(LTLf/PLTLf/MTLf). tyr supports none of them: `src/formalism/planning/loki_to_tyr.cpp` throws on oneof/probabilistic/
timed effects and timed/preference conditions (`:163, :204, :580, :759`) and silently drops durative actions,
constraints, timed initial literals, probabilistic initial effects and `goal_reward` (`translate(DomainView)
:1193-1234`, `translate(TaskView) :1236-1328`). Successor generation yields exactly one successor per action binding,
with one float metric (`Node{state, metric}`, `LabeledNode{binding, node}`), and a state is four keys (index, facts,
derived atoms, numeric values).

This plan extends tyr's **state representation** and **successor generation**, for both fully grounded (`GroundTag`)
and fully lifted (`LiftedTag`) tasks, following the design agreed with the user (also in `tyr/docs/PLAN_ENVIRONMENTS.md`,
untracked). Search algorithms for policies (FOND, MDP), temporal search and heuristics are out of scope here; the
successor generators are tested directly.

## Design (fixed)

- **Environment tag per task, nothing else tagged.** `DeterministicTag | NondeterministicTag | ProbabilisticTag` (named
  concept `Environment`), decided from the task's requirements; mixing `oneof` and `probabilistic` is rejected with an
  error. Every other construct is an optional component of states or an optional part of actions, usable in any
  environment.
- **State: one type per Kind**, two new identifying keys to optional components, each pointing at one shared empty record
  when unused: `agenda_storage` (temporal) and `monitor_storage` (trajectory constraints and preferences). Absolute time
  and penalties are not state: they are node data (time next to the metric/g).
- **Actions: one type per Kind**, optional parts behind keys. Uncertain effects follow loki's factored normal form
  (`(when c (oneof a b))`, `factor_shared_condition`): a conditional effect may hold a *choice* among alternatives
  instead of one effect. An action's outcome = one alternative per firing choice; outcomes are enumerated lazily as the
  product. Deterministic actions have no choices and are unchanged.
- **Successor generation: one core, packaged per environment.** A successor is labelled by a *transition*: apply an
  action (with an outcome assignment), start a durative action, or advance time to the next agenda event. Ground and
  lifted share the executor; the environment only decides how outcomes are grouped (one, a set, a distribution).
- **Constraints and preferences tracked natively**, never compiled into actions. Ground grounds their instances up front
  (with reachability); Lifted interns instances on the fly when an effect first changes one.

## Revised decisions (2026-11-25, supersede the environment tags above)

- **No environment templates.** `planning::Task<Kind>` is untemplated again (step 1's `Task<Kind, E>`, `make_task`,
  `LiftedTaskVariant` and the extra Python task classes are removed). Which kinds of choices a task has (`oneof`,
  `probabilistic`, both, neither) is a runtime property; algorithms check it. Nothing is rejected for mixing.
- **Records (step 2, extended):** one `Action<T>` with `choices`; `ChoiceEffect<T>` = variables, condition, outcomes,
  probabilities (empty: `oneof`); `Outcome<T>` = flat effects **and nested choices** (loki's normal form flattens only
  same-kind nests, so `oneof` inside `probabilistic` and the reverse reach tyr). Nested choices are scoped like outcome
  effects (enclosing variables and conditions included). Loops over an action's effects recurse with a path (choice,
  outcome per level); an invariant's delete balances an add iff the delete's path is a prefix of the add's.
- **Successor interface: two levels, a set of distributions** (MDPST: the adversary picks a distribution, chance draws an
  outcome). Any alternating nest flattens exactly: inner adversary levels become strategies (one distribution per
  combination of picks over the chance branches), outer ones a union, nested chance levels multiply. Assumes the adversary
  sees the chance outcome. Deterministic: 1 distribution of 1 outcome (probability 1); `oneof`: k distributions of 1;
  `probabilistic`: 1 of k; side-by-side choices multiply. Equal states within a distribution are merged.
  ```cpp
  struct Transition   { kind (apply | start | advance); Binding binding; };   // start/advance in step 4
  struct OutcomeNode  { Node node; std::optional<ygg::float_t> probability; bool failed; };  // failed: over-all / hard constraint violated
  struct Distribution { std::span<const OutcomeNode> outcomes; };
  struct Successor    { Transition label; std::span<const Distribution> distributions; };
  ```
  States are interned in the repository or written into pooled builders (`SuccessorBuilders`), as today; the existing
  deterministic API stays for classical search and throws on a task with choices.
- **Temporal (step 4)** fits the same interface: start resolves at-start choices, an advance resolves the due end effects'
  choices together (independent: they multiply).

## Temporal model: complete (2026-11-26, supersedes decision epochs)

No fragments: the temporal component is a simple temporal network over the happenings that can still constrain the future,
with snap transitions, so that every valid PDDL 2.1/2.2 plan (required concurrency, starts at any time, duration
inequalities, self-overlap, timed initial literals) is reachable; linear continuous change via linear programming; nonlinear
continuous change via a nonlinear real-arithmetic solver (last).

- **Happenings and transitions.** A start snap (at-start condition, effects, choices; opens the action), an end snap of a
  specific open action (at-end condition, effects, choices; closes it), an instantaneous action, a timed initial literal
  (fixed time). Search order is execution order of the happenings; times stay flexible within the network.
- **Network.** Timepoints: the origin (time 0), and one per happening still on the frontier. Constraints: t_j - t_i <= d
  (minimal network: all-pairs shortest paths, kept closed; consistency = no negative cycle). Durations lo <= end - start
  <= hi (=, <=, >=, intervals; ?duration evaluated at the start); timed initial literals at fixed offsets from the origin;
  ordering t_j - t_i >= epsilon (PDDL 2.1's separation) between interfering happenings, t_j >= t_i otherwise.
- **Interference (what orders a new happening after earlier ones).** Per fluent fact and numeric fluent, the frontier
  keeps its last writer and its readers since: a new reader is after the writer; a new writer is after the writer and the
  readers (epsilon); over-all conditions of open actions are readers until their end; deleting a fact an open action
  protects over all is not allowed (its end is not applied yet). Numeric writers and readers likewise.
- **Frontier and projection.** After each transition, happenings no record refers to are projected out of the minimal
  network (exact for simple temporal networks), so the component's size is bounded by the records, not the plan length.
- **Canonical form.** Frontier timepoints are ordered by their roles (origin; open actions' starts by action; last writers
  and readers by fact or numeric fluent); the component is the roles plus the projected distance matrix in that order. Two
  states are equal iff facts, numerics and this component are equal: duplicate detection stays exact.
- **Metric.** The node holds the earliest time of its latest happening (the makespan lower bound); total-time reads it,
  total-cost the accumulator; any metric expression over both.
- **?duration and timed conditional effects.** An open action keeps its duration bounds (a network edge) and the end effects
  its start triggered ((when (at start c) (at end e)): c evaluated at the start, per binding).
- **Continuous effects.** Each open action's linear rates; a numeric condition or effect at a happening becomes a linear
  constraint over the timepoints; consistency via an LP (when continuous effects occur), the network otherwise.

## Steps (each ends green and stops for the user's review; no commit or push before it)

### 1. Translation stops dropping silently; environment tags and typed tasks
- `loki_to_tyr.cpp`: every construct not yet supported is rejected with an error naming it (durative actions,
  constraints, timed initial literals, initial effects, goal reward, preferences), replacing the silent drops. Later
  steps remove the errors one by one.
- Tags + concept `Environment` beside `GroundTag`/`LiftedTag` (`include/tyr/common/...` where `TaskKind` lives).
- `planning::Task<Kind, E>` (today `Task<Kind>`: `include/tyr/planning/ground/task.hpp:36`, `lifted/task.hpp:36`); the
  task builder decides `E` from loki's requirements (`:non-deterministic` → Nondeterministic; `:probabilistic-effects`,
  `:rewards` → Probabilistic; else Deterministic) and returns `std::variant<Task<Kind, D>, Task<Kind, N>, Task<Kind, P>>`.
  `instantiate_ground_task` (`src/planning/lifted/task_grounder.cpp:506`) keeps `E`. All existing code uses
  `DeterministicTag`; behaviour unchanged. Python: three task classes per Kind.

### 2. Uncertain effects in the formalism
- New records (S1–S5: data, builder, view, canonicalization, formatter, bindings), lifted and ground, in
  `include/tyr/formalism/planning/`: `Choice<T>` = `{ alternatives: flat_vector<index<ConjunctiveEffect<T>>>,
  probabilities: flat_vector<float_t> (empty for oneof) }`.
- `ConditionalEffect<T>` (`conditional_effect_data.hpp:36-99`) gains `choice: key<Choice<T>>` (the shared empty choice
  when deterministic); `effect` stays the deterministic part.
- `loki_to_tyr.cpp :660-811`: accept `Effect<OneOfTag>` / `Effect<ProbabilisticTag>` under `forall`/`when` and
  translate them to choices; `prepare(EffectView) :169-211` stops throwing for them.
- Grounder (`grounder.hpp :127-191`): ground choices per effect binding like conjunctive effects.
- Probabilistic initial states (`initial_effects`): the task holds them as choices over initial facts; `get_initial_state`
  (`state_repository.cpp:272`) becomes `for_each_initial_state(outcome → state, probability)`; deterministic tasks have
  one. Rewards: `goal_reward` and `(reward)` become an ordinary numeric fluent and the metric (maximize).

### 3. Successors with outcomes
- `ActionExecutor` (`src/planning/action_executor.cpp :75-216`): `process_effects` / `apply_lifted_effect` take an
  **outcome assignment** (alternative index per firing choice, in firing order) and apply that alternative of each
  firing choice next to the deterministic effects. `is_applicable_if_fires` checks every alternative's numeric effects.
- `for_each_outcome(state, binding, callback)` enumerates assignments lazily (product over firing choices), with the
  outcome's probability (product) for Probabilistic.
- Successor generators (`include/tyr/planning/{ground,lifted}/successor_generator.hpp`, pimpl): the shared Impl stays
  untemplated on `E`; a thin header template `SuccessorGenerator<Kind, E>` packages results:
  `Successor<Kind, D> = LabeledNode` (unchanged API), `Successor<Kind, N> = {label, range of Node}`,
  `Successor<Kind, P> = {label, range of (probability, Node)}`. The label `Transition` = `{binding, outcome
  assignment}` (later also start/advance). The two-phase API (`generate_successor_state` +
  `finalize_successor_state`) takes the outcome assignment.
- Plan reconstruction (`search_space/sequential.hpp :35-131`, matches successors per binding) also matches the outcome.
- Lifted: `ApplicableActionProgram` (`src/planning/lifted/programs/action.cpp:64-145`) unchanged (applicability does not
  depend on outcomes); `EffectAtomIndex` reports firing effects with their choices.

### 4. Agenda component and durative actions
- Formalism: `Action<T>` gains `temporal_part: key<TemporalPart<T>>` = `{ duration (ground: value or expression over the
  binding, evaluated at start), over_all: ConjunctiveCondition, end_condition: ConjunctiveCondition, end_effects:
  flat_vector<ConditionalEffect> }`; the inline `condition`/`effects` are the at-start part. `loki_to_tyr` maps
  `DurativeAction` (`TimedTag` conditions/effects split by `TimeSpecifier`) into `Action` with a temporal part, and
  timed initial literals into the task (sorted by time). Grounder grounds temporal parts.
- State component `Agenda<Kind>` = `{ events: flat_vector<{float_t offset, event}>` sorted by `(offset, event)`,
  `til_progress: uint_t }`; `event` = the running action (Ground: dense ground action index; Lifted: the interned
  action binding row) or the next timed initial literal. Stored like numeric values: `FloatVectorSet`-style hash set in
  every storage policy (`state_storage/numeric.hpp:194-243` is the pattern).
- Touch points for a new component (from exploration): `state_data.hpp` (record; `data<State>` field, constructor,
  `fields`, `identifying_members`, `cista_members`), `declarations.hpp` (tag with `state_storage = true`, view alias),
  component builder (`state_storage.hpp`), read mixin (`state_storage_view.hpp`), `state_builder.hpp` (member, clear,
  assign, swap, fields, identifying, functions), `state_view.hpp` (getter, `hash_unextended`/`equal_unextended`,
  `describe_fields`), each policy's pools and context, `state_repository.hpp/.cpp` (`KeyTypes`, `PartStores`,
  `PartKeys`, `OwnParts`, `own_parts`, Evaluator pool, `insert_*` **before `find` at :188**, `unpack_storages`,
  `YGG_FORMALISM_PART` in header and source, `get_initial_state`), `apply_action_unregistered_impl` (copy from parent).
- Transitions (shared core, both Kinds): **start** a durative action whose at-start condition holds (existing match
  tree / `app_` predicate on `condition`): apply at-start effects, check over-all on the result, schedule its end event
  at its duration; **advance**: take the smallest offset, subtract it from every event, apply all end events due
  (end condition, end effects, over-all of the others still running) and due timed initial literals; cost = the elapsed
  time. Instantaneous actions apply as today, checking running actions' over-all conditions. Node time = parent time +
  elapsed (next to the metric; `node.hpp:49-112`).
- `StateContext<Kind>` (`applicability_decl.hpp:34-90`) gets the agenda view for over-all checks. `TemporalStateView`
  (+ `time()`, `events()`, `running()`) with a named concept for code that needs it.

### 5. Monitor component, constraints and preferences
- Formalism: task gains constraint instances (`Constraint` records translated from loki's `Constraint<…Tag>` incl.
  `Modal` operators and trajectory formulas) and preferences: goal preferences (task), precondition preferences
  (`Action<T>.preference_part`), trajectory preferences (constraints flagged as soft); metric `is-violated` weights
  extracted for a linear metric, a nonlinear one rejected with an error.
- Alternating automaton per constraint (one state per subformula); `Monitor<Kind>` component = bit words over
  (instance, subformula), all-zero = initial; stored like derived atoms (bitset trees).
- **Ground**: instances grounded at task construction (`instantiate_ground_task`), pruned by reachability (unreachable
  `sometime` atom → constant penalty or unsolvable; `always` over static atoms decided); dense indices; per ground action
  a table of the instances it advances.
- **Lifted**: instances interned on the fly when an applied effect unifies with a constraint's atom pattern; a never
  interned instance reads as its initial state; pending counts = total instances (type sizes or a Datalog count) minus
  interned ones; nested quantifiers evaluated as queries at trigger time.
- Monitor step after the transition's effects, before the repository lookup (it is identifying). If a task's
  constraints read derived predicates, the axiom closure runs before the lookup for that task (today it runs only for
  new states, `state_repository.cpp:191-202`). A rejecting hard-constraint instance discards the successor; a violated
  preference adds its penalty to the successor's metric; precondition preferences add theirs per application; goal
  preferences are evaluated at goal states.

## Critical files
- Formalism: `include/tyr/formalism/planning/{action,conditional_effect,conjunctive_effect,task,domain,metric}_data.hpp`
  (+ builder, view, canonicalization, formatter), `grounder.hpp`, `src/formalism/planning/loki_to_tyr.{hpp,cpp}`,
  `parser.hpp`.
- State: `include/tyr/planning/{state_data,state_builder,state_view,state_storage,state_storage_view,declarations,
  state_repository,state_delta}.hpp`, `state_storage/` policies, `src/planning/state_repository.cpp`.
- Successors: `include/tyr/planning/{action_executor,applicability,applicability_decl,successor_generator,
  successor_generator_concepts,node,plan}.hpp`, `{ground,lifted}/successor_generator.hpp`, `src/planning/
  {action_executor.cpp, ground/successor_generator.cpp, lifted/successor_generator.cpp, lifted/programs/action.cpp,
  lifted/effect_atom_index.hpp, lifted/task_grounder.cpp}`, `include/tyr/planning/{ground,lifted}/task.hpp`.
- Python bindings under `python/src/pytyr/` for every new record and the task classes.

## Rules
brokkr's schema rules S1–S5 for every record, T20 (named type lists), T17, determinism D1–D3; no special cases, explicit
C++ style (named concepts, written-out return types), no silent fallbacks; tests follow code. Builds `-j24` at most, one at
a time, under `systemd-run --user --scope -p MemoryMax=…`; build, ctest, wheel, pytest as separate steps; no precompiled
headers. No commit or push before the user's review of each step.

## Verification
- Every step: tyr build, ctest, wheel, pytest; peachycheck; `python -m pybrokkr.rules S T20 L I4`.
- Classical behaviour unchanged: the existing benchmark runs (`benchmarks/tools/runner.py`, `compare.py`) give identical
  expansions and plan costs, time within 10%.
- New tests per step, Ground and Lifted each, using pypddl-datasets ≥ 0.0.22 (`nondeterministic/`, `probabilistic/`,
  `temporal/` test families) and hand-written tasks: successor counts and outcome probabilities per action on small
  FOND/PPDDL tasks (step 3); start/advance transitions, agenda contents, over-all violations and timed initial literals
  on small temporal tasks (step 4); pruning by hard constraints and penalties of each preference kind (step 5); mixed
  `:action`/`:durative-action`; rejection of `oneof`+`probabilistic` and of still unsupported constructs.

## Progress log (temporal, complete model)

- Done: `TemporalNetwork` (include/tyr/planning/temporal_network.hpp, minimal STN, closure, projection; test
  tests/unit/planning/temporal_network.cpp). `TemporalStorage<Kind>` component (renamed from the decision-epoch agenda):
  record holds `TemporalWords` (packed canonical words), builder holds `TemporalParts` (open actions, roles, til progress,
  network) — temporal_parts.hpp, state_storage/temporal.{hpp,cpp}. Translation of durative actions (`TemporalPart<T>`).
- Next: snap transitions over the component, ground and lifted (src/planning/{ground,lifted}/successor_generator.cpp
  still hold the decision-epoch code: replace). Per happening: reads (condition and effect-condition FDR variables; derived
  literals read all), writes (the executor's fired add/del effects and numeric writes), orderings t_new - t >= epsilon
  after interfering writers/readers/over-all roles, END: lo <= t - start <= hi, TIL: t = time; consistency; roles update;
  projection onto origin + open starts + role timepoints; canonical renumbering. TransitionKind {APPLY, START, END,
  TIMED_LITERAL}, Transition carries the open action's position for END. Over-all post-check of open actions. Makespan:
  metric total-time = max(parent, earliest(new)). Then TIL translation (task records), ?duration, timed when, node clock.
- Done (2026-11-26): snap transitions over the network, ground and lifted (`TemporalStepper` in
  src/planning/temporal_transitions.hpp; `ground_transition`/`action_transition`/`end_transition` and lifted
  counterparts; `get_end_successor`; TransitionKind {APPLY, START, END, TIMED_LITERAL}). Tests:
  tests/unit/planning/temporal_successors.cpp (snaps, over-all protection, goal at depth 4, makespan 2), 38/38 green.
- Done (2026-11-26), step 1 complete: duration inequalities (min/max and at-end bounds, `duration_bounds`), timed
  initial literals (`TimedLiteral` record, `Task::timed_literals` sorted by time, TIMED_LITERAL transition fixed at its
  time; a predicate only TILs change is fluent for that task: `Parser::parse_task` retranslates the domain with
  `LokiToTyrTranslator::make_fluent`), over-all start ordered non-strictly after its supporting writers. Tests: TIL
  window, required concurrency and self-overlap, duration bounds. ctest 38/38, pytest 191 passed, peachycheck and
  `pybrokkr.rules S L I4 T20` clean. Sweep: 84 temporal domains translate and ground; left: 4 `?duration` in effects,
  4 constraints (steps 2 and 5).
- Next (step 2): `?duration` in effects/conditions, timed `when`, node clock and general metrics (total-cost with
  total-time).
