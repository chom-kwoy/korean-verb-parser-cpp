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

## nlohmann json ##
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(json)
set_property(TARGET nlohmann_json PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(parser_lib PUBLIC nlohmann_json::nlohmann_json)

# ---- Declare executable ----

add_executable(parser_exe source/main.cpp)
add_executable(parser::exe ALIAS parser_exe)

set_property(TARGET parser_exe PROPERTY OUTPUT_NAME parser)

target_compile_features(parser_exe PRIVATE cxx_std_17)

target_link_libraries(parser_exe PRIVATE parser_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
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
