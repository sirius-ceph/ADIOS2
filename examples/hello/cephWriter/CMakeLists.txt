#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(hello_cephWriter helloCephWriter.cpp  HelloCephArgs.cpp)
  
if(ADIOS2_HAVE_MPI)
  target_include_directories(hello_cephWriter PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(hello_cephWriter MPI::MPI_C)  
endif()

target_link_libraries(hello_cephWriter adios2)

