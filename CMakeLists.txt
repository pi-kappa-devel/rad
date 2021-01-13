cmake_minimum_required (VERSION 3.6)

## project details
project(rad VERSION 1.4.4 LANGUAGES C)
set(PROJECT_NAME_UPPER "RAD")
set(PROJECT_BRIEF "Radial Attention Model")

# generate JSON compilation commands export
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

## c compiler setup
set(CMAKE_C_STANDARD 11)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Build standard: ${CMAKE_C_STANDARD}")
if(CMAKE_BUILD_TYPE MATCHES Release)
  if(MSVC)
    set(CMAKE_C_FLAGS_RELEASE "/O2")
  else()
    set(CMAKE_C_FLAGS_RELEASE "-O3")
  endif()
  message(STATUS "Build flags: ${CMAKE_C_FLAGS_RELEASE}")

  set(PROJECT_LOG_CYCLE 20)
  set(PROJECT_SAVE_CYCLE 100)
elseif(CMAKE_BUILD_TYPE MATCHES Debug)
  if(MSVC)
    set(CMAKE_C_FLAGS_DEBUG "/W3 /DEBUG /ZI /Od")
  else()
    set(CMAKE_C_FLAGS_DEBUG "-Wall -gdwarf-3")
  endif()
  message(STATUS "Build flags: ${CMAKE_C_FLAGS_DEBUG}")
  
  set(PROJECT_COMPILER_DEFINITIONS ${PROJECT_COMPILER_DEFINITIONS}
    GRID_T_SAFE_MODE
    PMAP_T_SAFE_MODE 
    RAD_SAFE_MODE 
    RAD_DEBUG)
  set(PROJECT_LOG_CYCLE 1)
  set(PROJECT_SAVE_CYCLE 10)
endif()

## paths 
# set variables
set(C_DIR ${PROJECT_SOURCE_DIR}/crad)
set(C_INCLUDE_DIR ${C_DIR}/include)
set(C_SOURCE_DIR ${C_DIR}/src)
set(PYTHON_DIR ${PROJECT_SOURCE_DIR}/prad)
set(PROJECT_CONF_DIR ${PROJECT_SOURCE_DIR}/conf)
set(PROJECT_DOCS_DIR ${PROJECT_SOURCE_DIR}/docs)
set(PROJECT_DATA_DIR ${PROJECT_SOURCE_DIR}/data)
set(PROJECT_TEMP_DIR ${PROJECT_SOURCE_DIR}/tmp)
if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
      ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
endif()

# create paths 
file(MAKE_DIRECTORY ${PROJECT_DOCS_DIR})
file(MAKE_DIRECTORY ${PROJECT_DATA_DIR})
file(MAKE_DIRECTORY ${PROJECT_TEMP_DIR})
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

## libraries
# threading configuration
find_package(Threads)
if(${THREADS_FOUND})
  if(NOT DEFINED PROJECT_NUM_THREADS)
    # find available processors
    include(ProcessorCount)
    ProcessorCount(PROJECT_NUM_PROCESSORS)
    message(STATUS "Cores found: ${PROJECT_NUM_PROCESSORS}")
    set(PROJECT_NUM_THREADS ${PROJECT_NUM_PROCESSORS}-1)
    math(EXPR PROJECT_NUM_THREADS ${PROJECT_NUM_THREADS})
  endif()
else()
  set(PROJECT_NUM_THREADS 0)
endif()
message(STATUS "Setting number of worker threads: ${PROJECT_NUM_THREADS}")

## header files
file(GLOB C_HEADERS "${C_INCLUDE_DIR}/*.h")

## source files
file(GLOB C_SOURCES "${C_SOURCE_DIR}/*.c")

## configuration
if(MSVC)
  string(REGEX REPLACE "/" "\\\\\\\\" PROJECT_DOCS_DIR ${PROJECT_DOCS_DIR}) 
  string(REGEX REPLACE "/" "\\\\\\\\" PROJECT_DATA_DIR ${PROJECT_DATA_DIR}) 
  string(REGEX REPLACE "/" "\\\\\\\\" PROJECT_TEMP_DIR ${PROJECT_TEMP_DIR}) 
