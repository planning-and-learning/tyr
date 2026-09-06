# Serialized representations

Serialization produces JSON values for selected native entities. Registered entity types become compact references with their declared fields stored in table rows. Unregistered native entities use their existing text formatter, equivalent to Python `str(value)`. With no registered tables, serializing an entity returns its native text.

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

Pass `fields` when registering a table to select its immediate fields:

```python
dictionaries.register_table(fp.GroundTask, "tasks", "t", fields=["name", "static_ground_atoms"])
```

`fields=None` keeps all declared fields; `fields=[]` creates empty rows. Selection happens before recursive serialization, so omitted fields do not collect descendants. Field names are immediate schema keys, and retained columns follow schema order rather than selection order.

Pass `project` to define a complete row with your own column names and values:

```python
dictionaries.register_table(
    fp.ActionBinding,
    "actions",
    "a",
    project=lambda binding: {
        "name": binding.get_relation().get_name(),
        "objects": binding.get_objects(),
    },
)
```

The callable receives the native entity and returns a dictionary. Its values are recursively serialized using the same registry, so registered objects become references. The projected row replaces the declared fields. If `fields` is also supplied, it selects the projected column names before recursive conversion, retaining the order returned by the callable. Without `project`, the native schema is unchanged.

The result and table snapshot are separate values. Registered tables remain present even when empty. Snapshots contain the rows collected so far; later serialization does not change an earlier snapshot. Callers select the values to serialize and compose their output.

Registration controls where structural traversal continues. Native text formatting stops collection of descendants, even when their types are registered. For example, register an action to inspect its fields; leave its condition unregistered to receive native condition text in that cell. Register the condition too to obtain a reference and its own row. An unregistered task returns native task text; serialize its native entities directly to populate their tables.

Sequences are JSON arrays, pairs are two-element arrays, absent optional values are `null`, and paths are strings with `/` separators. Default registered rows retain their native field names. Registered variant wrappers contain `kind` and `value`, for example `{"kind":"constant","value":3}`. The kind is the native alternative's type name; `value` is its native text, primitive value, or dictionary reference. Enum fields use their native text, such as `+`, `>=`, or `increase`. Numeric data such as constants, costs, indices, and FDR values remain numbers.

Python callers can render snapshots with Yggdrasil's generic `tabulate` adapter:

```python
from pyyggdrasil.serialization import render_table

for name, table in dictionaries.tables().items():
    print(name)
    print(render_table(table["rows"], prefix=table["prefix"]))
```

The optional prefix adds entity references in an index column. Nested dictionaries expand into columns with grouped headers. Lists become compact JSON cells; other leaves use scalar formatting. Scalar formatting and layouts come from `tabulate`. Pass `tablefmt="github"` for Markdown. The renderer returns text and leaves file handling and report layout to the caller.

Snapshots are ordinary Python dictionaries and lists. Add application columns to snapshot rows before rendering, matching annotations by entity reference rather than evidence-list position. Preserve row order when using `prefix`; if sorting or filtering, put the original references into explicit cells first and omit `prefix`. These edits do not affect the registry or later snapshots. Shared `JSONValue`, `Row`, and `Table` types live in `pyyggdrasil.serialization`. Both `table()` and `tables()` expose generic rows because selection and projection can change their columns.

Registered planning states contain `fluent_ground_atoms`, `derived_ground_atoms`, and `fluent_ground_function_term_values`. Static facts belong to the task representation. Function-term values are pairs of a term representation or reference and its numeric value. A registered FDR fact preserves its `fdr_variable` and numeric `value`; zero represents the native none value.

Registered nodes contain `state` and `metric`. Plans, labeled nodes, and task owners are not registerable and return their native text. Serialize their states, nodes, actions, or underlying formalism views directly when those tables are needed. Every serialized native type must provide a text formatter; a missing formatter intentionally causes a compile-time error.
