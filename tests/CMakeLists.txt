add_executable(synth-contract-test contract/declaration_contract_test.cpp)
target_link_libraries(synth-contract-test PRIVATE tomlplusplus::tomlplusplus)
target_compile_definitions(synth-contract-test PRIVATE SYNTH_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_test(NAME contract.declarations COMMAND synth-contract-test)

add_test(NAME controller.validate COMMAND synth-controller validate)
set_tests_properties(controller.validate PROPERTIES LABELS "contract")

add_test(NAME resource.declared_budgets COMMAND ${CMAKE_COMMAND} -DSYNTH_SOURCE_DIR=${CMAKE_SOURCE_DIR} -P "${CMAKE_CURRENT_SOURCE_DIR}/resource/declared_budgets.cmake")
set_tests_properties(resource.declared_budgets PROPERTIES LABELS "resource")
