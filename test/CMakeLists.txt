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
target_link_libraries(Archiver-Tests PRIVATE Catch2::Catch2WithMain)
# Include Archiver_sources target which builds the application code
target_link_libraries(Archiver-Tests PRIVATE Archiver_sources)
# Include the source directory as an include directory
target_include_directories(Archiver-Tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../)

include(CTest)
include(Catch)
# Add all tests to CTest
catch_discover_tests(Archiver-Tests)

if (NOT DEFINED ARCHIVER_TEST_DATABASE_USERNAME)
    message(WARNING "ARCHIVER_TEST_DATABASE_USERNAME was not set and as such
     no connection to a database will be possible.")
    set(ARCHIVER_TEST_DATABASE_USERNAME "testDatabaseUsername")
endif ()
if (NOT DEFINED ARCHIVER_TEST_DATABASE_PASSWORD)
    message(WARNING "ARCHIVER_TEST_DATABASE_PASSWORD was not set and as such
     no connection to a database will be possible.")
    set(ARCHIVER_TEST_DATABASE_PASSWORD "testDatabasePassword")
endif ()

# Variables which will be replaced in test_config_template.json
set(ARCHIVER_TEST_CONFIG_STAGE_DIRECTORY_MATCH "{StageDirectory}")
set(ARCHIVER_TEST_CONFIG_STAGE_DIRECTORY_VALUE "${CMAKE_CURRENT_BINARY_DIR}/stage")
set(ARCHIVER_TEST_CONFIG_ARCHIVE_DIRECTORY_MATCH "{ArchiveDirectory}")
set(ARCHIVER_TEST_CONFIG_ARCHIVE_DIRECTORY_VALUE "${CMAKE_CURRENT_BINARY_DIR}/archive")
set(ARCHIVER_TEST_CONFIG_ARCHIVE_TEMP_DIRECTORY_MATCH "{ArchiveTempDirectory}")
set(ARCHIVER_TEST_CONFIG_ARCHIVE_TEMP_DIRECTORY_VALUE "${CMAKE_CURRENT_BINARY_DIR}/archive_temp")
set(ARCHIVER_TEST_CONFIG_DATABASE_USERNAME_MATCH "{TestDatabaseUser}")
set(ARCHIVER_TEST_CONFIG_DATABASE_USERNAME_VALUE ${ARCHIVER_TEST_DATABASE_USERNAME})
set(ARCHIVER_TEST_CONFIG_DATABASE_PASSWORD_MATCH "{TestDatabasePassword}")
set(ARCHIVER_TEST_CONFIG_DATABASE_PASSWORD_VALUE ${ARCHIVER_TEST_DATABASE_PASSWORD})

list(APPEND ARCHIVER_TEST_CONFIG_VARIABLES_TO_REPLACE
     ARCHIVER_TEST_CONFIG_STAGE_DIRECTORY
     ARCHIVER_TEST_CONFIG_ARCHIVE_DIRECTORY
     ARCHIVER_TEST_CONFIG_ARCHIVE_TEMP_DIRECTORY
     ARCHIVER_TEST_CONFIG_DATABASE_USERNAME
     ARCHIVER_TEST_CONFIG_DATABASE_PASSWORD
     )

# Modify test_config_template.json and output it as test_config.json
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/test_config_template.json ARCHIVER_TEST_CONFIG_JSON)

foreach (ARCHIVER_TEST_CONFIG_VARIABLE_TO_REPLACE ${ARCHIVER_TEST_CONFIG_VARIABLES_TO_REPLACE})
    string(REPLACE "${${ARCHIVER_TEST_CONFIG_VARIABLE_TO_REPLACE}_MATCH}"
           "${${ARCHIVER_TEST_CONFIG_VARIABLE_TO_REPLACE}_VALUE}" ARCHIVER_TEST_CONFIG_JSON ${ARCHIVER_TEST_CONFIG_JSON})
endforeach ()

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/test_config.json ${ARCHIVER_TEST_CONFIG_JSON})

# Add test source to target
target_sources(Archiver-Tests PRIVATE test1.cpp)