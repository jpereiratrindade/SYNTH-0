add_executable(synth-lente src/main.cpp)
target_link_libraries(synth-lente PRIVATE synth-contracts httplib::httplib Threads::Threads)
target_compile_options(synth-lente PRIVATE -Wall -Wextra -Wpedantic)
