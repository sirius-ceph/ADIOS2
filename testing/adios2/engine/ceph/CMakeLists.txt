add_executable(TestCephWriteReadADIOS2 TestCephWriteReadADIOS2.cpp) 
target_link_libraries(TestCephWriteReadADIOS2 adios2 gtest gtest_main) 

if(ADIOS2_HAVE_MPI)
    target_link_libraries(TestCephWriteReadADIOS2 MPI::MPI_C) 
    set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})
endif()

gtest_add_tests(TARGET TestCephWriteReadADIOS2 TEST_SUFFIX ".ceph" ${extra_test_args}) 
