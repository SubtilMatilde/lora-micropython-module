add_library(usermod_lora INTERFACE)

target_sources(usermod_lora INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/modlora.c
    ${CMAKE_CURRENT_LIST_DIR}/lorawan.c
    ${CMAKE_CURRENT_LIST_DIR}/encrypt.c
    ${CMAKE_CURRENT_LIST_DIR}/sx1276radiodriver.c
)

target_include_directories(usermod_lora INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(usermod INTERFACE usermod_lora)
