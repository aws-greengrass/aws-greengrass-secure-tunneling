# Integration tests

# Test: error handling
add_executable(
  test_integration_error_handling
  ${CMAKE_SOURCE_DIR}/src/tunnel_notification_parser.c
  test_integration_error_handling.c)
target_include_directories(
  test_integration_error_handling PRIVATE ${CMAKE_SOURCE_DIR}/include
                                          ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(test_integration_error_handling
                           PRIVATE "GG_MODULE=(\"test_integration\")")
target_link_libraries(test_integration_error_handling
                      PRIVATE unity test_helpers gg-sdk)
add_test(NAME test_integration_error_handling
         COMMAND test_integration_error_handling)

# Test: subscription
add_executable(
  test_subscription
  ${CMAKE_SOURCE_DIR}/src/subscription.c ${CMAKE_SOURCE_DIR}/src/tunnel.c
  ${CMAKE_SOURCE_DIR}/src/tunnel_notification_parser.c test_subscription.c)
target_include_directories(test_subscription PRIVATE ${CMAKE_SOURCE_DIR}/include
                                                     ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(test_subscription
                           PRIVATE "GG_MODULE=(\"test_subscription\")")
target_link_libraries(test_subscription PRIVATE unity gg-test gg-ipc-mock
                                                gg-sdk)
add_test(NAME test_subscription COMMAND test_subscription)
