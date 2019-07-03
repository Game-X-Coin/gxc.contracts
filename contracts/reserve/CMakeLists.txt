add_contract(reserve reserve ${CMAKE_CURRENT_SOURCE_DIR}/reserve.cpp)

target_include_directories(reserve
   PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
set_target_properties(reserve PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
