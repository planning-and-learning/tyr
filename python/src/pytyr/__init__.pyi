from pathlib import Path

# Load public native dependency packages before this package loads native extensions.
import pypddl as pypddl
import pyyggdrasil as pyyggdrasil

from . import (
    datalog as datalog,
    formalism as formalism,
    planning as planning,
)

__version__: str

def native_prefix() -> Path: ...
def native_include_dir() -> Path: ...
def native_lib_dir() -> Path: ...
def cmake_prefix() -> Path: ...
def cmake_dir() -> Path: ...
