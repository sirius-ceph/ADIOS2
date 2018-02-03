/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <cstring>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "../SmallTestData.h"


// see: ../source/cmake/upstream/GoogleTest.cmake
class CephWriteReadTestADIOS2 : public ::testing::Test
{
public:
    CephWriteReadTestADIOS2() = default;

    SmallTestData m_TestData;
};

//******************************************************************************
// 1D 1x8 test data
//******************************************************************************

// ADIOS2 BP write, native ADIOS1 read
TEST_F(CephWriteReadTestADIOS2, ADIOS2CephWriteRead1D8)
{
    // Each process would write a 1x8 array and all processes would
    // form a mpiSize * Nx 1D array
    const std::string fname("ADIOS2CephWriteRead1D8.obj");

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const size_t Nx = 8;

    // Number of steps
    const size_t NSteps = 3;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

// Write test data using BP

#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(true);
#endif
    {
        adios2::IO &io = adios.DeclareIO("TestIO");

        // Declare 1D variables (NumOfProcesses * Nx)
        // The local process' part (start, count) can be defined now or later
        // before Write().
        {
            const adios2::Dims shape{static_cast<size_t>(Nx * mpiSize)};
            const adios2::Dims start{static_cast<size_t>(Nx * mpiRank)};
            const adios2::Dims count{Nx};

            auto &var_iString = io.DefineVariable<std::string>("iString");
            auto &var_i8 = io.DefineVariable<int8_t>("i8", shape, start, count);
            auto &var_i16 =
                io.DefineVariable<int16_t>("i16", shape, start, count);
            auto &var_i32 =
                io.DefineVariable<int32_t>("i32", shape, start, count);
            auto &var_i64 =
                io.DefineVariable<int64_t>("i64", shape, start, count);
            auto &var_u8 =
                io.DefineVariable<uint8_t>("u8", shape, start, count);
            auto &var_u16 =
                io.DefineVariable<uint16_t>("u16", shape, start, count);
            auto &var_u32 =
                io.DefineVariable<uint32_t>("u32", shape, start, count);
            auto &var_u64 =
                io.DefineVariable<uint64_t>("u64", shape, start, count);
            auto &var_r32 =
                io.DefineVariable<float>("r32", shape, start, count);
            auto &var_r64 =
                io.DefineVariable<double>("r64", shape, start, count);
        }

        // from: engine/bp/TestBPWriteRead.cpp: TEST_F(BPWriteReadTest, ADIOS2BPWriteADIOS1Read1D8)
        // Create the BP Engine
        io.SetEngine("Ceph");
        io.AddTransport("CephObj");

        // QUESTION: It seems that BPFilterWriter cannot overwrite existing
        // files
        // Ex. if you tune Nx and NSteps, the test would fail. But if you clear
        // the cache in
        // ${adios2Build}/testing/adios2/engine/bp/ADIOS2BPWriteADIOS1Read1D8.bp.dir,
        // then it works
        adios2::Engine &CephWriter = io.Open(fname, adios2::Mode::Write);
        // CREATES this DIR:
        // /src/adios2/build/testing/adios2/engine/ceph/ADIOS2CephWriteRead1D8.obj.bp.dir

        for (size_t step = 0; step < NSteps; ++step)
        {
            // Generate test data for each process uniquely
            SmallTestData currentTestData = generateNewSmallTestData( 
                m_TestData, static_cast<int>(step), mpiRank, mpiSize);

            // Retrieve the variables that previously went out of scope
            auto &var_iString = *io.InquireVariable<std::string>("iString");
            auto &var_i8 = *io.InquireVariable<int8_t>("i8");
            auto &var_i16 = *io.InquireVariable<int16_t>("i16");
            auto &var_i32 = *io.InquireVariable<int32_t>("i32");
            auto &var_i64 = *io.InquireVariable<int64_t>("i64");
            auto &var_u8 = *io.InquireVariable<uint8_t>("u8");
            auto &var_u16 = *io.InquireVariable<uint16_t>("u16");
            auto &var_u32 = *io.InquireVariable<uint32_t>("u32");
            auto &var_u64 = *io.InquireVariable<uint64_t>("u64");
            auto &var_r32 = *io.InquireVariable<float>("r32");
            auto &var_r64 = *io.InquireVariable<double>("r64");

            // Make a 1D selection to describe the local dimensions of the
            // variable we write and its offsets in the global spaces
            adios2::Box<adios2::Dims> sel({mpiRank * Nx}, {Nx});

            EXPECT_THROW(var_iString.SetSelection(sel), std::invalid_argument);
            var_i8.SetSelection(sel);
            var_i16.SetSelection(sel);
            var_i32.SetSelection(sel);
            var_i64.SetSelection(sel);
            var_u8.SetSelection(sel);
            var_u16.SetSelection(sel);
            var_u32.SetSelection(sel);
            var_u64.SetSelection(sel);
            var_r32.SetSelection(sel);
            var_r64.SetSelection(sel);

            // Write each one
            // fill in the variable with values from starting index to
            // starting index + count
            CephWriter.BeginStep();

            CephWriter.PutDeferred(var_iString, currentTestData.S1);
            CephWriter.PutDeferred(var_i8, currentTestData.I8.data());
            CephWriter.PutDeferred(var_i16, currentTestData.I16.data());
            CephWriter.PutDeferred(var_i32, currentTestData.I32.data());
            CephWriter.PutDeferred(var_i64, currentTestData.I64.data());
            CephWriter.PutDeferred(var_u8, currentTestData.U8.data());
            CephWriter.PutDeferred(var_u16, currentTestData.U16.data());
            CephWriter.PutDeferred(var_u32, currentTestData.U32.data());
            CephWriter.PutDeferred(var_u64, currentTestData.U64.data());
            CephWriter.PutDeferred(var_r32, currentTestData.R32.data());
            CephWriter.PutDeferred(var_r64, currentTestData.R64.data());
            CephWriter.PerformPuts();

            CephWriter.EndStep();
        }

        // Close the file
        CephWriter.Close();
    }

}


//******************************************************************************
// main
//******************************************************************************

int main(int argc, char **argv)
{
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(nullptr, nullptr);
#endif

    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
