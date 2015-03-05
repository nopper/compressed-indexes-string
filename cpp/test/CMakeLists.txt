file(GLOB TEST_SOURCES test_*.cpp)
foreach(TEST_SRC ${TEST_SOURCES})
  get_filename_component (TEST_SRC_NAME ${TEST_SRC} NAME_WE)
  add_executable(${TEST_SRC_NAME} ${TEST_SRC})
  target_link_libraries(${TEST_SRC_NAME}
    ${Boost_LIBRARIES}
    )
  add_test(${TEST_SRC_NAME} ${TEST_SRC_NAME})
endforeach(TEST_SRC)

target_link_libraries(test_sequences
  block_codecs
  FastPFor_lib
  )

target_link_libraries(test_index
  block_codecs
  FastPFor_lib
  succinct
  )

target_link_libraries(test_sequence_file
  block_codecs
  FastPFor_lib
  )

target_link_libraries(test_dicts
  cpi00_lib
  )

target_link_libraries(test_iterate_edges
  ${ZLIB_LIBRARIES}
  )

target_link_libraries(test_iterate_graph
  ${ZLIB_LIBRARIES}
  )

target_link_libraries(test_rmq
  succinct
  )

SET_TESTS_PROPERTIES(test_queues
    PROPERTIES ENVIRONMENT "PS_THREADS=4")

find_program (BASH_PROGRAM bash)
find_program (PYTHON_PROGRAM python)

if (BASH_PROGRAM)
  add_test (fulltest ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/fulltest.sh)
endif (BASH_PROGRAM)

if (PYTHON_PROGRAM)
  add_test (stats ${PYTHON_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/stats.py)
  add_test (comparison ${PYTHON_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/comparison.py)
  add_test (baseline_test ${PYTHON_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/baseline_test.py)
  add_test (topk_test ${PYTHON_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/topk_test.py)
endif (PYTHON_PROGRAM)

# we don't want to copy if we're building in the source dir
if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)

    # list of files for which we add a copy rule
    set(data_SHADOW
      test_data/coverage.tsv.gz
      test_data/ranking-coverage.tsv.gz
      test_data/simple-test.tsv.gz
      test_data/simple-graph.tsv.gz
      test_data/simple-names.tsv.gz)

    foreach(item IN LISTS data_SHADOW)
        message(STATUS ${item})
        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${item}"
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${item}" "${CMAKE_CURRENT_BINARY_DIR}/${item}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${item}"
        )
    endforeach()
endif()

add_custom_target(data-target ALL DEPENDS ${data_SHADOW})
