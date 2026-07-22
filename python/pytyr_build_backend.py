"""PEP 517 backend for the pytyr wheel; machinery lives in pyyggdrasil.build_support."""

from pyyggdrasil.build_support import ProviderBackend

ProviderBackend(
    package="pytyr",
    providers=("pypddl", "pyyggdrasil"),
    jobs_env="TYR_JOBS",
).install_hooks(globals())
