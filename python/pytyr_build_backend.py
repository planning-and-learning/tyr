# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""PEP 517 backend for the pytyr wheel; machinery lives in pybrokkr.build_backend."""

from pybrokkr.build_backend import ProviderBackend

ProviderBackend(
    package="pytyr",
    providers=("pypddl", "pyyggdrasil", "pyurd"),
    prefix_defines={"pyyggdrasil": "YGGDRASIL_NATIVE_PREFIX", "pyurd": "URD_NATIVE_PREFIX"},
    jobs_env="TYR_JOBS",
    strip_env="TYR_STRIP_WHEEL",
).install_hooks(globals())
