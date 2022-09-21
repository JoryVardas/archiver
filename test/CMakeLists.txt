# Get Catch2 from the git repository
include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG devel
)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

# Create the target
add_executable(Archiver-Tests "")
set(Archiver_Test_Clang_GCC_Disabled_Warnings -Wno-unused-function)
target_compile_options(Archiver-Tests PRIVATE ${Archiver_Compile_Flags}
                       $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
                       ${Archiver_Test_Clang_GCC_Disabled_Warnings}>)
target_link_libraries(Archiver-Tests PRIVATE Catch2::Catch2WithMain)
# Include Archiver_sources target which builds the application code
target_link_libraries(Archiver-Tests PRIVATE Archiver_sources)
# Include the source directory as an include directory
target_include_directories(Archiver-Tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../)
target_include_directories(Archiver-Tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../)

# Prevent warnings from included files when using MSVC
if (MSVC)
    target_compile_options(Archiver_sources PRIVATE /external:anglebrackets /external:W0)
endif ()

include(CTest)
include(Catch)
# Add all tests to CTest
catch_discover_tests(Archiver-Tests)

# Get random test files
function(archiver_test_get_random_file size_to_download file_location output_sha_var output_blake_var output_size_var)
    execute_process(COMMAND python3 ${Archiver_Util_Path}/generate_test_file.py -n ${size_to_download} -o ${file_location} OUTPUT_VARIABLE file_hashes)

    string(REGEX MATCH "(SHA512:)[^\n\r]*" file_sha_match_part ${file_hashes})
    string(REGEX MATCH "(BLAKE2B:)[^\n\r]*" file_blake_match_part ${file_hashes})
    string(REGEX MATCH ":.*" file_sha_match_part2 ${file_sha_match_part})
    string(REGEX MATCH ":.*" file_blake_match_part2 ${file_blake_match_part})
    string(REGEX MATCH "[A-Za-z0-9]+" file_sha_match ${file_sha_match_part2})
    string(REGEX MATCH "[A-Za-z0-9]+" file_blake_match ${file_blake_match_part2})

    set(${output_sha_var} ${file_sha_match} PARENT_SCOPE)
    set(${output_blake_var} ${file_blake_match} PARENT_SCOPE)
    set(${output_size_var} ${size_to_download} PARENT_SCOPE)
endfunction()

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test_data)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test_data_additional)

archiver_test_get_random_file(1024
                              ${CMAKE_CURRENT_BINARY_DIR}/test_data/TestData1.test
                              ARCHIVER_TEST_TEST_DATA_1_SHA
                              ARCHIVER_TEST_TEST_DATA_1_BLAKE2B
                              ARCHIVER_TEST_TEST_DATA_1_SIZE)
archiver_test_get_random_file(5120
                              ${CMAKE_CURRENT_BINARY_DIR}/test_data/TestData_Single_Exact.test
                              ARCHIVER_TEST_TEST_DATA_SINGLE_EXACT_SHA
                              ARCHIVER_TEST_TEST_DATA_SINGLE_EXACT_BLAKE2B
                              ARCHIVER_TEST_TEST_DATA_SINGLE_EXACT_SIZE)
archiver_test_get_random_file(5121
                              ${CMAKE_CURRENT_BINARY_DIR}/test_data/TestData_Single.test
                              ARCHIVER_TEST_TEST_DATA_SINGLE_SHA
                              ARCHIVER_TEST_TEST_DATA_SINGLE_BLAKE2B
                              ARCHIVER_TEST_TEST_DATA_SINGLE_SIZE)
archiver_test_get_random_file(5119
                              ${CMAKE_CURRENT_BINARY_DIR}/test_data/TestData_Not_Single.test
                              ARCHIVER_TEST_TEST_DATA_NOT_SINGLE_SHA
                              ARCHIVER_TEST_TEST_DATA_NOT_SINGLE_BLAKE2B
                              ARCHIVER_TEST_TEST_DATA_NOT_SINGLE_SIZE)
file(COPY_FILE ${CMAKE_CURRENT_BINARY_DIR}/test_data/TestData_Not_Single.test ${CMAKE_CURRENT_BINARY_DIR}/test_data/TestData_Copy.test)
set(ARCHIVER_TEST_TEST_DATA_COPY_SHA ${ARCHIVER_TEST_TEST_DATA_NOT_SINGLE_SHA})
set(ARCHIVER_TEST_TEST_DATA_COPY_BLAKE2B ${ARCHIVER_TEST_TEST_DATA_NOT_SINGLE_BLAKE2B})
set(ARCHIVER_TEST_TEST_DATA_COPY_SIZE ${ARCHIVER_TEST_TEST_DATA_NOT_SINGLE_SIZE})

archiver_test_get_random_file(1024
                              ${CMAKE_CURRENT_BINARY_DIR}/test_data_additional/TestData_Additional_1.additional
                              ARCHIVER_TEST_TEST_DATA_ADDITIONAL_1_SHA
                              ARCHIVER_TEST_TEST_DATA_ADDITIONAL_1_BLAKE2B
                              ARCHIVER_TEST_TEST_DATA_ADDITIONAL_1_SIZE)
archiver_test_get_random_file(5120
                              ${CMAKE_CURRENT_BINARY_DIR}/test_data_additional/TestData_Additional_Single_Exact.additional
                              ARCHIVER_TEST_TEST_DATA_ADDITIONAL_SINGLE_EXACT_SHA
                              ARCHIVER_TEST_TEST_DATA_ADDITIONAL_SINGLE_EXACT_BLAKE2B
                              ARCHIVER_TEST_TEST_DATA_ADDITIONAL_SINGLE_EXACT_SIZE)

# Generate TestConstantTemplate
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test_constant_template.hpp ${CMAKE_CURRENT_BINARY_DIR}/test_constant.hpp)

# Add source files which may be required by all tests
target_sources(Archiver-Tests PRIVATE
               additional_matchers.cpp
               additional_generators.cpp)

# Add config tests
add_subdirectory(config)

# Make directories for staging, archiving, and dearchiving
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/stage")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/archive")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/archive_temp")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/dearchive")

# Add util tests
add_subdirectory(util)
add_subdirectory(database)

target_sources(Archiver-Tests PRIVATE
               stager.cpp
               archiver.cpp
               raw_file.cpp
               dearchiver.cpp)