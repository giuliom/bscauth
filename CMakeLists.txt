cmake_minimum_required(VERSION 3.23)
project(BascauthProject LANGUAGES CXX)

# Enable folders in IDEs (optional)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# GoogleTest requires at least C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Avoid warnings by using new policy
cmake_policy(SET CMP0135 NEW)

# ------ Fetch standalone Asio ------
# Official repository: https://github.com/chriskohlhoff/asio
include(FetchContent)
FetchContent_Declare(
  asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG asio-1-34-2
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(asio)

# Create an interface library for asio
add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)
target_compile_definitions(asio INTERFACE ${ASIO_DEFS})

# Ensure pthread is used on Unix-like systems
if(UNIX AND NOT APPLE)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
elseif(APPLE)
  find_package(Threads REQUIRED)
endif()

# Helper to link system libs needed by Asio (sockets, threads)
function(link_platform_libs target)
  if(WIN32)
    target_link_libraries(${target} PRIVATE ws2_32) # Winsock
  else()
    target_link_libraries(${target} PRIVATE Threads::Threads)
  endif()
endfunction()

# Shared definitions for standalone Asio
set(ASIO_DEFS
  ASIO_STANDALONE            # use standalone (no Boost)
  ASIO_NO_DEPRECATED         # optional: hide deprecated API
)

# ------ Producer ----- #
set (TARGET_PROD "producer")
file(GLOB LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/${TARGET_PROD}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/${TARGET_PROD}/*.cpp")

add_executable(${TARGET_PROD} ${LIB_SOURCES})
set_target_properties(${TARGET_PROD} PROPERTIES 
    LINKER_LANGUAGE CXX
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

# Set the library version (optional)
set_target_properties(${TARGET_PROD} PROPERTIES VERSION 1.0 SOVERSION 1)

target_link_libraries(${TARGET_PROD} PRIVATE asio)
target_compile_definitions(${TARGET_PROD} PRIVATE ${ASIO_DEFS})
target_include_directories(${TARGET_PROD} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
link_platform_libs(${TARGET_PROD})

install(TARGETS ${TARGET_PROD} DESTINATION lib)


# ------ Consumer ----- #
set (TARGET_CONS "consumer")
file(GLOB LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/${TARGET_CONS}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/${TARGET_CONS}/*.cpp")

add_executable(${TARGET_CONS} ${LIB_SOURCES})
set_target_properties(${TARGET_CONS} PROPERTIES 
    LINKER_LANGUAGE CXX
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

# Set the library version (optional)
set_target_properties(${TARGET_CONS} PROPERTIES VERSION 1.0 SOVERSION 1)

target_link_libraries(${TARGET_CONS} PRIVATE asio)
target_compile_definitions(${TARGET_CONS} PRIVATE ${ASIO_DEFS})
target_include_directories(${TARGET_CONS} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
link_platform_libs(${TARGET_CONS})

install(TARGETS ${TARGET_CONS} DESTINATION lib)