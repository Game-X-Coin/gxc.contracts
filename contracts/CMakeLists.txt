cmake_minimum_required( VERSION 3.5 )

project(contracts)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D${TARGET_NETWORK}")

add_subdirectory(libsio4)
link_libraries(sio4)

add_subdirectory(account)
add_subdirectory(bancor)
add_subdirectory(htlc)
#add_subdirectory(random)
add_subdirectory(reserve)
add_subdirectory(system)
add_subdirectory(token)
