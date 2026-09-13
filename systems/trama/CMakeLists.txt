add_executable(synth-trama src/main.cpp)
target_link_libraries(synth-trama PRIVATE synth-contracts httplib::httplib Threads::Threads)
target_compile_options(synth-trama PRIVATE -Wall -Wextra -Wpedantic)
