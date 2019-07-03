add_contract(htlc htlc ${CMAKE_CURRENT_SOURCE_DIR}/htlc.cpp)

target_include_directories(htlc
   PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
set_target_properties(htlc PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
