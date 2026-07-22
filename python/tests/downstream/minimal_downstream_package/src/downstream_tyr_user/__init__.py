import importlib
from typing import Any

from pyyggdrasil.execution import ExecutionContext

# _downstream_tyr is the compiled extension built during the downstream smoke test;
# it is absent at type-check time, so import it dynamically and re-export its symbols
# (multiply is consumed by the test's subprocess as downstream_tyr_user.multiply).
_downstream_tyr: Any = importlib.import_module("._downstream_tyr", __name__)
float_t_size = _downstream_tyr.float_t_size
multiply = _downstream_tyr.multiply


def describe_pytyr_imports() -> dict[str, object]:
    execution_context = ExecutionContext(1)
    return {
        "execution_context": type(execution_context).__name__,
        "float_t_size": float_t_size(),
    }
