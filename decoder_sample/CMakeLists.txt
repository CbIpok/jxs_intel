cmake_minimum_required(VERSION 3.10)
project(DecoderSample)

# Добавляем исполняемый файл
add_executable(DecoderSample "main.cpp")



# Определение конфигурации сборки (Debug или Release)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(LIBRARY_PATH "${CMAKE_SOURCE_DIR}/Bin/Debug/SvtJpegxs.lib")
    set(DLL_PATH "${CMAKE_SOURCE_DIR}/Bin/SvtJpegxs.dll")
else()
    set(LIBRARY_PATH "${CMAKE_SOURCE_DIR}/Bin/Release/SvtJpegxs.lib")
    set(DLL_PATH "${CMAKE_SOURCE_DIR}/Bin/SvtJpegxs.dll")
endif()

# Подключение библиотеки
target_link_libraries(DecoderSample "${LIBRARY_PATH}")


# Настройка для копирования DLL в выходной каталог сборки
add_custom_command(
    TARGET DecoderSample POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${DLL_PATH}"
        $<TARGET_FILE_DIR:DecoderSample>
)
# Добавьте путь к заголовочным файлам, если это необходимо
target_include_directories(DecoderSample PRIVATE ${CMAKE_SOURCE_DIR}/API)

