project(PREFIX_SEARCH)
cmake_minimum_required(VERSION 2.8)
include(ExternalProject)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif ()

if (UNIX)
   # C++11
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

   # For hardware popcount
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2")

   # Extensive warnings
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-missing-braces")
   # Silence a warning bug in Boost
   if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedefs")
   endif ()
   # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wconversion")

   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter -Wno-unused-const-variable")

   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb") # Add debug info anyway

endif()

find_package(Boost 1.55.0 COMPONENTS iostreams unit_test_framework log program_options filesystem system thread chrono REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})
link_directories(${ZLIB_LIBRARY_DIRS})

# External dependencies

set(partitioned_elias_fano_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/external/partitioned_elias_fano)
ExternalProject_Add(partitioned_elias_fano
  GIT_REPOSITORY git@github.com:ot/partitioned_elias_fano.git
  PREFIX ${partitioned_elias_fano_PREFIX}
  PATCH_COMMAND cd succinct && git reset --hard &&
                git apply --ignore-whitespace < ${PROJECT_SOURCE_DIR}/../patches/succinct-ostream.patch &&
                git apply --ignore-whitespace < ${PROJECT_SOURCE_DIR}/../patches/succinct-constructors.patch &&
                cd .. && git reset --hard &&
                git apply --ignore-whitespace < ${PROJECT_SOURCE_DIR}/../patches/gcc-bug-rename-var-end.patch &&
                git apply --ignore-whitespace < ${PROJECT_SOURCE_DIR}/../patches/pef-multithread-optpfor-bug.patch
  UPDATE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  BUILD_IN_SOURCE 1
  LOG_DOWNLOAD ON
)

set(cpi00_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/external/cpi00)
ExternalProject_Add(cpi00
  GIT_REPOSITORY https://code.google.com/p/cpi00/
  PREFIX ${cpi00_PREFIX}
  PATCH_COMMAND git reset --hard && git apply --ignore-whitespace < ${PROJECT_SOURCE_DIR}/../patches/cpi00-private-upgrade.patch
  CONFIGURE_COMMAND ""
  UPDATE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  BUILD_IN_SOURCE 1
  LOG_DOWNLOAD ON
)

# Libraries

include_directories(${PREFIX_SEARCH_SOURCE_DIR})
include_directories(${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano)
include_directories(${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/succinct)
include_directories(${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/FastPFor/headers)
include_directories(${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/integer_encoding_library/include)
include_directories(${cpi00_PREFIX}/src/cpi00/src)

set_property(SOURCE
  ${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/block_codecs.cpp
  ${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/integer_encoding_library/src/compress/table/decUnary.cpp
  ${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/integer_encoding_library/src/compress/table/decGamma.cpp
  ${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/integer_encoding_library/src/compress/table/decDelta.cpp
  ${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/integer_encoding_library/src/io/BitsReader.cpp
  ${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/integer_encoding_library/src/io/BitsWriter.cpp
  PROPERTY GENERATED ON)

add_library(block_codecs
  ${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/block_codecs.cpp
  ${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/integer_encoding_library/src/compress/table/decUnary.cpp
  ${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/integer_encoding_library/src/compress/table/decGamma.cpp
  ${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/integer_encoding_library/src/compress/table/decDelta.cpp
  ${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/integer_encoding_library/src/io/BitsReader.cpp
  ${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/integer_encoding_library/src/io/BitsWriter.cpp
  )

add_dependencies(block_codecs partitioned_elias_fano)

set_property(SOURCE
  ${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/FastPFor/src/bitpacking.cpp
  ${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/FastPFor/src/bitpackingaligned.cpp
  ${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/FastPFor/src/bitpackingunaligned.cpp
  ${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/FastPFor/src/simdbitpacking.cpp
  ${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/succinct/rs_bit_vector.cpp
  ${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/succinct/bp_vector.cpp
  PROPERTY GENERATED ON)

add_library(FastPFor_lib STATIC
  ${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/FastPFor/src/bitpacking.cpp
  ${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/FastPFor/src/bitpackingaligned.cpp
  ${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/FastPFor/src/bitpackingunaligned.cpp
  ${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/FastPFor/src/simdbitpacking.cpp)

add_dependencies(FastPFor_lib partitioned_elias_fano)

add_library(succinct
  ${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/succinct/rs_bit_vector.cpp
  ${partitioned_elias_fano_PREFIX}/src/partitioned_elias_fano/succinct/bp_vector.cpp
  )

set_property(SOURCE
  ${cpi00_PREFIX}/src/cpi00/src/CompPermIdx.cpp
  ${cpi00_PREFIX}/src/cpi00/src/HufWatTree.cpp
  ${cpi00_PREFIX}/src/cpi00/src/BitVec.cpp
  PROPERTY GENERATED ON)
add_library(cpi00_lib STATIC
  ${cpi00_PREFIX}/src/cpi00/src/CompPermIdx.cpp
  ${cpi00_PREFIX}/src/cpi00/src/HufWatTree.cpp
  ${cpi00_PREFIX}/src/cpi00/src/BitVec.cpp)

# Executables section

add_executable(friends_at_k friends_at_k.cpp)
target_link_libraries(friends_at_k
  ${Boost_LIBRARIES}
  block_codecs
  FastPFor_lib
)

add_executable(create_index create_index.cpp)
target_link_libraries(create_index
  ${Boost_LIBRARIES}
  succinct
  block_codecs
  FastPFor_lib
)

add_executable(reassign_ids reassign_ids.cpp)
target_link_libraries(reassign_ids
  ${Boost_LIBRARIES}
  ${ZLIB_LIBRARIES}
)

add_executable(create_dictionary create_dictionary.cpp)
add_dependencies(create_dictionary cpi00_lib)
target_link_libraries(create_dictionary
  ${Boost_LIBRARIES}
  cpi00_lib
)

add_executable(simple_scheme simple_scheme.cpp)
add_dependencies(simple_scheme cpi00_lib)
target_link_libraries(simple_scheme
  ${Boost_LIBRARIES}
  ${ZLIB_LIBRARIES}
  succinct
  cpi00_lib
  block_codecs
  FastPFor_lib
)

add_executable(verify_index verify_index.cpp)
target_link_libraries(verify_index
  ${Boost_LIBRARIES}
  block_codecs
  FastPFor_lib
)

add_executable(index_stats index_stats.cpp)
target_link_libraries(index_stats
  ${Boost_LIBRARIES}
  block_codecs
  FastPFor_lib
)

add_executable(sample_queries sample_queries.cpp)
target_link_libraries(sample_queries
  ${Boost_LIBRARIES}
  ${ZLIB_LIBRARIES}
  block_codecs
  FastPFor_lib
)

add_executable(iterate_edges iterate_edges.cpp)
target_link_libraries(iterate_edges
  ${Boost_LIBRARIES}
)

enable_testing()
add_subdirectory(test)
