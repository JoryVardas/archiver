target_sources(Archiver_sources PRIVATE
               commandline_options.cpp
               common.cpp
               raw_file.cpp
               archiver.cpp
               dearchiver.cpp
               compressor.cpp
               stager.cpp
               )
target_sources(Archiver PRIVATE app.cpp)