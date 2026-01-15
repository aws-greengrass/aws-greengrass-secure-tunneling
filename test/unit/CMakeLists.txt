# Unit tests

# Generate mocks
set(MOCK_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/mocks)
file(MAKE_DIRECTORY ${MOCK_OUTPUT_DIR})

# Mock generation function
function(generate_mock header_file)
  get_filename_component(header_name ${header_file} NAME_WE)
  get_filename_component(header_abs ${header_file} ABSOLUTE)
  set(mock_file ${MOCK_OUTPUT_DIR}/Mock${header_name}.c)
  set(mock_header ${MOCK_OUTPUT_DIR}/Mock${header_name}.h)

  add_custom_command(
    OUTPUT ${mock_file} ${mock_header}
    COMMAND ruby ${cmock_SOURCE_DIR}/lib/cmock.rb
            -o${CMAKE_CURRENT_SOURCE_DIR}/cmock_config.yml ${header_abs}
    DEPENDS ${header_abs} ${CMAKE_CURRENT_SOURCE_DIR}/cmock_config.yml
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating mock for ${header_name}")

  set(MOCK_${header_name}_SOURCE
      ${mock_file}
      PARENT_SCOPE)
  set(MOCK_${header_name}_HEADER
      ${mock_header}
      PARENT_SCOPE)
endfunction()

# Generate mocks for headers
generate_mock(${CMAKE_SOURCE_DIR}/src/subscriptions.h)

# Mark mock files as generated
set_source_files_properties(
  ${MOCK_subscriptions_SOURCE} ${MOCK_subscriptions_HEADER} PROPERTIES GENERATED
                                                                       TRUE)

# Create a target that depends on all mocks
add_custom_target(
  generate_all_mocks
  DEPENDS ${MOCK_subscriptions_SOURCE} ${MOCK_subscriptions_HEADER}
  COMMENT "Generating all mocks")

# Test: secure_tunnel
add_executable(
  test_secure_tunnel ${CMAKE_SOURCE_DIR}/src/secure-tunnel.c
                     ${MOCK_subscriptions_SOURCE} test_secure_tunnel.c)
add_dependencies(test_secure_tunnel generate_all_mocks)
target_include_directories(
  test_secure_tunnel
  PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src
          ${MOCK_OUTPUT_DIR} ${cmock_SOURCE_DIR}/src ${unity_SOURCE_DIR}/src)
target_compile_definitions(test_secure_tunnel
                           PRIVATE "GG_MODULE=(\"test_unit\")")
target_link_libraries(test_secure_tunnel PRIVATE cmock unity test_helpers
                                                 gg-sdk)
add_test(NAME test_secure_tunnel COMMAND test_secure_tunnel)

# Test: localproxy failure cleanup
add_executable(
  test_localproxy_failure ${CMAKE_SOURCE_DIR}/src/tunnel_notification_parser.c
                          test_localproxy_failure.c)
target_include_directories(
  test_localproxy_failure PRIVATE ${CMAKE_SOURCE_DIR}/include
                                  ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(test_localproxy_failure
                           PRIVATE "GG_MODULE=(\"test_localproxy_failure\")")
target_link_libraries(test_localproxy_failure PRIVATE unity test_helpers gg-sdk)
add_test(NAME test_localproxy_failure COMMAND test_localproxy_failure)

# Test: service name validation
add_executable(
  test_service_name_validation
  ${CMAKE_SOURCE_DIR}/src/tunnel_notification_parser.c
  test_service_name_validation.c)
target_include_directories(
  test_service_name_validation PRIVATE ${CMAKE_SOURCE_DIR}/include
                                       ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(test_service_name_validation
                           PRIVATE "GG_MODULE=(\"test_service_validation\")")
target_link_libraries(test_service_name_validation PRIVATE unity test_helpers
                                                           gg-sdk)
add_test(NAME test_service_name_validation COMMAND test_service_name_validation)
