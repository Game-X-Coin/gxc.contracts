add_contract(gacha gacha ${CMAKE_CURRENT_SOURCE_DIR}/gacha.cpp)

target_include_directories(gacha
   PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
set_target_properties(gacha PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
