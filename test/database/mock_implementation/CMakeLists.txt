add_library(Archiver_mock_database OBJECT "")

target_compile_options(Archiver_mock_database PRIVATE ${Archiver_Compile_Flags})
target_link_libraries(Archiver_mock_database PUBLIC Archiver_sources)
target_include_directories(Archiver_mock_database PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../)

target_compile_options(Archiver_mock_database PRIVATE ${Archiver_Compile_Flags})

target_link_libraries(Archiver-Tests PRIVATE Archiver_mock_database)

target_sources(Archiver_mock_database PRIVATE
        staged_database.cpp
        archived_database.cpp
        )