cmake_minimum_required (VERSION 3.18)

project ("MyFFmpeg" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

set(CMAKE_BUILD_TYPE "Release")

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
set(PLATFORM "x64-linux")
set(LIB_SUFFIX ".so")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 ")

set(CMAKE_PREFIX_PATH "$ENV{VCPKG_ROOT}/installed/x64-linux/share")

set(OpenCV_DIR "$ENV{VCPKG_ROOT}/installed/x64-linux/share/opencv4")
set(FFMPEG_DIR "$ENV{VCPKG_ROOT}/installed/x64-linux/share/ffmpeg")
set(pybind11_DIR "$ENV{VCPKG_ROOT}/installed/x64-linux/share/pybind11")
set(Protobuf_DIR "$ENV{VCPKG_ROOT}/installed/x64-linux/share/protobuf")
set(quirc_DIR "$ENV{VCPKG_ROOT}/installed/x64-linux/share/quirc")

list(APPEND CMAKE_MODULE_PATH "${FFMPEG_DIR}")

find_package(Python COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG REQUIRED)
find_package(OpenCV REQUIRED)
find_package(FFMPEG REQUIRED)

# find_library(LIBAVCODEC NAMES libavcodec.so.58.134.100 PATHS /usr/local/ffmpeg/lib)
# find_library(LIBAVFILTER NAMES libavfilter.so.7.110.100 PATHS /usr/local/ffmpeg/lib)
# find_library(LIBAVFORMAT NAMES libavformat.so.58.76.100 PATHS /usr/local/ffmpeg/lib)
# find_library(LIBAVUTIL NAMES libavutil.so.56.70.100 PATHS /usr/local/ffmpeg/lib)
# find_library(LIBAVSWRESAMPLE NAMES libswresample.so.3.9.100 PATHS /usr/local/ffmpeg/lib)
# find_library(LIBAVSWSCALE NAMES libswscale.so.5.9.100 PATHS /usr/local/ffmpeg/lib)

# set(LIBAVCODEC "/usr/local/ffmpeg/lib/libavcodec.so.58.134.100")
# set(LIBAVFILTER "/usr/local/ffmpeg/lib/libavfilter.so.7.110.100")
# set(LIBAVFORMAT "/usr/local/ffmpeg/lib/libavformat.so.58.76.100")
# set(LIBAVUTIL "/usr/local/ffmpeg/lib/libavutil.so.56.70.100")
# set(LIBAVSWRESAMPLE "/usr/local/ffmpeg/lib/libswresample.so.3.9.100")
# set(LIBAVSWSCALE "/usr/local/ffmpeg/lib/libswscale.so.5.9.100")

# list(APPEND FFmpeg_LIBS ${LIBAVCODEC} ${LIBAVFILTER} ${LIBAVFORMAT} ${LIBAVUTIL} ${LIBAVSWRESAMPLE} ${LIBAVSWSCALE})

include_directories(
    "$ENV{VCPKG_ROOT}/installed/x64-linux/include"
    # "/usr/local/ffmpeg/include"    
)
link_directories(
    "$ENV{VCPKG_ROOT}/installed/x64-linux/lib"
    # "/usr/local/ffmpeg/lib"
)

pybind11_add_module(MyFFmpeg MyFFmpeg.cpp)
target_include_directories(MyFFmpeg PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(MyFFmpeg PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(MyFFmpeg  
    PRIVATE 
    pybind11::module
    ${OpenCV_LIBS}
    ${FFMPEG_LIBRARIES}
)
# set_target_properties(MyFFmpeg PROPERTIES SUFFIX ".so")





