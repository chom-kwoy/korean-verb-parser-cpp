install(
    TARGETS parser_exe
    RUNTIME COMPONENT parser_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
