target_sources(Archiver_sources PRIVATE
               archive.cpp
               commandline_options.cpp
               common.cpp
               directory.cpp
               raw_file.cpp
               staged_directory.cpp
               staged_file.cpp
               )
target_sources(Archiver PRIVATE app.cpp)