endif()
configure_file("${PROJECT_CONF_DIR}/${PROJECT_NAME}.h.cnf"
               "${C_INCLUDE_DIR}/${PROJECT_NAME}_conf.h")
configure_file("${PROJECT_CONF_DIR}/${PROJECT_NAME}.py.cnf"
	       "${PYTHON_DIR}/${PROJECT_NAME}_conf.py")
configure_file("${PROJECT_CONF_DIR}/setup.cfg"
	       "${PROJECT_SOURCE_DIR}/setup.cfg")
configure_file("${PROJECT_CONF_DIR}/.clang-format"
	       "${PROJECT_SOURCE_DIR}/.clang-format")
execute_process(COMMAND cmake -E create_symlink
                "${PROJECT_SOURCE_DIR}/build/compile_commands.json"
                "compile_commands.json")


## add executable targets
set(EXEC_TARGETS msol mcont pardep)
foreach(EXEC_TARGET ${EXEC_TARGETS}) 
  set(TARGET_NAME ${PROJECT_NAME}_${EXEC_TARGET})
  message(STATUS "Creating executable target: ${TARGET_NAME}")
  source_group("HEADERS" FILES ${C_HEADERS})
  set(OTHER_TARGETS ${EXEC_TARGETS})
  list(FILTER OTHER_TARGETS EXCLUDE REGEX ".*${EXEC_TARGET}")
  set(TARGET_SOURCES ${C_SOURCES})
  foreach(OTHER_TARGET ${OTHER_TARGETS}) 
    list(FILTER TARGET_SOURCES EXCLUDE REGEX ".*${OTHER_TARGET}")
  endforeach()
  add_executable(${TARGET_NAME} ${TARGET_SOURCES} ${C_HEADERS})
  target_compile_definitions(${TARGET_NAME} PUBLIC ${PROJECT_COMPILER_DEFINITIONS})
  set_target_properties(${TARGET_NAME} PROPERTIES 
                        LINKER_LANGUAGE C
                        OUTPUT_NAME ${TARGET_NAME}${PROJECT_VERSION})
  target_link_libraries(${TARGET_NAME} ${CMAKE_THREAD_LIBS_INIT})
  if(NOT MSVC)
    target_link_libraries(${TARGET_NAME} m)
  endif()
  target_include_directories(${TARGET_NAME} PUBLIC ${C_INCLUDE_DIR})
endforeach()

## add documentation target
find_package(Doxygen)
if(DOXYGEN_FOUND)
  if(MSVC)
    set(PY_FILTER_DOCS "py_filter.bat")
    set(MAKE_DOCS "${PROJECT_TEMP_DIR}/make_docs.bat")
    file(COPY "${PROJECT_CONF_DIR}/make_docs.bat" DESTINATION "${PROJECT_TEMP_DIR}")
  else()
    set(PY_FILTER_DOCS "./py_filter")
    set(MAKE_DOCS "${PROJECT_TEMP_DIR}/make_docs.sh")
    file(COPY "${PROJECT_CONF_DIR}/make_docs.sh"
         DESTINATION "${PROJECT_TEMP_DIR}" 
         FILE_PERMISSIONS 
         OWNER_READ OWNER_WRITE OWNER_EXECUTE
	 GROUP_READ GROUP_EXECUTE
	 WORLD_READ WORLD_EXECUTE)
  endif()

  message(STATUS "Creating documentation target: ${PROJECT_NAME}_doc")
  set(DOXYGEN_CONF ${PROJECT_TEMP_DIR}/${PROJECT_NAME}.doxy)
  configure_file(${PROJECT_CONF_DIR}/${PROJECT_NAME}_doxy.cnf ${DOXYGEN_CONF} @ONLY)

  add_custom_target(${PROJECT_NAME}_doc
                    COMMAND ${MAKE_DOCS}
                    ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CONF}
                    ${PYTHON_DIR} ${PROJECT_NAME}
                    ${PROJECT_DOCS_DIR}
                    WORKING_DIRECTORY ${PROJECT_TEMP_DIR}
                    COMMENT "Generating documentation with Doxygen"
                    VERBATIM)
endif()
