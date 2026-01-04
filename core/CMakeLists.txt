add_library(core SHARED
    shared.cpp
    logger.cpp
    timeutil.cpp
    master.cpp
    children.cpp
    cli.cpp
)

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Экспорт символов из DLL на Windows
if (WIN32)
    target_compile_definitions(core PRIVATE CORE_EXPORTS=1)
endif()

if (UNIX AND NOT APPLE)
    target_link_libraries(core PUBLIC pthread)
endif()