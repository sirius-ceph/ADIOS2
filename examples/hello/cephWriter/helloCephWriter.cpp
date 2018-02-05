/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloBPWriter.cpp: Simple self-descriptive example of how to write a variable
 * to a BP File that lives in several MPI processes.
 *
 *  Created on: Feb 16, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include <ios>      //std::ios_base::failure
#include <iostream> //std::cout
#include <mpi.h>
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include "HelloCephArgs.h"
#include <adios2.h>

// sirius@01836545d244:/src/adios2/sirius-ceph/build$ ./bin/hello_cephWriter ../source/examples/hello/cephWriter/hello_ceph.xml 1 1 4 4 5 100


int main(int argc, char *argv[])
{
    int rank = 0, nproc = 1;
    int wrank = 0, wnproc = 1;
    MPI_Comm mpiWriterComm;

#ifdef ADIOS2_HAVE_MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wnproc);

    const unsigned int color = 1;
    MPI_Comm_split(MPI_COMM_WORLD, color, wrank, &mpiWriterComm);

    MPI_Comm_rank(mpiWriterComm, &rank);
    MPI_Comm_size(mpiWriterComm, &nproc);
#endif

    /** Application variable */
    std::vector<float> myFloats = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<int> myInts = {0, -1, -2, -3, -4, -5, -6, -7, -8, -9};
    const std::string myString("Hello Variable String from rank " +
            std::to_string(rank)); 
    const std::size_t Nx = myFloats.size();

    try
    {
        // user input.
        HelloCephArgs settings(true, argc, argv, rank, nproc);
        
        // dataset
        std::vector<float> myArray(settings.ndx * settings.ndy);
        
        adios2::ADIOS adios(settings.configfile, mpiWriterComm, adios2::DebugON);
        
        /** ADIOS class factory of IO class objects, DebugON is recommended */
        /** adios2::ADIOS adios("cfg.xml", MPI_COMM_WORLD, adios2::DebugON); */
        
//~ #ifdef ADIOS2_HAVE_MPI
        //~ adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
//~ #else
        //~ adios2::ADIOS adios(adios2::DebugON);
//~ #endif

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        
        // unique engine name for this adios class scope.        
        //~ adios2::IO &cephIO = adios.DeclareIO("Ceph_N2N");   
        adios2::IO &cephIO = adios.DeclareIO("writer");   
        
        // valid engine name from core/IO.cpp, engine will do the writes.
        //~ cephIO.SetEngine("CephXX");
        
        // IO params, can also pull from local xml config file
        cephIO.SetParameters({
            {"cephMonIP", "127.0.0.1"},
            {"cephUser", "admin"},
            {"objectSizeMB", "8"}, 
            {"MaxBufferSize","2Gb"}, 
            {"BufferGrowthFactor", "1.5" },
            {"verbose", "5"}
        });
        
        std::cout << "here" << std::endl;
            
        // Define variable and local size
        /** global array : name, { shape (total) }, { start (local) }, {
         * count
         * (local) }, all are constant dimensions */
        adios2::Variable<float> &floatVars = cephIO.DefineVariable<float>(
            "floats", {nproc * Nx}, {rank * Nx}, {Nx}, adios2::ConstantDims);

        adios2::Variable<int> &intVars = cephIO.DefineVariable<int>(
            "ints", {nproc * Nx}, {rank * Nx}, {Nx}, adios2::ConstantDims);

        adios2::Variable<std::string> &stringVar =
            cephIO.DefineVariable<std::string>("strings");
        
        /** Engine derived class, spawned to start IO operations */
        adios2::Engine &cephWriter = cephIO.Open("ObjectNameGeneratorFunctionGoesHere", adios2::Mode::Write);
        
        /** Put variables for buffering, template type is optional */
        cephWriter.PutSync<float>(floatVars, myFloats.data());
        cephWriter.PutSync<int>(intVars, myInts.data());
        cephWriter.PutSync<std::string>(stringVar, myString);    
        
        /** Create object, engine becomes unreachable after this*/
        cephWriter.Close();
        
    }
    catch (std::invalid_argument &e)
    {
        std::cout << "Invalid argument exception, STOPPING PROGRAM from rank "
                  << rank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::ios_base::failure &e)
    {
        std::cout << "IO System base failure exception, STOPPING PROGRAM "
                     "from rank "
                  << rank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::exception &e)
    {
        std::cout << "Exception, STOPPING PROGRAM from rank " << rank << "\n";
        std::cout << e.what() << "\n";
    }

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return 0;
}
