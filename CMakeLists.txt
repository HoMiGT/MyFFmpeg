cmake_minimum_required (VERSION 3.18)

project ("MyFFmpeg" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

#set(CMAKE_BUILD_TYPE Release)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_DEBUG_POSTFIX "")

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")
set(PLATFORM "x64-linux")
#set(LIB_SUFFIX ".so")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -O3 -pthread -std=c++17 -fvisibility=hidden")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

set(CMAKE_INSTALL_PATH "~/Projects/MyFFmpeg")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(OpenCV_DIR "/usr/local/opencv4/lib64/cmake/opencv4")
find_package(OpenCV REQUIRED)

set(ENV{PKG_CONFIG_PATH} "/usr/local/ffmpeg/lib/pkgconfig")
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavformat libavcodec libavutil libswscale libavfilter)

set(Python3_ROOT_DIR "/home/wpwl/miniconda3/envs/py310")
set(Python_ADDITIONAL_VERSIONS 3.10.11)
find_package(PythonInterp REQUIRED)
find_package(PythonLibs REQUIRED)

set(pybind11_DIR "/usr/local/pybind11/share/cmake/pybind11")
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG REQUIRED)


include_directories(
        "/usr/local/opencv4/include/opencv4"
        "/usr/local/pybind11/include"
        "/usr/local/ffmpeg/include"
        "/usr/local/opencv4/include/"
        "/home/wpwl/miniconda3/envs/py310/include/python3.10"
)

pybind11_add_module(MyFFmpeg MyFFmpeg.cpp)
#add_executable(MyFFmpeg MyFFmpeg.cpp MyFFmpeg.h MyFFmpegState.h)
target_include_directories(MyFFmpeg PRIVATE ${FFMPEG_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})
target_link_directories(MyFFmpeg PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(MyFFmpeg  
    PRIVATE 
        pybind11::module
        ${OpenCV_LIBS}
        ${FFMPEG_LIBRARIES}
        ${PYTHON_LIBRARIES}
)







