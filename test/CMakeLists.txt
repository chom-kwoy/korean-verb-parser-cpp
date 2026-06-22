# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(parserTests LANGUAGES CXX)

# ---- Dependencies ----

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.0   # bump to whatever release you want
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(Catch2)
include(Catch)

# ---- Tests ----

add_executable(parser_test source/parser_test.cpp)
target_link_libraries(
    parser_test PRIVATE
    parser_lib
    Catch2::Catch2WithMain
)
target_compile_features(parser_test PRIVATE cxx_std_17)

# Run tests from the repository root so fixtures referenced by relative path
# (e.g. examples/prods.json) are found regardless of where ctest is invoked.
catch_discover_tests(parser_test WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# ---- Benchmark ----

# Not registered with catch_discover_tests on purpose: it is opt-in and slow, so
# it should not run as part of the normal `ctest` suite. Run it manually from the
# project root (the grammar fixture is loaded via a relative path):
#   ./build/dev/test/parser_benchmark
add_executable(parser_benchmark source/parser_benchmark.cpp)
target_link_libraries(
    parser_benchmark PRIVATE
    parser_lib
    Catch2::Catch2WithMain
)
target_compile_features(parser_benchmark PRIVATE cxx_std_17)

# ---- End-of-file commands ----

add_folders(Test)
