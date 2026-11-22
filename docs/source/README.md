The repo root of tarn: one CMake project that builds `tarn-check`, the C11 program that checks a docs tree against the definitions, and its tests. The build sets warnings as errors on every compiler (`/W4 /WX` on MSVC, `-Wall -Wextra -Wpedantic -Werror` elsewhere), compiles the checker's modules once as an object library, and links them into both the program and the test runner. Build and test with `cmake -S . -B build`, `cmake --build build --config Debug` and `ctest --test-dir build --output-on-failure -C Debug`.

<!-- generated: source-children -->
- [CMakeLists.txt](../../CMakeLists.txt) - the project: the C11 settings, the strict warnings, the checker's modules and the program
- [src/](src/) - the checker: the walk of a docs tree and the source tree, and every rule it refuses or warns by
- [tests/](tests/) - the fixture trees, one mini repo per case, and the runner that asserts each one's exact report
<!-- end generated -->
