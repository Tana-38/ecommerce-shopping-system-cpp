cmake_minimum_required(VERSION 3.10)
project(ShopSphere LANGUAGES CXX)

# C++14 by default (works with MinGW GCC 6.3); configure with
# -DCMAKE_CXX_STANDARD=17 to build as C++17.
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 14)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB SHOP_LIB_SOURCES
    ${CMAKE_SOURCE_DIR}/src/util/*.cpp
    ${CMAKE_SOURCE_DIR}/src/core/*.cpp
    ${CMAKE_SOURCE_DIR}/src/cart/*.cpp
    ${CMAKE_SOURCE_DIR}/src/payment/*.cpp
    ${CMAKE_SOURCE_DIR}/src/services/*.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/*.cpp)

add_library(shopsphere_lib STATIC ${SHOP_LIB_SOURCES})
target_include_directories(shopsphere_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(shopsphere_lib PUBLIC /W4 /utf-8 /EHsc)
    target_compile_definitions(shopsphere_lib PUBLIC _CRT_SECURE_NO_WARNINGS NOMINMAX)
else()
    target_compile_options(shopsphere_lib PUBLIC -Wall -Wextra)
endif()

# Binaries go to <repo>/bin so the app finds data/ and invoices/ when run
# from the repository root.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
foreach(cfg ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${cfg} CFG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG} ${CMAKE_SOURCE_DIR}/bin)
endforeach()

add_executable(shopsphere src/main.cpp)
target_link_libraries(shopsphere PRIVATE shopsphere_lib)

enable_testing()
foreach(t test_all test_partA test_partB)
    add_executable(${t} tests/${t}.cpp)
    target_link_libraries(${t} PRIVATE shopsphere_lib)
    add_test(NAME ${t} COMMAND ${t} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endforeach()
