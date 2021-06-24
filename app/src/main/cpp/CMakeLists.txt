# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.10.2)
project("laputa-ffmpeg")
# 两个参数 .:目录，表示当前目录 SOURCE:名称
aux_source_directory(. LAPUTA_SOURCE)

add_library(
        native-lib
        SHARED
        ${LAPUTA_SOURCE} )


#设置一个变量
set(laputa_libs ${CMAKE_SOURCE_DIR}/${ANDROID_ABI})
#-L：引入库查找路径
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${laputa_libs}")

#引入头文件
include_directories(include)

target_link_libraries(
        native-lib
        avfilter avformat  avcodec avutil swresample swscale rtmp android z
        OpenSLES
        log )