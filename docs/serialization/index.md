# Serialized representations

Serialization produces JSON values for selected native entities. Registered entity types become compact references with their declared fields stored in table rows. Every native entity reached during serialization must be registered. A missing registration raises `ValueError` in Python (`std::invalid_argument` in C++), naming the missing type. After a serialization failure, create a new registry.

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

Import `Dictionaries` from `pyyggdrasil.serialization`, and `register_table`, `serialize`, and `table` from `pytyr.serialization`. These free functions accept the shared registry as their first argument. Its `tables()` method returns all snapshots.

Use a native type's `Fields` enum to inspect the ordered default column names without creating an entity or registry:

```python
from pytyr.formalism import planning as fp

list(fp.ActionBinding.Fields.__members__)  # ["relation", "objects"]
list(fp.FunctionExpression.Fields.__members__)  # ["kind", "value"]
```

This describes the native declaration; registration's `fields` selection and `project` callback do not change it.

Each native class exposes a nested `Fields` enum generated from the same C++ declaration. Its members support IDE completion and selections checked against the registered native type:

```python
register_table(
    dictionaries, fp.ActionBinding, "actions", "a",
    fields=[fp.ActionBinding.Fields.objects],
)
```

`fields=None` keeps all declared fields; `fields=[]` creates empty rows. Selection happens before recursive serialization, so omitted fields do not collect descendants. Field names are immediate schema keys, and retained columns follow schema order rather than selection order. Members from another type's `Fields` enum are rejected. String selections remain supported, including custom column names returned by `project`.

Pass `project` to define a complete row with your own column names and values:

```python
register_table(
    dictionaries,
    fp.ActionBinding,
    "actions",
    "a",
    project=lambda binding: {
        "name": binding.get_relation().get_name(),
        "objects": binding.get_objects(),
    },
)
```

The callable receives the native entity and returns a dictionary. Its values are recursively serialized using the same registry, so the example also requires an `fp.Object` table before serialization. The projected row replaces the declared fields. If `fields` is also supplied, it selects the projected column names before recursive conversion, retaining the order returned by the callable. Without `project`, the native schema is unchanged.

The result and table snapshot are separate values. Registered tables remain present even when empty. Snapshots contain the rows collected so far; later serialization does not change an earlier snapshot. Callers select the values to serialize and compose their output.

Register nested types whose selected fields you want to traverse. To represent an entity as text, explicitly return `str(entity)` from a Python projection or call `ygg::to_string(entity)` in a C++ projection. For example, a projection can return `{"condition": str(action.get_condition())}` without registering the condition. Excluded fields are never evaluated or traversed.

Sequences are JSON arrays, pairs are two-element arrays, absent optional values are `null`, and paths are strings with `/` separators. Default registered rows retain their native field names. Registered variant wrappers contain `kind` and `value`, for example `{"kind":"constant","value":3}`. The kind is the native alternative's type name; `value` is its primitive value or dictionary reference. Enum fields use their native text, such as `+`, `>=`, or `increase`. Numeric data such as constants, costs, indices, and FDR values remain numbers.

Python callers can render snapshots with Yggdrasil's table renderer:

```python
from pyyggdrasil.serialization.table import render_table

for name, table in dictionaries.tables().items():
    print(name)
    print(render_table(table["rows"], prefix=table["prefix"]))
```

The optional prefix adds entity references in an index column. Nested dictionaries expand into columns with grouped headers. Simple lists use brackets and commas, such as `[a0,a1]`; nested or ambiguous lists use compact JSON. Columns use `|` separators; pass `aligned=True` to pad them. The renderer returns text and leaves file handling and report layout to the caller.

Snapshots are ordinary Python dictionaries and lists. Add application columns to snapshot rows before rendering, matching annotations by entity reference rather than evidence-list position. Preserve row order when using `prefix`; if sorting or filtering, put the original references into explicit cells first and omit `prefix`. These edits do not affect the registry or later snapshots. Shared `JSONValue`, `Row`, and `Table` types live in `pyyggdrasil.serialization.table`. Both `table()` and `tables()` expose generic rows because selection and projection can change their columns.

Registered planning states contain `fluent_ground_atoms`, `derived_ground_atoms`, and `fluent_ground_function_term_values`. Static facts belong to the task representation. Function-term values are pairs of a term representation or reference and its numeric value. A registered FDR fact preserves its `fdr_variable` and numeric `value`; zero represents the native none value.

Registered nodes contain `state` and `metric`. Plans, labeled nodes, and task owners are not registerable and cannot be serialized directly. Serialize their states, nodes, actions, or underlying formalism views instead. Native text formatting is used only when explicitly requested; a serializer does not require a text formatter.

In C++, `describe_fields` declares each name and accessor together. For example, the object declaration is:

```cpp
namespace ygg::serialization
{
template<class Archive>
void describe_fields(Archive& ar, std::type_identity<tyr::formalism::planning::ObjectView>)
{
    ar.field("name", [](const auto& value) -> decltype(auto) { return (value.get_name()); });
}
}
```

Serialization invokes the accessors; `ygg::serialization::fields<tyr::formalism::planning::ObjectView>()` returns `{"name"}` without invoking them or constructing a value. Both operations use this single declaration. Variant declarations use `ar.variant(accessor)` and expose `kind` and `value`.
