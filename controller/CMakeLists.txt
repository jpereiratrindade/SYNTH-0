add_executable(synth-controller src/main.cpp src/controller.cpp)
target_include_directories(synth-controller PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(
  synth-controller
  PRIVATE
    synth-contracts
    httplib::httplib
    tomlplusplus::tomlplusplus
    OpenSSL::Crypto
    Threads::Threads
)
target_compile_definitions(synth-controller PRIVATE SYNTH_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_compile_options(synth-controller PRIVATE -Wall -Wextra -Wpedantic)
