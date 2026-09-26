cmake_minimum_required(VERSION 3.10)

project(r04-compiler LANGUAGES CXX)

include_directories(include)
include_directories(lib/ctpg/include)
add_definitions("-O0 -ggdb3 -std=c++20")

# tests
find_package(Catch2 REQUIRED)

file(GLOB_RECURSE TEST_FILES "${PROJECT_SOURCE_DIR}/tests/**/*.cpp")
set(ALL_TESTS all_tests)

add_executable(${ALL_TESTS} ${TEST_FILES})
target_link_libraries(${ALL_TESTS} PRIVATE Catch2::Catch2WithMain)
target_compile_options(${ALL_TESTS} PRIVATE --coverage)
target_link_options(${ALL_TESTS} PRIVATE --coverage)
add_test(NAME ${ALL_TESTS} COMMAND ${ALL_TESTS})

# compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(cmp_cmd_file "compile_commands.json")
if (EXISTS ${CMAKE_CURRENT_BINARY_DIR}/${cmp_cmd_file})
    configure_file(${CMAKE_CURRENT_BINARY_DIR}/${cmp_cmd_file}
                   ${CMAKE_CURRENT_SOURCE_DIR}/${cmp_cmd_file}
                   COPYONLY)
endif()

# coverage
find_program(GCOVR gcovr)
if(GCOVR)
    set(COVERAGE_DIR "${CMAKE_BINARY_DIR}/coverage")
    set(COVERAGE_HTML "${COVERAGE_DIR}/html")

    add_custom_target(coverage
        COMMAND rm -rf ${COVERAGE_DIR}
        COMMAND rm -f ${CMAKE_BINARY_DIR}/**/*.gcda
        COMMAND mkdir -p ${COVERAGE_HTML}
        COMMAND ${ALL_TESTS}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_DIR}
        COMMAND ${GCOVR} --html --html-details -o ${COVERAGE_HTML}/index.html
            --root=${PROJECT_SOURCE_DIR}
            --filter="${PROJECT_SOURCE_DIR}/src/"
            --filter="${PROJECT_SOURCE_DIR}/include/"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating coverage report with gcovr"
        DEPENDS ${TESTS}
    )

    # Открыть в браузере
    if(CMAKE_HOST_UNIX)
        add_custom_target(show-coverage
            COMMAND xdg-open ${COVERAGE_HTML}/index.html
            DEPENDS coverage
        )
    endif()
else()
    message(WARNING "gcovr not found")
endif()
