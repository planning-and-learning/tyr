# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""PEP 517 backend for the pytyr wheel; machinery lives in pyyggdrasil.build_support."""

from pyyggdrasil.build_support import ProviderBackend

ProviderBackend(
    package="pytyr",
    providers=("pypddl", "pyyggdrasil"),
    jobs_env="TYR_JOBS",
    strip_env="TYR_STRIP_WHEEL",
).install_hooks(globals())
