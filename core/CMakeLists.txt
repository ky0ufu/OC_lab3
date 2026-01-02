add_library(core SHARED
    shared.cpp
    logger.cpp
    timeutil.cpp
    master.cpp
    children.cpp
    cli.cpp
)

target_include_directories(core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

if (WIN32)
    target_compile_definitions(core PRIVATE CORE_EXPORTS)
endif()

if (UNIX)
    target_link_libraries(core pthread)
endif()