uv pip install     --no-cache     --no-build-isolation     --reinstall-package pyyggdrasil     ../yggdrasil
uv pip install     --no-cache     --no-build-isolation     --reinstall-package pypddl     ../loki
cmake -S . -B build     -DCMAKE_BUILD_TYPE=Release     -DPython_EXECUTABLE="$PWD/.venv/bin/python"     -DCMAKE_PREFIX_PATH="$("$PWD/.venv/bin/python" -c 'import pypddl, pyyggdrasil; print(f"{pypddl.native_prefix()};{pyyggdrasil.native_prefix()}")')"     -DTYR_BUILD_EXECUTABLES=ON     -DTYR_ENABLE_SEMI_NAIVE=ON     -DTYR_ENABLE_INNER_PARALLELISM=OFF -DTYR_HEADER_INSTANTIATION=ON

cmake --build build --target gbfs_lazy -j32

