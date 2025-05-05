cmake_minimum_required(VERSION 3.12)

project(AudioPadProj)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)

pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
message("Using GTK version: ${GTK3_VERSION}")


set(src_core
    AudioPad/nfd_gtk.cpp
    AudioPad/Media.cpp
    AudioPad/PlayList.cpp
    AudioPad/AudioPad.cpp
    AudioPad/imgui/imgui_demo.cpp
    AudioPad/imgui/imgui_draw.cpp
    AudioPad/imgui/imgui_impl_opengl2.cpp
    AudioPad/imgui/imgui_impl_sdl2.cpp
    AudioPad/imgui/imgui_tables.cpp
    AudioPad/imgui/imgui_widgets.cpp
    AudioPad/imgui/imgui.cpp
)

add_executable(AudioApp ${src_core})

target_include_directories(AudioApp PRIVATE ${GTK3_INCLUDE_DIRS})

target_link_libraries(AudioApp
    ${GTK3_LINK_LIBRARIES}
    m
    dl
    SDL2::SDL2
    SDL2_mixer
    OpenGL::GL
)
