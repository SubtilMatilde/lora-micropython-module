add_library(usermod_lora INTERFACE)

target_sources(usermod_lora INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/lora.c
)

target_include_directories(usermod_lora INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(usermod INTERFACE usermod_lora)
