cmake_minimum_required(VERSION 3.28)

include(FetchContent)

include(cmake/prelude.cmake)

project(
    parser
    VERSION 0.1.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    parser_lib OBJECT
    source/production.hpp source/production.cpp
    source/nonterminal.hpp source/nonterminal.cpp
    source/pcfg.hpp source/pcfg.cpp
    source/viterbiparser.h source/viterbiparser.cpp
    source/tree.h source/tree.cpp
)

target_include_directories(
    parser_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(parser_lib PUBLIC cxx_std_20)

## fmt ##
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG        e69e5f977d458f2650bb346dadf2ad30c5320281  # 10.2.1
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(fmt)
set_property(TARGET fmt PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(parser_lib PRIVATE fmt::fmt)

## nlohmann json ## (used for output serialization)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(json)
set_property(TARGET nlohmann_json PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(parser_lib PUBLIC nlohmann_json::nlohmann_json)

## simdjson ## (fast input parsing; nlohmann is convenient but ~5x slower to parse)
set(SIMDJSON_DEVELOPER_MODE OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    simdjson
    GIT_REPOSITORY https://github.com/simdjson/simdjson
    GIT_TAG        v3.10.1
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL
    SYSTEM  # treat simdjson headers as system so its -Wundef etc. don't trip -Werror
)
FetchContent_MakeAvailable(simdjson)
set_property(TARGET simdjson PROPERTY POSITION_INDEPENDENT_CODE ON)
# simdjson's own sources aren't clean under this project's strict warning set
# (notably -Werror=float-equal in to_chars.cpp). It's a vendored dependency, so
# disable warnings for its own translation units; SYSTEM (above) keeps its
# headers from warning in our translation units.
if(MSVC)
  target_compile_options(simdjson PRIVATE /w)
else()
  target_compile_options(simdjson PRIVATE -w)
endif()
target_link_libraries(parser_lib PRIVATE simdjson::simdjson)

# ---- Declare executable ----

add_executable(parser_exe source/main.cpp)
add_executable(parser::exe ALIAS parser_exe)

set_property(TARGET parser_exe PROPERTY OUTPUT_NAME parser)

target_compile_features(parser_exe PRIVATE cxx_std_17)

target_link_libraries(parser_exe PRIVATE parser_lib simdjson::simdjson)

# ---- Python bindings (nanobind) ----

option(parser_BUILD_PYTHON "Build the nanobind Python bindings" OFF)

if(parser_BUILD_PYTHON)
  # The bindings build a shared Python module, so the object library linked into
  # it must be position-independent.
  set_property(TARGET parser_lib PROPERTY POSITION_INDEPENDENT_CODE ON)

  find_package(Python 3.8 REQUIRED COMPONENTS Interpreter Development.Module)

  # Prefer a pip-installed nanobind (this is how it is provided when building a
  # wheel via scikit-build-core); fall back to fetching it from source for a
  # plain standalone CMake build.
  execute_process(
      COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
      OUTPUT_STRIP_TRAILING_WHITESPACE
      OUTPUT_VARIABLE nanobind_cmake_dir
      RESULT_VARIABLE nanobind_query_result)
  if(nanobind_query_result EQUAL 0)
    list(APPEND CMAKE_PREFIX_PATH "${nanobind_cmake_dir}")
    find_package(nanobind CONFIG REQUIRED)
  else()
    FetchContent_Declare(
        nanobind
        GIT_REPOSITORY https://github.com/wjakob/nanobind
        GIT_TAG        v2.13.0  # bump as desired
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(nanobind)
  endif()

  # Builds the compiled extension korean_verb_parser._core.
  nanobind_add_module(parser_python NB_STATIC bindings/bindings.cpp)
  set_target_properties(parser_python PROPERTIES OUTPUT_NAME _core)
  target_link_libraries(parser_python PRIVATE parser_lib)
  target_compile_features(parser_python PRIVATE cxx_std_20)

  # When building a wheel, install the extension into the package directory.
  if(DEFINED SKBUILD)
    install(TARGETS parser_python LIBRARY DESTINATION korean_verb_parser)
  endif()
endif()

# ---- Install rules ----

# Skip the C++ executable's install rules when building a Python wheel, where
# only the extension module should be installed.
if(NOT CMAKE_SKIP_INSTALL_RULES AND NOT DEFINED SKBUILD)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT parser_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of parser"
  )
endif()

include(cmake/dev-mode.cmake)
