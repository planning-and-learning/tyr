# tyr: environments, state components and action parts for loki 1.15's formalisms

Plan for implementing in tyr (`/home/dominik/projects/code/planning-and-learning/tyr`) the formalisms loki 1.15 parses:
nondeterministic (`oneof`) and probabilistic effects (PPDDL, rewards, probabilistic initial states), durative actions and
timed initial literals (PDDL 2.1/2.2), PDDL 3 constraints and preferences, trajectory formulas (LTLf, PLTLf, MTLf). The
design below was decided with the user; implement it as written, step by step, and stop for review after each step.

## Decisions (fixed)

1. **One environment tag per task, nothing else gets a tag.** `Environment` = `DeterministicTag` | `NondeterministicTag` |
   `ProbabilisticTag` (top-level in `tyr`, beside `GroundTag`/`LiftedTag`, each with `static constexpr std::string_view
   name`; a named concept `Environment`). It decides only how outcomes are packaged and what a solution is. Every other
   construct (temporal, numeric, derived predicates, conditional effects, constraints, preferences) can occur in every
   environment and is an optional component of states or an optional part of actions. A task mixing `oneof` and
   `probabilistic` is rejected with an error (that model, MDPSTs, would be a later `ImpreciseTag`; not now).
2. **The task carries the environment in its type.** `PlanningTask<Kind, E>`; building a task from a loki task decides
   `E` from its requirements (`:non-deterministic` → Nondeterministic, `:probabilistic-effects`/`:rewards` →
   Probabilistic, else Deterministic) and returns `std::variant<PlanningTask<Kind, DeterministicTag>,
   PlanningTask<Kind, NondeterministicTag>, PlanningTask<Kind, ProbabilisticTag>>`. Callers `std::visit` once. No
   function throws later because of a mismatched environment: mismatches are compile errors.
3. **Successor generation is one core, packaged per environment.** `SuccessorGenerator<Kind, E>` is constructible only
   from `PlanningTask<Kind, E>`. It yields `Successor<Kind, E>`:
   - Deterministic: `{ transition, state, cost }`
   - Nondeterministic: `{ transition, range of states, cost }` (outcomes grouped per action, lazily enumerated where
     factored `oneof`s multiply)
   - Probabilistic: `{ transition, range of (probability, state), cost }`

   A transition is: applying an action; starting a durative action; or advancing time to the next agenda event (which
   applies the due end snaps). Cost: the action cost, 0 for a start, the elapsed time for an advance. Matching,
   application of effects, numeric updates, axioms, agenda and monitor updates and interning are shared; `E` only
   packages. Search algorithms, search spaces and the solution (plan vs policy) are per `E`. No time tag: a task without
   durative actions has an empty agenda and produces no start/advance transitions.
4. **State: one type per Kind, extended by components the constructs need.** `data<State<Kind>>` gets two new keys
   (4 bytes each), pointing to a shared empty record when the task does not use the construct:
   `{ index, fact_storage, atom_storage, numeric_storage, agenda_storage, monitor_storage }`. Both new keys are
   identifying members (deduplication covers them).
   - **Agenda** (`Agenda<Kind>`): `flat_vector<Event>` sorted by `(offset, event)` (canonical) + `uint_t til_progress`.
     `Event = { float_t offset (time until it happens, relative to the state's time); key event (end of a running ground
     durative action, or the next timed initial literal) }`. Over-all conditions are not stored: they follow from the
     running actions. Stored like numeric values: whole vectors in a hash set (offsets shift together on each advance).
   - **Monitor** (`Monitor<Kind>`): bit words over (constraint instance, subformula), the alternating-automaton states of
     each ground instance of a trajectory constraint or trajectory preference; all-0 = the initial automaton state.
     Stored like derived atoms (bitset trees).
   - Not in the state: absolute time (node data, next to g: it is the makespan, a cost), preference penalties (in g),
     the environment (transitions), goal preferences (evaluated from the end state's facts).
5. **Action: one type per Kind, carrying everything, optional parts behind keys.** `data<Action<Kind>>` =
   `{ conditions (precondition, or at-start), effects (the only or first outcome, inline), outcome_part,
   temporal_part, preference_part }`; each part a key to a record, the shared empty record where unused.
   - `outcome_part`: the other outcomes and the probabilities of all outcomes (`oneof`/probabilistic; loki's factored
     normal form nests them: keep it factored and enumerate lazily).
   - `temporal_part`: duration (expression evaluated with the binding at start), over-all conditions, at-end conditions
     and at-end outcomes. loki's `Action` and `DurativeAction` both translate into this one record.
   - `preference_part`: precondition preferences (name, condition).
   Deterministic classical actions are unchanged apart from three keys; code that reads only conditions and effects
   (grounding, match trees, applicability, most heuristics) is unaffected. Do NOT template actions on the environment.
6. **Constraints and preferences are tracked natively, never compiled into actions.**
   - Hard trajectory constraints: the monitor steps per transition; a rejecting instance prunes the successor; the goal
     test also requires every instance accepting.
   - Goal preferences: penalty from the end state's facts (no component). Precondition preferences: penalty per
     application into g (linear metric; a nonlinear metric would need counters in numeric values: reject for now with
     an error). Trajectory preferences: monitor; penalty into g when the instance's automaton rejects.
   - Heuristics: hard constraints add the monitor's distance to acceptance (infinity if unreachable → prune); preferences
     add the penalties that can no longer be avoided (pending ones count 0: admissible).
7. **Ground stays fully ground, Lifted fully lifted.**
   - Ground: constraint/preference instances grounded at task construction by the same grounder as actions, with
     reachability pruning (unreachable `sometime` atoms → constant penalty or unsolvable; `always` over static atoms
     decided at grounding); dense instance indices; per ground action a precomputed table of the instances it advances.
     Agenda events are dense ground durative-action indices.
   - Lifted: instances interned on the fly when an applied action's effect first changes one (unify effect atoms with
     the constraints' atom patterns); a never-interned instance reads as its initial automaton state (as an unreached
     atom reads false); pending counts = total instances (type sizes or a Datalog count) minus moved ones; nested
     quantifiers inside a formula are evaluated as queries at trigger time. Agenda events refer to interned ground
     durative actions.
