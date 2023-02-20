cmake_minimum_required(VERSION 3.22)

project(process-server
        VERSION 0.0.1
        DESCRIPTION ""
        LANGUAGES C)

set(CMAKE_C_STANDARD 17)

set(SOURCE_DIR src)
set(INCLUDE_DIR include)
set(SOURCE_LIST
        ${SOURCE_DIR}/api_functions.c
        ${SOURCE_DIR}/process_server.c
        ${SOURCE_DIR}/setup_teardown.c
        #=vvvv= SOURCE FOR DUMMY MAIN =vvvv=#
#        ${SOURCE_DIR}/test_main.c
#        ../core/src/util.c
        )
set(HEADER_LIST
        ${INCLUDE_DIR}/objects.h
        ${INCLUDE_DIR}/process_server.h
        ${INCLUDE_DIR}/setup_teardown.h
        ../api_functions.h
        #=vvvv= INCLUDES FOR DUMMY MAIN =vvvv=#
#        ../core/include/util.h
        )

set(SANITIZE TRUE)

add_compile_definitions(_POSIX_C_SOURCE=200809L)
add_compile_definitions(_XOPEN_SOURCE=700)

if (APPLE)
    add_definitions(-D_DARWIN_C_SOURCE)
endif ()

include_directories(${INCLUDE_DIR})
add_compile_options("-Wall"
        "-Wextra"
        "-Wpedantic"
        "-Wshadow"
        "-Wstrict-overflow=4"
        "-Wswitch-default"
        "-Wswitch-enum"
        "-Wunused"
        "-Wunused-macros"
        "-Wdate-time"
        "-Winvalid-pch"
        "-Wmissing-declarations"
        "-Wmissing-include-dirs"
        "-Wmissing-prototypes"
        "-Wstrict-prototypes"
        "-Wundef"
        "-Wnull-dereference"
        "-Wstack-protector"
        "-Wdouble-promotion"
        "-Wvla"
        "-Walloca"
        "-Woverlength-strings"
        "-Wdisabled-optimization"
        "-Winline"
        "-Wcast-qual"
        "-Wfloat-equal"
        "-Wformat=2"
        "-Wfree-nonheap-object"
        "-Wshift-overflow"
        "-Wwrite-strings")

if (${SANITIZE})
    add_compile_options("-fsanitize=address")
    add_compile_options("-fsanitize=undefined")
    add_compile_options("-fsanitize-address-use-after-scope")
    add_compile_options("-fstack-protector-all")
    add_compile_options("-fdelete-null-pointer-checks")
    add_compile_options("-fno-omit-frame-pointer")

    if (NOT APPLE)
        add_compile_options("-fsanitize=leak")
    endif ()

    add_link_options("-fsanitize=address")
    add_link_options("-fsanitize=bounds")
endif ()

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
    #    add_compile_options("-O2")
    add_compile_options("-Wcast-align"
            "-Wunsuffixed-float-constants"
            "-Warith-conversion"
            "-Wcast-align=strict"
            "-Wunsafe-loop-optimizations"
            "-Wvector-operation-performance"
            "-Walloc-zero"
            "-Wtrampolines"
            "-Wtsan"
            "-Wformat-overflow=2"
            "-Wformat-signedness"
            "-Wjump-misses-init"
            "-Wformat-truncation=2")
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
endif ()

find_package(Doxygen
        REQUIRED
        REQUIRED dot
        OPTIONAL_COMPONENTS mscgen dia)

set(DOXYGEN_ALWAYS_DETAILED_SEC YES)
set(DOXYGEN_REPEAT_BRIEF YES)
set(DOXYGEN_EXTRACT_ALL YES)
set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)
set(DOXYGEN_GENERATE_HTML YES)
set(DOXYGEN_WARNINGS YES)
set(DOXYGEN_QUIET YES)

doxygen_add_docs(doxygen
        ${HEADER_LIST}
        WORKING_DIRECTORY ..
        COMMENT "Generating Doxygen documentation for process-server")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CLANG_TIDY_CHECKS "*")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-llvmlibc-restrict-system-libc-headers")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-misc-unused-parameters")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-clang-diagnostic-unused-parameter")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-clang-diagnostic-unused-variable")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-cppcoreguidelines-init-variables")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-readability-identifier-length")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-clang-diagnostic-unused-but-set-variable")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-clang-analyzer-deadcode.DeadStores")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-altera-id-dependent-backward-branch")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-cert-dcl03-c")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-hicpp-static-assert")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-misc-static-assert")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-altera-unroll-loops")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-altera-struct-pack-align")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-clang-analyzer-security.insecureAPI.strcpy")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-bugprone-easily-swappable-parameters")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-android-cloexec-open")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-android-cloexec-accept")
set(CMAKE_C_CLANG_TIDY clang-tidy -checks=${CLANG_TIDY_CHECKS};--quiet)

#========= vvv COMPILE AS LIBRARY vvv =========#

add_library(process-server SHARED ${SOURCE_LIST} ${HEADER_LIST})
target_include_directories(process-server PRIVATE include/process-server)
target_include_directories(process-server PRIVATE /usr/local/include)
target_link_directories(process-server PRIVATE /usr/local/lib)

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_include_directories(process-server PRIVATE /usr/include)
endif ()

set_target_properties(process-server PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR})

get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)

if ("${LIB64}" STREQUAL "TRUE")
    set(LIBSUFFIX 64)
else()
    set(LIBSUFFIX "")
endif()

set(INSTALL_LIB_DIR lib${LIBSUFFIX} CACHE PATH "Installation directory for libraries")
mark_as_advanced(INSTALL_LIB_DIR)

install(TARGETS process-server LIBRARY DESTINATION ${INSTALL_LIB_DIR})
install(FILES ${HEADER_LIST} DESTINATION include/process-server)

#========= ^^^ COMPILE AS LIBRARY ^^^ =========#
#========= vvv COMPILE AS EXECUTABLE vvv =========#

#add_executable(process-server ${SOURCE_LIST})
#target_include_directories(process-server PRIVATE /usr/local/include)

#========= ^^^ COMPILE AS EXECUTABLE ^^^ =========#

add_dependencies(process-server doxygen)

find_library(LIBDC_ERROR dc_error REQUIRED)
find_library(LIBDC_ENV dc_env REQUIRED)
find_library(LIBDC_C dc_c REQUIRED)
find_library(LIBDC_POSIX dc_posix REQUIRED)
find_library(LIBDC_UNIX dc_unix REQUIRED)
find_library(LIBDC_UTIL dc_util REQUIRED)
find_library(LIBDC_FSM dc_fsm REQUIRED)
find_library(LIB_CONFIG config REQUIRED)
find_library(LIBDC_APPLICATION dc_application REQUIRED)
find_library(MEM_MANAGER mem_manager REQUIRED)

target_link_libraries(process-server PUBLIC ${LIBDC_ERROR})
target_link_libraries(process-server PUBLIC ${LIBDC_ENV})
target_link_libraries(process-server PUBLIC ${LIBDC_C})
target_link_libraries(process-server PUBLIC ${LIBDC_POSIX})
target_link_libraries(process-server PUBLIC ${LIBDC_UNIX})
target_link_libraries(process-server PUBLIC ${LIBDC_UTIL})
target_link_libraries(process-server PUBLIC ${LIBDC_FSM})
target_link_libraries(process-server PUBLIC ${LIB_CONFIG})
target_link_libraries(process-server PUBLIC ${LIBDC_APPLICATION})
target_link_libraries(process-server PUBLIC ${MEM_MANAGER})
