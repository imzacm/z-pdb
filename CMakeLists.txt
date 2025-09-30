cmake_minimum_required(VERSION 4.0)
project(z_pdb LANGUAGES C)

set(PROJECT_AUTHOR "Zac McChesney")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include_directories(${CMAKE_CURRENT_LIST_DIR}/cosmocc/include ${CMAKE_CURRENT_LIST_DIR}/cosmocc/lib)

# Only do these if this is the main project, and not if it is included through
# add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Optionally set things like CMAKE_CXX_STANDARD,
    # CMAKE_POSITION_INDEPENDENT_CODE here
    set(CMAKE_C_STANDARD 23)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_EXTENSIONS OFF)

    # Let's nicely support folders in IDEs
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

include(FetchContent)

FetchContent_Declare(zip
        GIT_REPOSITORY https://github.com/kuba--/zip.git
        GIT_TAG v0.3.5
        OVERRIDE_FIND_PACKAGE)

add_subdirectory(src)


include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)

if( supported )
    message(STATUS "IPO / LTO enabled")
    set_property(TARGET z_pdb PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO / LTO not supported: <${error}>")
endif()
