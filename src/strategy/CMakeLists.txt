include_directories(${CMAKE_INCLUDE_PATH})
link_directories(${CMAKE_LIBRARY_PATH})

set(BINARY_NAME demo)
aux_source_directory(. SRCs)

add_executable(${BINARY_NAME} ${SRCs})
target_link_libraries(${BINARY_NAME} framework)

if (NOT WIN32)
    find_library(HIREDIS_LIB hiredis)
    if (NOT HIREDIS_LIB)
        message(FATAL_ERROR "hiredis库未找到")
    endif()


# 添加jsoncpp库查找与链接
    find_library(JSONCPP_LIB jsoncpp)
    if (NOT JSONCPP_LIB)
        message(FATAL_ERROR "jsoncpp库未找到")
    endif()
endif()
if (WIN32)
# do nothing
else()
    target_link_libraries(${BINARY_NAME} ${HIREDIS_LIB} ${JSONCPP_LIB})
endif()
