target_include_directories(Archiver SYSTEM PUBLIC unfmt/include)
target_include_directories(Archiver SYSTEM PUBLIC json/single_include/nlohmann)
target_include_directories(Archiver SYSTEM PUBLIC fmt/include)
target_include_directories(Archiver SYSTEM PUBLIC cxxopts/include)
target_include_directories(Archiver SYSTEM PUBLIC spdlog/include)
target_include_directories(Archiver SYSTEM PUBLIC date/include)
target_include_directories(Archiver SYSTEM PUBLIC sqlpp11/include)
target_include_directories(Archiver SYSTEM PUBLIC cxxsubs/include)

find_package(MySQL REQUIRED)
target_include_directories(Archiver SYSTEM PUBLIC ${MySQL_INCLUDE_DIR})
target_link_libraries(Archiver ${MySQL_LIBRARY})

add_subdirectory(sqlpp11)