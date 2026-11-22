set(TARN_TREES ${CMAKE_CURRENT_BINARY_DIR}/trees)
set(TARN_SNAPSHOTS ${CMAKE_CURRENT_SOURCE_DIR}/snapshots)

add_executable(check_test check_test.c test_snapshot.c)
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
    add_test(NAME check/${case} COMMAND check_test ${TARN_TREES} ${case} ${TARN_SNAPSHOTS})
    set_tests_properties(check/${case} PROPERTIES FIXTURES_REQUIRED trees)
endforeach()

add_executable(generate_test generate_test.c test_snapshot.c)
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
    add_test(NAME generate/${case} COMMAND generate_test ${TARN_WRITE_TREES} ${case} ${TARN_SNAPSHOTS})
    set_tests_properties(generate/${case} PROPERTIES FIXTURES_REQUIRED write-trees RESOURCE_LOCK write-trees)
endforeach()

function(tarn_expect_exit name expected)
    list(JOIN ARGN "|" arguments)
    add_test(NAME cli/${name}
        COMMAND ${CMAKE_COMMAND}
            -DPROGRAM=$<TARGET_FILE:tarn-check>
            -DARGUMENTS=${arguments}
            -DEXPECTED=${expected}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_exit.cmake)
    set_tests_properties(cli/${name} PROPERTIES FIXTURES_REQUIRED trees)
endfunction()

tarn_expect_exit(passes-a-clean-tree 0 ${TARN_TREES}/anchor-link)
tarn_expect_exit(refuses-a-broken-tree 1 ${TARN_TREES}/missing-readme)
tarn_expect_exit(refuses-an-unknown-option 2 --no-such-option)
tarn_expect_exit(refuses-a-missing-root 2 ${TARN_TREES}/no-such-tree)
tarn_expect_exit(refuses-a-public-tree-naming-a-private-repo 1
    --snapshot ${TARN_SNAPSHOTS}/intisy.snapshot --repository intisy/public ${TARN_TREES}/boundary-public-names-private)
tarn_expect_exit(reads-a-crlf-snapshot-with-a-bom 0
    --snapshot ${TARN_SNAPSHOTS}/crlf-bom.snapshot --repository intisy/tarn ${TARN_TREES}/anchor-link)
tarn_expect_exit(refuses-a-snapshot-without-a-repository 2
    --snapshot ${TARN_SNAPSHOTS}/intisy.snapshot ${TARN_TREES}/anchor-link)
tarn_expect_exit(refuses-a-repository-without-a-snapshot 2 --repository intisy/tarn ${TARN_TREES}/anchor-link)
tarn_expect_exit(refuses-a-repository-the-snapshot-lacks 2
    --snapshot ${TARN_SNAPSHOTS}/intisy.snapshot --repository intisy/nobody ${TARN_TREES}/anchor-link)
tarn_expect_exit(refuses-a-malformed-snapshot 2
    --snapshot ${TARN_SNAPSHOTS}/malformed.snapshot --repository intisy/tarn ${TARN_TREES}/anchor-link)
tarn_expect_exit(refuses-a-snapshot-with-a-blank-line 2
    --snapshot ${TARN_SNAPSHOTS}/blank-line.snapshot --repository intisy/tarn ${TARN_TREES}/anchor-link)
tarn_expect_exit(refuses-a-snapshot-with-a-trailing-space 2
    --snapshot ${TARN_SNAPSHOTS}/trailing-space.snapshot --repository intisy/tarn ${TARN_TREES}/anchor-link)
tarn_expect_exit(refuses-a-snapshot-with-a-tab 2
    --snapshot ${TARN_SNAPSHOTS}/tab.snapshot --repository intisy/tarn ${TARN_TREES}/anchor-link)
tarn_expect_exit(refuses-a-snapshot-with-a-missing-word 2
    --snapshot ${TARN_SNAPSHOTS}/missing-word.snapshot --repository intisy/tarn ${TARN_TREES}/anchor-link)
tarn_expect_exit(refuses-a-snapshot-with-an-extra-word 2
    --snapshot ${TARN_SNAPSHOTS}/extra-word.snapshot --repository intisy/tarn ${TARN_TREES}/anchor-link)
tarn_expect_exit(refuses-a-snapshot-listing-a-repo-twice 2
    --snapshot ${TARN_SNAPSHOTS}/duplicate.snapshot --repository intisy/tarn ${TARN_TREES}/anchor-link)
tarn_expect_exit(refuses-a-missing-snapshot 2
    --snapshot ${TARN_SNAPSHOTS}/no-such.snapshot --repository intisy/tarn ${TARN_TREES}/anchor-link)

add_test(NAME cli/write
    COMMAND ${CMAKE_COMMAND}
        -DPROGRAM=$<TARGET_FILE:tarn-check>
        -DTREE=${TARN_WRITE_TREES}/write-source-children
        -DWORKING=${CMAKE_CURRENT_BINARY_DIR}/cli-write
        -DFILE=docs/source/src/README.md
        -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_write.cmake)
set_tests_properties(cli/write PROPERTIES FIXTURES_REQUIRED write-trees RESOURCE_LOCK write-trees)

add_test(NAME self
    COMMAND tarn-check --snapshot ${TARN_SNAPSHOTS}/self.snapshot --repository intisy/tarn ${PROJECT_SOURCE_DIR})

add_test(NAME cli/warn-only-prints-warnings-and-exits-zero
    COMMAND ${CMAKE_COMMAND}
        -DPROGRAM=$<TARGET_FILE:tarn-check>
        -DARGUMENTS=--warn-only|${TARN_TREES}/missing-readme
        -DEXPECTED=0
        "-DHAS=^warned \\.:0 snapshot-absent "
        -DLACKS=refused
        -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_exit.cmake)
set_tests_properties(cli/warn-only-prints-warnings-and-exits-zero PROPERTIES FIXTURES_REQUIRED trees)

if(CMAKE_HOST_WIN32)
    set(TARN_STUB_GIT git.cmd)
else()
    set(TARN_STUB_GIT git)
endif()
set(TARN_STUB_GIT_FOLDER ${CMAKE_CURRENT_BINARY_DIR}/stub-git)
file(REMOVE_RECURSE ${TARN_STUB_GIT_FOLDER})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/stub-git/${TARN_STUB_GIT} DESTINATION ${TARN_STUB_GIT_FOLDER}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/stub-git/${TARN_STUB_GIT})
file(TO_NATIVE_PATH ${TARN_STUB_GIT_FOLDER} TARN_STUB_GIT_PATH)
set_tests_properties(check/source-doc-stale check/source-doc-shallow PROPERTIES ENVIRONMENT "PATH=${TARN_STUB_GIT_PATH}")

if(CMAKE_HOST_WIN32)
    file(TO_NATIVE_PATH ${TARN_TREES}/source-doc-stale TARN_STALE_TREE)
    set(TARN_STALE_TREE "${TARN_STALE_TREE}\\")
else()
    set(TARN_STALE_TREE "${TARN_TREES}/source-doc-stale/")
endif()
add_test(NAME cli/staleness-trims-a-trailing-separator
    COMMAND ${CMAKE_COMMAND}
        -DPROGRAM=$<TARGET_FILE:tarn-check>
        -DARGUMENTS=${TARN_STALE_TREE}
        -DEXPECTED=0
        "-DHAS=warned docs/source/src:0 source-doc-stale src has commits 20 days"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_exit.cmake)
set_tests_properties(cli/staleness-trims-a-trailing-separator PROPERTIES
    FIXTURES_REQUIRED trees ENVIRONMENT "PATH=${TARN_STUB_GIT_PATH}")
