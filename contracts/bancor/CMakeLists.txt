add_contract(bancor bancor ${CMAKE_CURRENT_SOURCE_DIR}/bancor.cpp)

target_include_directories(bancor
   PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
set_target_properties(bancor PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
