cmake_minimum_required(VERSION 3.23)
project(SEM C)
set(CMAKE_C_STANDARD 99)

# Find the required packages
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)

if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
endif()

# Add your executable
add_executable(SEM main.c lzw.c files.c bmp.c png.c pngs.h
        checksuma.c
        checksuma.h)

# Include directories for the target SEM
target_include_directories(SEM PRIVATE ${PNG_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})

# Link libraries for the target SEM
target_link_libraries(SEM PRIVATE ${PNG_LIBRARIES} ${ZLIB_LIBRARIES})
