target_include_directories(Archiver_sources SYSTEM PUBLIC unfmt/include)
target_include_directories(Archiver_sources SYSTEM PUBLIC json/single_include/nlohmann)
target_include_directories(Archiver_sources SYSTEM PUBLIC fmt/include)
target_include_directories(Archiver_sources SYSTEM PUBLIC cxxopts/include)
target_include_directories(Archiver_sources SYSTEM PUBLIC spdlog/include)
target_include_directories(Archiver_sources SYSTEM PUBLIC date/include)
target_include_directories(Archiver_sources SYSTEM PUBLIC sqlpp11/include)
target_include_directories(Archiver_sources SYSTEM PUBLIC cxxsubs/include)

target_compile_definitions(Archiver_sources PUBLIC FMT_HEADER_ONLY)

find_package(MySQL REQUIRED)
target_include_directories(Archiver_sources SYSTEM PUBLIC ${MySQL_INCLUDE_DIR})
target_link_libraries(Archiver_sources ${MySQL_LIBRARY})

add_subdirectory(sqlpp11)

# The above includes are marked as SYSTEM to prevent warnings, but CMAKE can't
# enforce that on MSVC since the compiler flags are new, so manually add them
# if MSVC is the compiler.
if (MSVC)
    target_compile_options(Archiver_sources PRIVATE /external:anglebrackets /external:W0)
endif ()