add_executable(program main.cpp)
target_link_libraries(program PRIVATE core)
target_include_directories(program PRIVATE ${CMAKE_SOURCE_DIR}/core)


if (WIN32)
  add_custom_command(TARGET program POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:core>
      $<TARGET_FILE_DIR:program>
  )
endif()