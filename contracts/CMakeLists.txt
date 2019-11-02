cmake_minimum_required( VERSION 3.5 )

project(contracts)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eoscc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D${TARGET_NETWORK}")

add_subdirectory(eostd)
link_libraries(eostd)

add_subdirectory(account)
add_subdirectory(bancor)
add_subdirectory(htlc)
add_subdirectory(reserve)
add_subdirectory(system)
add_subdirectory(token)
add_subdirectory(gacha)
