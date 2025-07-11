
cmake_minimum_required(VERSION 3.10)

project(statestorage)
set(TRIE_EXECUTABLE_NAME trieTest)
set(RTREE_EXECUTABLE_NAME rTreeTest)

set(CMAKE_CXX_STANDARD 17) # Change this if you want c++11 or 20 rather than 17
set(CMAKE_CXX_STANDARD_REQUIRED True)

# set(STORM_PATH /home/landon/storm)
message("For portability, if you have a custom Storm install path, please pass it in during CMake invocation via cmake .. <params> -DSTORM_PATH=/path/to/storm")
message("STORM_PATH is currently set as ${STORM_PATH}")

option(STATE_STORAGE_DEBUG "Include debug information" OFF)
if (STATE_STORAGE_DEBUG)
	set(CMAKE_BUILD_TYPE Debug)
endif()

find_package(Boost)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    include_directories(${Boost_INCLUDES})
endif (Boost_FOUND)

ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

include_directories("src")

set(SOURCE_DIR src)
set(SOURCE_FILES
	# Add your source files here.
	${SOURCE_DIR}/main.cpp
	${SOURCE_DIR}/Trie.cpp
)

find_package(storm REQUIRED PATHS ${STORM_PATH})

# Create and link the executable
add_executable(${TRIE_EXECUTABLE_NAME} ${SOURCE_FILES})
target_include_directories(${TRIE_EXECUTABLE_NAME} PUBLIC ${SOURCE_DIR} ${storm_INCLUDE_DIR} ${storm-parsers_INCLUDE_DIR} ${STORM_PATH} ${LIB_PATH})
target_link_libraries(${TRIE_EXECUTABLE_NAME} PUBLIC storm storm-parsers)

install(TARGETS ${TRIE_EXECUTABLE_NAME} RUNTIME DESTINATION lib COMPONENT libraries)
# install(TARGETS ${RTREE_EXECUTABLE_NAME} RUNTIME DESTINATION lib COMPONENT libraries)
