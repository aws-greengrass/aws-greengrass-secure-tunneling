# Test configuration for aws-greengrass-secure-tunnel

# Load test dependencies (cmock only)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/fc_deps.json FC_DEPS_JSON)
string(JSON FC_DEPS_COUNT LENGTH "${FC_DEPS_JSON}")
math(EXPR FC_DEPS_INDEX_MAX "${FC_DEPS_COUNT} - 1")

foreach(index RANGE ${FC_DEPS_INDEX_MAX})
  string(JSON dep_name MEMBER "${FC_DEPS_JSON}" ${index})
  string(JSON dep_url GET "${FC_DEPS_JSON}" "${dep_name}" url)
  string(JSON dep_rev GET "${FC_DEPS_JSON}" "${dep_name}" rev)
  fetchcontent_declare(
    "${dep_name}"
    GIT_REPOSITORY "${dep_url}"
    GIT_TAG "${dep_rev}"
    SOURCE_SUBDIR nonexistent_dir)
  fetchcontent_makeavailable(${dep_name})
endforeach()

# CMock framework
add_library(cmock STATIC ${cmock_SOURCE_DIR}/src/cmock.c)
target_include_directories(cmock PUBLIC ${cmock_SOURCE_DIR}/src)
target_link_libraries(cmock PUBLIC unity)

# Test helper library
add_library(test_helpers STATIC test_helpers.c)
target_include_directories(test_helpers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_helpers PUBLIC unity cmock gg-sdk)

# Add subdirectories
add_subdirectory(unit)
add_subdirectory(integration)
