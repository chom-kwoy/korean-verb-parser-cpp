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

catch_discover_tests(parser_test)

# ---- End-of-file commands ----

add_folders(Test)
