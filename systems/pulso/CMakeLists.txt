add_executable(synth-pulso src/main.cpp)
target_link_libraries(synth-pulso PRIVATE synth-contracts httplib::httplib Threads::Threads)
target_compile_options(synth-pulso PRIVATE -Wall -Wextra -Wpedantic)
