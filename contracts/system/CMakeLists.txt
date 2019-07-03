add_contract(system system ${CMAKE_CURRENT_SOURCE_DIR}/system.cpp)

target_include_directories(system
   PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
set_target_properties(system PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
