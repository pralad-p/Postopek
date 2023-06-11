cmake_minimum_required(VERSION 3.16)

# --- Fetch FTXUI --------------------------------------------------------------
include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(ftxui
        GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
        GIT_TAG v4.1.1
        )

FetchContent_GetProperties(ftxui)
if (NOT ftxui_POPULATED)
    FetchContent_Populate(ftxui)
    add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif ()

# ------------------------------------------------------------------------------

if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

project(Postopek)

set(CMAKE_CXX_STANDARD 17)

set(SOURCES
        main.cpp
        src/Utilities.cpp
        src/Application.cpp
        src/ValidationCheck.cpp
        src/MarkdownParser.cpp
        )

set(HEADERS
        src/Utilities.h
        src/Application.h
        src/ValidationCheck.h
        src/WindowsUtilities.h
        src/StateTracker.h
        src/MarkdownParser.h
        )

add_executable(Postopek)

target_sources(Postopek PRIVATE ${SOURCES} ${HEADERS})

target_include_directories(Postopek PRIVATE src)

target_link_libraries(Postopek
        PRIVATE ftxui::screen
        PRIVATE ftxui::dom
        PRIVATE ftxui::component
        )
