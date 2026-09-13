cmake_minimum_required(VERSION 3.30)
project(SYNTH0 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(SYNTH_BUILD_TESTS "Build SYNTH-0 tests" ON)

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
  GIT_SHALLOW TRUE
)
FetchContent_Declare(
  tomlplusplus
  GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
  GIT_TAG v3.4.0
  GIT_SHALLOW TRUE
)
FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.15.3
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json tomlplusplus httplib)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

add_subdirectory(contracts)
add_subdirectory(systems/pulso)
add_subdirectory(systems/trama)
add_subdirectory(systems/lente)
add_subdirectory(controller)

if(SYNTH_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

set(SYNTH_BIN_DIR "${CMAKE_BINARY_DIR}/bin")
file(MAKE_DIRECTORY "${SYNTH_BIN_DIR}")

set(SYNTH_PULSO_BIN "${CMAKE_BINARY_DIR}/systems/pulso/synth-pulso")
set(SYNTH_TRAMA_BIN "${CMAKE_BINARY_DIR}/systems/trama/synth-trama")
set(SYNTH_LENTE_BIN "${CMAKE_BINARY_DIR}/systems/lente/synth-lente")
set(SYNTH_CONTROLLER_BIN "${CMAKE_BINARY_DIR}/controller/synth-controller")

foreach(unit pulso trama lente controller)
  configure_file(
    "${CMAKE_SOURCE_DIR}/packaging/systemd/synth0-${unit}.service.in"
    "${CMAKE_BINARY_DIR}/packaging/systemd/synth0-${unit}.service"
    @ONLY
  )
endforeach()
configure_file(
  "${CMAKE_SOURCE_DIR}/packaging/systemd/synth0.target.in"
  "${CMAKE_BINARY_DIR}/packaging/systemd/synth0.target"
  @ONLY
)
