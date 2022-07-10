# Add test source to target
target_sources(Archiver-Tests PRIVATE
               get_file_read_buffer.cpp
               string_helpers/remove_prefix.cpp
               string_helpers/remove_suffix.cpp)