import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

import pytyr
import pypddl
import pyyggdrasil
import pytest


ROOT_DIR = Path(__file__).resolve().parents[3]
DOWNSTREAM_PACKAGE_DIR = Path(__file__).resolve().parent / "minimal_downstream_package"


def test_downstream_python_binding_imports_public_pytyr_api_and_links_tyr_core(tmp_path: Path) -> None:
    cmake = shutil.which("cmake")
    if cmake is None:
        pytest.skip("cmake is required for the downstream binding smoke test")

    pytyr_prefix = Path(pytyr.native_prefix())
    pypddl_prefix = Path(pypddl.native_prefix())
    pyyggdrasil_prefix = Path(pyyggdrasil.native_prefix())
    tyr_library_dir = pytyr_prefix / "lib"
    dependency_library_dirs = [
        tyr_library_dir,
        pypddl_prefix / "lib",
        pyyggdrasil_prefix / "lib",
    ]
    tyr_core_libraries = list(tyr_library_dir.glob("libtyr_core.*"))
    if not tyr_core_libraries:
        pytest.skip("pytyr was not installed with a shared tyr::core library")

    tyr_include_dir = pytyr_prefix / "include"
    if not (tyr_include_dir / "tyr" / "formalism" / "planning" / "repository.hpp").exists():
        pytest.skip("pytyr was not installed with Tyr C++ public headers")

    tyr_cmake_dir = pytyr_prefix / "lib" / "cmake" / "tyr"
    if not (tyr_cmake_dir / "tyrConfig.cmake").exists():
        pytest.skip("pytyr was not installed with Tyr CMake package files")

    nanobind_cmake_dir = pyyggdrasil_prefix / "lib" / "cmake" / "nanobind"
    if not nanobind_cmake_dir.exists():
        pytest.skip("nanobind CMake package was not found in the pyyggdrasil installation prefix")

    project_dir = tmp_path / "minimal_downstream_package"
    shutil.copytree(DOWNSTREAM_PACKAGE_DIR, project_dir)

    env = os.environ.copy()
    env["CMAKE_ARGS"] = " ".join(
        [
            f"-DCMAKE_PREFIX_PATH={pytyr_prefix};{pypddl_prefix};{pyyggdrasil_prefix}",
            f"-Dtyr_DIR={tyr_cmake_dir}",
            f"-DPython_EXECUTABLE={sys.executable}",
            f"-DDOWNSTREAM_RUNTIME_LIBRARY_DIRS={';'.join(str(path) for path in dependency_library_dirs)}",
            env.get("CMAKE_ARGS", ""),
        ]
    ).strip()

    build_dir = tmp_path / "build"
    subprocess.run(
        [
            cmake,
            "-S",
            str(project_dir),
            "-B",
            str(build_dir),
            f"-DCMAKE_PREFIX_PATH={pytyr_prefix};{pypddl_prefix};{pyyggdrasil_prefix}",
            f"-Dtyr_DIR={tyr_cmake_dir}",
            f"-DPython_EXECUTABLE={sys.executable}",
            f"-DDOWNSTREAM_RUNTIME_LIBRARY_DIRS={';'.join(str(path) for path in dependency_library_dirs)}",
        ],
        check=True,
        env=env,
    )
    subprocess.run([cmake, "--build", str(build_dir), "-j3"], check=True)

    env["PYTHONPATH"] = str(project_dir / "src") + os.pathsep + env.get("PYTHONPATH", "")
    runtime_library_path = os.pathsep.join(str(path) for path in dependency_library_dirs)
    env["LD_LIBRARY_PATH"] = runtime_library_path + os.pathsep + env.get("LD_LIBRARY_PATH", "")
    env["DYLD_LIBRARY_PATH"] = runtime_library_path + os.pathsep + env.get("DYLD_LIBRARY_PATH", "")

    result = subprocess.run(
        [
            sys.executable,
            "-c",
            (
                "import json, downstream_tyr_user; "
                "print(json.dumps({"
                "'imports': downstream_tyr_user.describe_pytyr_imports(), "
                "'product': downstream_tyr_user.multiply(6, 7)"
                "}))"
            ),
        ],
        check=True,
        capture_output=True,
        text=True,
        env=env,
    )

    payload = json.loads(result.stdout)
    assert payload == {
        "imports": {
            "execution_context": "ExecutionContext",
            "float_t_size": 8,
        },
        "product": 42,
    }
