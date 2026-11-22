set(TARN_TREES ${CMAKE_CURRENT_BINARY_DIR}/trees)

add_executable(check_test check_test.c)
target_link_libraries(check_test PRIVATE tarn_check_objects)
tarn_strict_warnings(check_test)

add_test(NAME prepare-trees
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/trees
        -DDESTINATION=${TARN_TREES}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/prepare_trees.cmake)
set_tests_properties(prepare-trees PROPERTIES FIXTURES_SETUP trees)

file(GLOB TARN_CASES LIST_DIRECTORIES true CONFIGURE_DEPENDS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/trees
    ${CMAKE_CURRENT_SOURCE_DIR}/trees/*)
foreach(case IN LISTS TARN_CASES)
    add_test(NAME check/${case} COMMAND check_test ${TARN_TREES} ${case})
    set_tests_properties(check/${case} PROPERTIES FIXTURES_REQUIRED trees)
endforeach()

add_executable(generate_test generate_test.c)
target_link_libraries(generate_test PRIVATE tarn_check_objects)
tarn_strict_warnings(generate_test)

set(TARN_WRITE_TREES ${CMAKE_CURRENT_BINARY_DIR}/write-trees)
add_test(NAME prepare-write-trees
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/write-trees
        -DDESTINATION=${TARN_WRITE_TREES}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/prepare_trees.cmake)
set_tests_properties(prepare-write-trees PROPERTIES FIXTURES_SETUP write-trees)

file(GLOB TARN_WRITE_CASES LIST_DIRECTORIES true CONFIGURE_DEPENDS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/write-trees
    ${CMAKE_CURRENT_SOURCE_DIR}/write-trees/*)
foreach(case IN LISTS TARN_WRITE_CASES)
    add_test(NAME generate/${case} COMMAND generate_test ${TARN_WRITE_TREES} ${case})
    set_tests_properties(generate/${case} PROPERTIES FIXTURES_REQUIRED write-trees)
endforeach()

function(tarn_expect_exit name expected argument)
    add_test(NAME cli/${name}
        COMMAND ${CMAKE_COMMAND}
            -DPROGRAM=$<TARGET_FILE:tarn-check>
            -DARGUMENT=${argument}
            -DEXPECTED=${expected}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_exit.cmake)
    set_tests_properties(cli/${name} PROPERTIES FIXTURES_REQUIRED trees)
endfunction()

tarn_expect_exit(passes-a-clean-tree 0 ${TARN_TREES}/anchor-link)
tarn_expect_exit(refuses-a-broken-tree 1 ${TARN_TREES}/missing-readme)
tarn_expect_exit(refuses-an-unknown-option 2 --no-such-option)
tarn_expect_exit(refuses-a-missing-root 2 ${TARN_TREES}/no-such-tree)

add_test(NAME self COMMAND tarn-check ${PROJECT_SOURCE_DIR})
