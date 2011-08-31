macro(add_photk_executable name)
  add_executable(${name} ${ARGN})
  foreach(lib ${PHOTK_USED_LIBS})
    if( NOT ${lib} STREQUAL "optimized" AND
        NOT ${lib} STREQUAL "debug" )
      target_link_libraries( ${name} ${lib} )
    endif()
  endforeach(lib)
  if ( PHOTK_ENABLE_PROFILING )
    target_link_libraries( ${name} ${PROFILER_LIBRARY} )
  endif()
  if ( PHOTK_ENABLE_TCMALLOC )
    target_link_libraries( ${name} ${TCMALLOC_LIBRARY} )
  endif()
endmacro(add_photk_executable name)

macro(add_photk_tool name)
  add_photk_executable(${name} ${ARGN})
  if( PHOTK_BUILD_TOOLS )
    message( "Installing: " ${name} )
    install(TARGETS ${name} RUNTIME DESTINATION bin)
  endif()
  set_target_properties(${name} PROPERTIES FOLDER "tools")
endmacro(add_photk_tool name)

function(photk_enable_testing)
  set(GTEST_DIR ${CMAKE_SOURCE_DIR}/thirdparty/gtest)
  include_directories(${VISIONWORKBENCH_INCLUDE_DIRS})
  include_directories(${GTEST_DIR}/include)
  add_library(gtest SHARED EXCLUDE_FROM_ALL
    ${GTEST_DIR}/src/gtest-all.cc
    ${CMAKE_SOURCE_DIR}/src/test/test_main.cc
    )
  target_link_libraries(gtest
    ${VISIONWORKBENCH_CORE_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    )
  set_target_properties(gtest
    PROPERTIES
    COMPILE_FLAGS "-DTEST_SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}\\\" -DTEST_DSTDIR=\\\"${CMAKE_CURRENT_BINARY_DIR}\\\""
    )

  add_custom_target(check)
  add_dependencies(check gtest)
endfunction(photk_enable_testing)


function(photk_add_test source_file)
  string(REGEX REPLACE "^([A-Za-z0-9_]*)\\.([A-Za-z0-9]*)" "\\1" executable "${source_file}")

  add_executable(${executable} EXCLUDE_FROM_ALL ${source_file} )
  target_link_libraries( ${executable} gtest )
  foreach(lib ${PHOTK_USED_LIBS})
    target_link_libraries( ${executable} ${lib} )
  endforeach(lib)

  set_target_properties(${executable}
    PROPERTIES
    COMPILE_FLAGS "-DTEST_SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}\\\" -DTEST_DSTDIR=\\\"${CMAKE_CURRENT_BINARY_DIR}\\\""
    )
  add_custom_target(${executable}Exec ${CMAKE_CURRENT_BINARY_DIR}/${executable}
    DEPENDS ${executable}
    )
  add_dependencies(check ${executable}Exec)

  # file(READ "${source_file}" contents)
  # string(REGEX MATCHALL "TEST_?F?\\(([A-Za-z_0-9 ,]+)\\)" found_tests ${contents})
  # foreach(hit ${found_tests})
  #   string(REGEX REPLACE ".*\\( *([A-Za-z_0-9]+), *([A-Za-z_0-9]+) *\\).*" "\\1.\\2" test_name ${hit})
  #   message("Added test name: " ${executable} "\t" ${test_name})
  #   add_test(${test_name} ${executable} --gtest_filter=${test_name})
  # endforeach()
endfunction(photk_add_test)