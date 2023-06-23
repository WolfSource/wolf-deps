include(CTest)

if (WDEPS_MSQUIC)
    add_executable(msquic-test msquic.cpp)
    target_link_libraries(msquic-test PRIVATE msquic::msquic)
    add_test(NAME msquic-test COMMAND msquic-test)
endif()
