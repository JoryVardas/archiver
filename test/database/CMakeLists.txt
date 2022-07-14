# Give a warning if no database connection is possible.
if (NOT DEFINED ARCHIVER_TEST_DATABASE_USERNAME OR NOT DEFINED ARCHIVER_TEST_DATABASE_PASSWORD)
    message(WARNING
            "ARCHIVER_TEST_DATABASE_USERNAME and/or ARCHIVER_TEST_DATABASE_PASSWORD"
            " were not set and as such no connection to a database will be possible.")
endif ()

# Add test source to target
target_sources(Archiver-Tests PRIVATE
               staged_database.cpp
               archived_database.cpp)

# Add mock database
add_subdirectory(mock_implementation)
target_link_libraries(Archiver-Tests PRIVATE Archiver_mock_database)