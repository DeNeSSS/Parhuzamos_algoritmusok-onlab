cmake_minimum_required(VERSION 3.10)

project(ParallelAlgorithmsLab CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Modern C++17 és Clang kényszerítése
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_COMPILER "clang++")

# Megkeressük az OpenMP-t
find_package(OpenMP REQUIRED)

# --- BUILD TÍPUSOK KONFIGURÁCIÓJA ---

# Ha nincs megadva build típus, legyen alapból Debug
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# 1. DEBUG: Nincs optimalizálás, sok infó a hibakereséshez
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -Wall -Wextra")

# 2. FAST_TEST (Gyors tesztelés): Közepes optimalizálás, gyors fordítás
# Itt a RelWithDebInfo-t használjuk alapnak
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

# 3. MEASURE (A méréshez): Maximális sebesség, hardver-specifikus optimalizálás
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

# Clang-tidy: Csak Debug és FastTest módban futtatjuk, hogy a mérést ne zavarja
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(CMAKE_CXX_CLANG_TIDY "clang-tidy")
else()
    unset(CMAKE_CXX_CLANG_TIDY CACHE)
endif()

# --- FORRÁSFÁJLOK ÉS CÉLOK ---

file(GLOB SOURCES "src/*.cpp")
add_executable(parallel_lab ${SOURCES})

target_include_directories(parallel_lab PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(OpenMP_CXX_FOUND)
    target_link_libraries(parallel_lab PUBLIC OpenMP::OpenMP_CXX)
endif()

# Kiírjuk, épp melyik módban vagyunk, hogy ne legyen kavarodás
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")