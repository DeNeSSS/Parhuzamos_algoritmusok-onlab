cmake_minimum_required(VERSION 3.10)

# A projekt neve és a nyelv (C++)
project(ParallelAlgorithmsLab CXX)

# Generálja le a fordítási parancsok adatbázisát (ez kell a kódelemzőknek)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Bekapcsoljuk a clang-tidy-t a build folyamatba
set(CMAKE_CXX_CLANG_TIDY "clang-tidy")

# Használjunk modern C++17 szabványt
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Kényszerítjük a rendszert, hogy a Clang-ot használja
set(CMAKE_CXX_COMPILER "clang++")

# Megkeressük az OpenMP csomagot
find_package(OpenMP REQUIRED)

# 1. ÚJÍTÁS: Összegyűjtjük az összes .cpp fájlt az src mappából egy SOURCES nevű változóba
file(GLOB SOURCES "src/*.cpp")

# 2. ÚJÍTÁS: A futtatható fájlt most már az src mappából kigyűjtött fájlokból rakjuk össze
add_executable(parallel_lab ${SOURCES})

# Megadjuk, hogy az "include" mappában keresse a fejléceket
target_include_directories(parallel_lab PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Ha megtalálta az OpenMP-t, hozzákapcsoljuk a futtatható fájlunkhoz
if(OpenMP_CXX_FOUND)
    target_link_libraries(parallel_lab PUBLIC OpenMP::OpenMP_CXX)
endif()