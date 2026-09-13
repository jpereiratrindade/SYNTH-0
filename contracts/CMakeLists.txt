add_library(synth-contracts INTERFACE)
target_include_directories(synth-contracts INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(synth-contracts INTERFACE nlohmann_json::nlohmann_json)
