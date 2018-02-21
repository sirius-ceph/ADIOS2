/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloCephWriter.cpp: Simple self-descriptive example of how to write a variable
 * to Ceph tiered storage. Based upong helloBPWriter.cpp
 *
 *  Created on: 
 *      Author: 
 */

#include <ios>      //std::ios_base::failure
#include <iostream> //std::cout
#include <mpi.h>
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include "HelloCephArgs.h"
#include <adios2.h>
#include "adios2/ADIOSTypes.h"

// single process write.
// sirius@jdev:/src/adios2/build$ ./bin/hello_cephWriter ../source/examples/hello/cephWriter/hello_ceph.xml 1 1 4 4 5 500
 
// multiple process write.  (n=2 in /etc/proc, and hence args N*M are 1*2 = 2)
// sirius@jdev:/src/adios2/build/bin$ mpirun hello_cephWriter ../../source/examples/hello/cephWriter/hello_ceph.xml 1 2 2 2 5 500


int main(int argc, char *argv[])
{
    int rank = 0, nproc = 1;
    int wrank = 0, wnproc = 1;
    MPI_Comm mpiWriterComm;
    std::string expName = "myExperimentName";

#ifdef ADIOS2_HAVE_MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wnproc);

    const unsigned int color = 1;
    MPI_Comm_split(MPI_COMM_WORLD, color, wrank, &mpiWriterComm);

    MPI_Comm_rank(mpiWriterComm, &rank);
    MPI_Comm_size(mpiWriterComm, &nproc);
#endif

    /** Application data */
    std::vector<float> tempVals = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<int> pressureVals = {0, -1, -2, -3, -4, -5, -6, -7, -8, -9};
    const std::string label("Hello Variable String from rank " +
            std::to_string(rank)); 
    const std::size_t Nx = tempVals.size();
    std::cout << "helloCephWriter" << std::endl;

    try
    {
        // user input.
        HelloCephArgs settings(true, argc, argv, rank, nproc);
        
        // dataset
        std::vector<float> myArray(settings.ndx * settings.ndy);
                
        /** ADIOS class factory of IO class objects, DebugON is recommended */
        adios2::ADIOS adios(settings.configfile, mpiWriterComm, adios2::DebugON);

        // name is in the xml config file, other associated params set as well.
        adios2::IO &cephIO = adios.DeclareIO("writer");   
        
        // valid engine name from core/IO.cpp, engine will do the writes.
        //~ cephIO.SetEngine("CephXX");
        
        // Extra IO params, can also pull from local xml config file
        cephIO.SetParameter("ExpName", expName);  // overwrites xml config val.
        
        adios2::Variable<float> &varArray = cephIO.DefineVariable<float>(
                "myArray", {settings.gndx, settings.gndy},
                {settings.offsx, settings.offsy}, {settings.ndx, settings.ndy},
                adios2::ConstantDims);
            
        adios2::Params ioParams = cephIO.GetParameters();
        for (std::map<std::string,std::string>::iterator it=ioParams.begin(); it!=ioParams.end(); ++it)
            std::cout << "helloCephWriter:IO Eng Params: " << it->first << " => " << it->second << '\n';
            
        std::vector<adios2::Params> v = cephIO.m_TransportsParameters;
        int count = 0;
        for (std::vector<adios2::Params>::iterator it = v.begin(); it != v.end(); ++it)
        {
            adios2::Params p = *it;
            for (std::map<std::string,std::string>::iterator i=p.begin(); i!=p.end(); ++i)
            { 
                std::cout << "helloCephWriter:Transp Params(" << count++ << "):" << i->first << " => " << i->second << '\n';
            }
        }

        // Define variable and local size
        /** global array : name, { shape (total) }, { start (local) }, {
         * count
         * (local) }, all are constant dimensions */
        adios2::Variable<float> &TemperatureVar = cephIO.DefineVariable<float>(
            "myTempsVar", {nproc * Nx}, {rank * Nx}, {Nx}, adios2::ConstantDims);

        adios2::Variable<int> &PressureVar = cephIO.DefineVariable<int>(
            "myPressuresVar", {nproc * Nx}, {rank * Nx}, {Nx}, adios2::ConstantDims);

        adios2::Variable<std::string> &LabelVar =
            cephIO.DefineVariable<std::string>("myLabelVar");
        
        /** Engine derived class, spawned to start IO operations */
        adios2::Engine &cephWriter = cephIO.Open(expName, adios2::Mode::Write);

        for (int step = 0; step < settings.steps; ++step)
        {
            int idx = 0;
            for (int j = 0; j < settings.ndy; ++j)
            {
                for (int i = 0; i < settings.ndx; ++i)
                {
                    myArray[idx] = rank + (step / 100.0f);
                    ++idx;
                }
            }
            cephWriter.BeginStep(adios2::StepMode::Append);
            //cephWriter.PutDeferred<float>(varArray, myArray.data());
            cephWriter.PutSync<float>(TemperatureVar, tempVals.data());
            cephWriter.PutSync<int>(PressureVar, pressureVals.data());
            cephWriter.PutSync<std::string>(LabelVar, label);    
            cephWriter.EndStep();
            std::this_thread::sleep_for(
                std::chrono::milliseconds(settings.sleeptime));
        }
            
            
        /** Put variables for buffering, template type is optional */
       // cephWriter.PutSync<float>(floatVars, myFloats.data());
       // cephWriter.PutSync<int>(intVars, myInts.data());
       // cephWriter.PutSync<std::string>(stringVar, myString);    
        
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
