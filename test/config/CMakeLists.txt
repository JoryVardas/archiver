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

# Variables which will be replaced
set(ARCHIVER_TEST_CONFIG_STAGE_DIRECTORY_VALUE "${CMAKE_CURRENT_BINARY_DIR}/stage")
set(ARCHIVER_TEST_CONFIG_ARCHIVE_DIRECTORY_VALUE "${CMAKE_CURRENT_BINARY_DIR}/archive")
set(ARCHIVER_TEST_CONFIG_ARCHIVE_TEMP_DIRECTORY_VALUE "${CMAKE_CURRENT_BINARY_DIR}/archive_temp")
set(ARCHIVER_TEST_CONFIG_DATABASE_USERNAME_VALUE ${ARCHIVER_TEST_DATABASE_USERNAME})
set(ARCHIVER_TEST_CONFIG_DATABASE_PASSWORD_VALUE ${ARCHIVER_TEST_DATABASE_PASSWORD})

# Copy additional files to output
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config_template.cpp ${CMAKE_CURRENT_BINARY_DIR}/config.cpp)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test_config_template.json ${CMAKE_CURRENT_BINARY_DIR}/test_config.json)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test_config_incomplete1.json ${CMAKE_CURRENT_BINARY_DIR}/test_config_incomplete1.json)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test_config_incomplete2.json ${CMAKE_CURRENT_BINARY_DIR}/test_config_incomplete2.json)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test_config_empty.json ${CMAKE_CURRENT_BINARY_DIR}/test_config_empty.json)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test_config_incorrect_type.json ${CMAKE_CURRENT_BINARY_DIR}/test_config_incorrect_type.json)

# Add test source to target
target_sources(Archiver-Tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/config.cpp)