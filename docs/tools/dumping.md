# Dumping

```python
dumped = dump_result(
    task, result, output_dir,
    formats=(DumpFormat.JSON, DumpFormat.MD), include_plan_text=True,
)
```

Pass the native task used for search and its native `SearchResult`. `output_dir`
is a string or `Path`; `formats` defaults to `(DumpFormat.JSON,)` and
`include_plan_text` defaults to `False`.

| Output | When written |
|---|---|
| `result.json`, `task.json` | `DumpFormat.JSON` is requested. |
| `summary.md` | `DumpFormat.MD` is requested. |
| `plan.txt` | `include_plan_text=True` and a plan exists. |

`DumpResult.output_dir` is the absolute directory used; `DumpResult.files` is a
tuple of the paths written. Reusing an output directory allocates `run-002`,
`run-003`, and so on, preserving earlier artifacts. A reservation marker or
existing output files reserve a directory.

## JSON schema

Schema version `2` contains `schema_version`, `tool`, `status`, `context`, and
`task`. `tool` is `tyr.tools.find_satisficing_plan`; the top-level `status` is
`success` when native search reports `SOLVED`, otherwise `failure`.
`context.backend` is `ground` or `lifted`, and `context.index` comes from
`int(task.get_task().get_index())`.

`task.json` contains the same metadata as the nested `task` object:

| Field | Meaning |
|---|---|
| `schema_version` | `2`. |
| `name` | Native task name. |
| `domain_path`, `task_path` | Native source paths as supplied, or `null` when absent. |
| `status` | Native search status name, such as `SOLVED` or `TIMEOUT`. |
| `solved` | Whether search found a plan. |
| `plan_length`, `plan_cost` | Native plan length/cost, or `null` without a plan. |
| `plan_path` | Absolute path to the written `plan.txt`, or `null`. |

`plan.txt` contains a metadata header and one block per step, with `[facts]`
sections for the initial and successor states. `summary.md` uses `tabulate` to
show task name, search status, solved flag, plan length, cost, and plan filename.
