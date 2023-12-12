# CMakeList.txt : CMake project for amxmul, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.22.0)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("amxmul" LANGUAGES CXX)

#find_package(IntelDPCPP REQUIRED)


# Add source to this project's executable.
add_executable (amxmul "main.cpp" "amx.tmul.ref.ixx" "amx.tmul.skl.ixx" "amx.tmul.spr.ixx" "amx.tools.ixx" "amx.tile_config.ixx" "amx.tile.ixx" "amx.test.ixx" "amx.test_data.ixx" "amx.matrix.ixx" "amx.conversion.ixx" "amx.print.ixx")

if (MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mamx-tile -mamx-int8 -mamx-bf16")
endif()

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET amxmul PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
