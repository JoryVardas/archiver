# Get Catch2 from the git repository
include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG devel
)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

# Create the target
add_executable(Archiver-Tests "")
target_link_libraries(Archiver-Tests PRIVATE Catch2::Catch2WithMain)
# Include Archiver_sources target which builds the application code
target_link_libraries(Archiver-Tests PRIVATE Archiver_sources)
# Include the source directory as an include directory
target_include_directories(Archiver-Tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../)

include(CTest)
include(Catch)
# Add all tests to CTest
catch_discover_tests(Archiver-Tests)

# Add test source to target
target_sources(Archiver-Tests PRIVATE "")

add_subdirectory(config)