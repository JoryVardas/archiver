target_sources(Archiver_sources PRIVATE
               commandline_options.cpp
               common.cpp
               raw_file.cpp
               staged_directory.cpp
               staged_file.cpp
               archived_directory.cpp
               )
target_sources(Archiver PRIVATE app.cpp)