8. **Interfaces per model are views, not types of the record.** `StateView<Kind>` as today; `TemporalStateView<Kind>`
   (+ `time()`, `events()`, `running()`) and `MonitoredStateView<Kind>` (+ instance states, `accepting()`), with named
   concepts (`TemporalStateViewConcept`, ...). Only code that needs an extended view is instantiated for it.

## Steps (each: build, ctest, wheel, pytest, peachycheck, brokkr rules; then stop for review)

1. **Environment tags and the task variant.** Tags + concept in tyr's common declarations; `PlanningTask<Kind, E>`;
   `make_task` returning the variant from loki requirements, rejecting mixed `oneof`+`probabilistic`. Existing code uses
   `DeterministicTag` (all current tests stay green). Python: three task classes, `make_task` returns the right one.
2. **Action parts.** `outcome_part`, `temporal_part`, `preference_part` records (data, builder, view, canonicalization,
   formatter, bindings per brokkr's schema rules S1–S5); translation from loki (`src/formalism/planning/loki_to_tyr.*`)
   fills them; `Action` and `DurativeAction` both map to `Action<Kind>`. Ground and lifted grounding carry the parts.
3. **Successor generator per environment.** `Successor<Kind, E>`, `SuccessorGenerator<Kind, E>` over the shared core
   (`include/tyr/planning/successor_generator*.hpp`, `action_executor.hpp`, `applicability*`); outcome enumeration
   (lazy for factored outcomes); construction-time check that a Deterministic task has single-outcome actions (it is
   guaranteed by `make_task`; assert). Searches: keep GBFS/A* for Deterministic; add an AND-OR search space and a
   strong-cyclic policy search for Nondeterministic and an MDP search (LAO* or LRTDP) for Probabilistic. Relaxed
   programs: each outcome's effects as rules (all-outcomes determinization).
4. **Agenda and durative actions.** `Agenda<Kind>` record and storage pool; `agenda_storage` in `data<State<Kind>>`,
   state builder, view, repository (`PartStores`/`PartKeys` gain one entry, see `state_repository.hpp`); start snaps
   through the existing match tree on `conditions`; end events on the agenda; advance-time transitions; over-all
   checks; timed initial literals via `til_progress`; time and makespan in the search node. `TemporalStateView`.
   Temporal relaxed planning heuristic (durations as costs in the Datalog solver) after the successors work.
5. **Monitor, constraints and preferences.** Alternating-automaton construction from loki's constraint and trajectory
   formulas; `Monitor<Kind>` record and pool; `monitor_storage`; Ground: instance grounding with reachability and
   per-action trigger tables; Lifted: on-the-fly interning via effect/pattern unification; pruning, goal test,
   penalties into g; heuristic terms (distance to acceptance, unavoidable penalties). `MonitoredStateView`.

## Tests (tests follow code; hand-written tasks where no benchmark exists)

- Benchmarks (pypddl-datasets ≥ 0.0.22): `temporal/`, `probabilistic/`, `nondeterministic/` families, Ground and Lifted.
- Small hand-written tasks for every cross-combination the data lacks, Ground and Lifted each: `oneof` with durative
  actions; probabilistic with trajectory constraints; preferences (goal, precondition, trajectory) in each environment;
  numeric effects inside outcomes; mixed `:action`/`:durative-action`; `make_task` rejecting `oneof`+`probabilistic`.
- Classical behaviour unchanged: identical expansions and plan costs on the existing benchmark runs
  (`benchmarks/tools/runner.py`, `compare.py`; ≤10% time difference is noise).

## Rules to follow (from the user and brokkr's docs)

- brokkr rules (https://planning-and-learning.github.io/brokkr/): schema rules S1–S5 for every new record; template rule
  T20 (name any type list by a class of its own; `python -m pybrokkr.rules T20`), T17; determinism D1–D3.
- No special cases: extend the data/builder/view pattern and urd/yggdrasil generically; explicit C++ style (named
  concepts, written-out return types, if/else-if/else); no silent fallbacks (reject unsupported input with an error).
- Builds: at most `-j24`, never overlapping builds, every run under `systemd-run --user --scope -p MemoryMax=...`;
  build, ctest, wheel and pytest as separate checked steps; never precompiled headers.
- Do not commit or push until the user has reviewed each step.

## Not now (add when tasks or a use appear)

- `ImpreciseTag` (probabilities over `oneof` sets, MDPSTs).
- Uncertain durations (STNU-style: interval end events in the agenda).
- QNP's termination check (a solver over `NondeterministicTag` tasks; relevant to generalized planning in runir).
- Belief states (conformant / partially observable): would need a set-of-states type, not a component.
- Nonlinear preference metrics (counters in numeric values).
