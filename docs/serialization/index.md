# Serialized representations

Serialization produces JSON values for selected native entities and their dependencies. Registered entity types become compact references; other entities remain nested objects. With no registered tables, the complete representation is inline.

A reference combines its table prefix and zero-based row position. For example, `a0` denotes the first row of the table whose prefix is `a`:

```json
{
  "atoms": {
    "prefix": "a",
    "rows": [{"binding": "b0"}]
  },
  "bindings": {
    "prefix": "b",
    "rows": [{"relation": "p0", "objects": ["o0"]}]
  },
  "predicates": {
    "prefix": "p",
    "rows": [{"name": "at", "arity": 1}]
  },
  "objects": {
    "prefix": "o",
    "rows": [{"name": "truck"}]
  }
}
```

Table names and prefixes are chosen by the caller. Both must be nonempty and unique; a prefix cannot end in a digit. Registrations are fixed when serialization begins. Rows follow first encounter order, including dependencies encountered inside other rows. Values that compare equal under their native equality share a row.

The result, table snapshot, and enum snapshot are separate values. Registered tables remain present even when empty. Snapshots contain the rows collected so far; later serialization does not change an earlier snapshot. The library does not choose a report envelope, render tables, or write files.

Sequences are JSON arrays, pairs are two-element arrays, absent optional values are `null`, and paths are strings with `/` separators. Ordinary fields retain their native names. Variant wrappers contain `kind` and `value`; the numeric kind identifies the native alternative, and `value` is its inline representation or dictionary reference. Numeric constants remain numbers. Enum and variant legends contain only encountered alternatives, sorted by native numeric ID:

```json
{
  "FunctionExpression": [
    {"id": 0, "name": "constant"}
  ]
}
```

Planning states contain only `fluent_facts`, `derived_atoms`, and `fluent_fterm_values`. Static facts belong to the task representation. Function-term values are pairs of a term representation or reference and its numeric value. An FDR fact preserves its `variable` and numeric `value`; zero represents the native none value.

Nodes contain `state` and `metric`. Labeled nodes contain `label` and `node`. Plans contain `start_node`, `labeled_succ_nodes`, `length`, and `cost`; repeated states and nodes reuse their registered references. Ground and lifted task wrappers contain `formalism_task`, which preserves the underlying task and domain information without exposing runtime caches.
