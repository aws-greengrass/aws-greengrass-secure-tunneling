add_library(gg_test STATIC unity/gg_test/process_wait.c unity/gg_test/test.c)
target_include_directories(gg_test PUBLIC unity/gg_test/include)
target_compile_definitions(gg_test PRIVATE "GG_MODULE=(\"gg_test\")")
target_link_libraries(gg_test PUBLIC unity gg-sdk)

add_library(
  ipc_mock STATIC mock.c object_compare.c packets/connect_packet_sequences.c
                  packets/mqtt_packet_sequences.c packets/packets.c)
target_compile_definitions(ipc_mock PRIVATE "GG_MODULE=(\"ipc_mock\")")
target_link_libraries(ipc_mock PUBLIC gg-sdk gg_test)
target_include_directories(ipc_mock PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                           ${gg_sdk_SOURCE_DIR}/priv_include)
