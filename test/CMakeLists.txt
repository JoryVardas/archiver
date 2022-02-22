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
target_compile_options(Archiver-Tests PRIVATE ${Archiver_Compile_Flags})
target_link_libraries(Archiver-Tests PRIVATE Catch2::Catch2WithMain)
# Include Archiver_sources target which builds the application code
target_link_libraries(Archiver-Tests PRIVATE Archiver_sources)
# Include the source directory as an include directory
target_include_directories(Archiver-Tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../)
target_include_directories(Archiver-Tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../)

include(CTest)
include(Catch)
# Add all tests to CTest
catch_discover_tests(Archiver-Tests)

# Get random test files
function(archiver_test_get_random_file size_to_download file_location output_sha_var output_blake_var output_size_var)
    # Using Random.org is a good option by ultimately not viable due to quotas
    # file(DOWNLOAD https://www.random.org/cgi-bin/randbyte?nbytes=${size_to_download}&format=f ${file_location})
    # use urandom instead
    execute_process(COMMAND head -c ${size_to_download} /dev/random OUTPUT_FILE ${file_location})
    execute_process(COMMAND sha512sum -b ${file_location} OUTPUT_VARIABLE downloaded_file_sha_results)
    execute_process(COMMAND b2sum -b ${file_location} OUTPUT_VARIABLE downloaded_file_blake_results)
    string(FIND ${downloaded_file_sha_results} " *" downloaded_file_sha_end)
    string(FIND ${downloaded_file_blake_results} " *" downloaded_file_blake_end)
    string(SUBSTRING ${downloaded_file_sha_results} 0 ${downloaded_file_sha_end} downloaded_file_sha)
    string(SUBSTRING ${downloaded_file_blake_results} 0 ${downloaded_file_blake_end} downloaded_file_blake)
    set(${output_sha_var} ${downloaded_file_sha} PARENT_SCOPE)
    set(${output_blake_var} ${downloaded_file_blake} PARENT_SCOPE)
    set(${output_size_var} ${size_to_download} PARENT_SCOPE)
endfunction()

archiver_test_get_random_file(1024
                              ${CMAKE_CURRENT_BINARY_DIR}/TestData1.test
                              ARCHIVER_TEST_TEST_DATA_1_SHA
                              ARCHIVER_TEST_TEST_DATA_1_BLAKE2B
                              ARCHIVER_TEST_TEST_DATA_1_SIZE)
archiver_test_get_random_file(5120
                              ${CMAKE_CURRENT_BINARY_DIR}/TestData_Single_Exact.test
                              ARCHIVER_TEST_TEST_DATA_SINGLE_EXACT_SHA
                              ARCHIVER_TEST_TEST_DATA_SINGLE_EXACT_BLAKE2B
                              ARCHIVER_TEST_TEST_DATA_SINGLE_EXACT_SIZE)
archiver_test_get_random_file(5121
                              ${CMAKE_CURRENT_BINARY_DIR}/TestData_Single.test
                              ARCHIVER_TEST_TEST_DATA_SINGLE_SHA
                              ARCHIVER_TEST_TEST_DATA_SINGLE_BLAKE2B
                              ARCHIVER_TEST_TEST_DATA_SINGLE_SIZE)
archiver_test_get_random_file(5119
                              ${CMAKE_CURRENT_BINARY_DIR}/TestData_Not_Single.test
                              ARCHIVER_TEST_TEST_DATA_NOT_SINGLE_SHA
                              ARCHIVER_TEST_TEST_DATA_NOT_SINGLE_BLAKE2B
                              ARCHIVER_TEST_TEST_DATA_NOT_SINGLE_SIZE)
file(COPY_FILE ${CMAKE_CURRENT_BINARY_DIR}/TestData_Not_Single.test ${CMAKE_CURRENT_BINARY_DIR}/TestData_Copy.test)
set(ARCHIVER_TEST_TEST_DATA_COPY_SHA ${ARCHIVER_TEST_TEST_DATA_NOT_SINGLE_SHA})
set(ARCHIVER_TEST_TEST_DATA_COPY_BLAKE2B ${ARCHIVER_TEST_TEST_DATA_NOT_SINGLE_BLAKE2B})
set(ARCHIVER_TEST_TEST_DATA_COPY_SIZE ${ARCHIVER_TEST_TEST_DATA_NOT_SINGLE_SIZE})

# Generate TestConstantTemplate
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test_constant_template.hpp ${CMAKE_CURRENT_BINARY_DIR}/test_constant.hpp)

# Add source files which may be required by all tests
target_sources(Archiver-Tests PRIVATE additional_matchers.cpp)

add_subdirectory(config)