set(SHAREDFORGE_TEST_SOURCES
    test_ringbuffer.cpp
    test_diag.cpp
    test_pragma.cpp
    test_preprocessor.cpp
    test_state.cpp
    test_bus.cpp
    test_adsr.cpp
    test_envelopes.cpp
    test_slot.cpp
    test_params.cpp
    test_param_binding.cpp
    test_snapshot.cpp
    test_presets.cpp
)

foreach(source IN LISTS SHAREDFORGE_TEST_SOURCES)
    get_filename_component(test_name ${source} NAME_WE)
    add_executable(${test_name} ${source})
    target_link_libraries(${test_name} PRIVATE sf_core sf_warnings Catch2::Catch2WithMain)
    target_compile_definitions(${test_name} PRIVATE
        SF_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
