add_contract(account account ${CMAKE_CURRENT_SOURCE_DIR}/account.cpp)

target_include_directories(account
   PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
set_target_properties(account PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
