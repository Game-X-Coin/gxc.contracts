add_contract(token token ${CMAKE_CURRENT_SOURCE_DIR}/token.cpp)

target_include_directories(token
   PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
set_target_properties(token PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
