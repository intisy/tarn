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

add_test(NAME cli/refuses-a-broken-tree COMMAND tarn-check ${TARN_TREES}/missing-readme)
set_tests_properties(cli/refuses-a-broken-tree PROPERTIES FIXTURES_REQUIRED trees WILL_FAIL TRUE)

add_test(NAME cli/refuses-an-unknown-option COMMAND tarn-check --no-such-option)
set_tests_properties(cli/refuses-an-unknown-option PROPERTIES WILL_FAIL TRUE)

add_test(NAME self COMMAND tarn-check ${PROJECT_SOURCE_DIR})
