cmake_minimum_required (VERSION 3.18)

project ("MyFFmpeg" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

set(CMAKE_BUILD_TYPE "Release")

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")
set(PLATFORM "x64-linux")
set(LIB_SUFFIX ".so")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 ")

set(CMAKE_INSTALL_PATH "~/Projects/MyFFmpeg")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(protobuf_DIR "$ENV{VCPKG_ROOT}/installed/x64-linux-dynamic/share/protobuf")
set(Protobuf_DIR "$ENV{VCPKG_ROOT}/installed/x64-linux-dynamic/share/protobuf")
set(OpenCV_DIR "$ENV{VCPKG_ROOT}/installed/x64-linux-dynamic/share/opencv4")
set(FFMPEG_DIR "$ENV{VCPKG_ROOT}/installed/x64-linux-dynamic/share/ffmpeg")
set(pybind11_DIR "$ENV{VCPKG_ROOT}/installed/x64-linux-dynamic/share/pybind11")
set(quirc_DIR "$ENV{VCPKG_ROOT}/installed/x64-linux-dynamic/share/quirc")

list(APPEND CMAKE_MODULE_PATH "${FFMPEG_DIR}")

find_package(protobuf CONFIG REQUIRED)
find_package(Python COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG REQUIRED)
find_package(OpenCV REQUIRED)
find_package(FFMPEG REQUIRED)


include_directories(
    "$ENV{VCPKG_ROOT}/installed/x64-linux-dynamic/include"
)
link_directories(
    "$ENV{VCPKG_ROOT}/installed/x64-linux-dynamic/lib"
)

pybind11_add_module(MyFFmpeg MyFFmpeg.cpp)
target_include_directories(MyFFmpeg PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(MyFFmpeg PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(MyFFmpeg  
    PRIVATE 
    pybind11::module
    ${OpenCV_LIBS}
    ${FFMPEG_LIBRARIES}
    protobuf::libprotoc 
    protobuf::libprotobuf 
    protobuf::libprotobuf-lite
)

install(TARGETS MyFFmpeg
        RUNTIME DESTINATION ${CMAKE_INSTALL_PATH}/lib
        LIBRARY DESTINATION ${CMAKE_INSTALL_PATH}/lib)

# list(APPEND MY_SHARED_LIBS
#     $ENV{VCPKG_ROOT}/installed/x64-linux-dynamic/lib/libavcodec.so.59.37.100
#     $ENV{VCPKG_ROOT}/installed/x64-linux-dynamic/lib/libavfilter.so.8.44.100
#     $ENV{VCPKG_ROOT}/installed/x64-linux-dynamic/lib/libavformat.so.59.27.100
#     $ENV{VCPKG_ROOT}/installed/x64-linux-dynamic/lib/libavutil.so.57.28.100
#     $ENV{VCPKG_ROOT}/installed/x64-linux-dynamic/lib/libswresample.so.4.7.100
#     $ENV{VCPKG_ROOT}/installed/x64-linux-dynamic/lib/libswscale.so.6.7.100
#     $ENV{VCPKG_ROOT}/installed/x64-linux-dynamic/lib/libopencv_core4.so.4.7.0
#     $ENV{VCPKG_ROOT}/installed/x64-linux-dynamic/lib/libopencv_imgcodecs4.so.4.7.0
#     $ENV{VCPKG_ROOT}/installed/x64-linux-dynamic/lib/libopencv_imgproc4.so.4.7.0
# )
# 
# foreach(SHARED_LIB ${MY_SHARED_LIBS})
#     file(COPY ${SHARED_LIB} DESTINATION /home/c/projects/MyFFmpeg/lib)
# endforeach()







