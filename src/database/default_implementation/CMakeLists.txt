target_sources(Archiver PRIVATE
               staged_database.cpp
               )

execute_process(COMMAND python3 ${CMAKE_SOURCE_DIR}/lib/sqlpp11/scripts/ddl2cpp ${CMAKE_SOURCE_DIR}/src/database/default_implementation/archiver_database.sql ${CMAKE_SOURCE_DIR}/src/database/default_implementation/archiver_database archiver_database)