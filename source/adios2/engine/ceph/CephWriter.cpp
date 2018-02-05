/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CephWriter.cpp
 *
 *  Created on: 
 *      Author: 
 */

#include "CephWriter.h"
#include "CephWriter.tcc"

#include <iostream>

#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/core/IO.h"
#include "adios2/helper/adiosFunctions.h" //CheckIndexRange
#include "adios2/toolkit/transport/file/FileFStream.h"
#include "adios2/toolkit/transport/ceph/CephObjTrans.h"



namespace adios2
{

CephWriter::CephWriter(IO &io, const std::string &name, const Mode mode,
                           MPI_Comm mpiComm)
: Engine("CephWriter", io, name, mode, mpiComm)
{
    m_EndMessage = " in call to IO Open CephWriter " + m_Name + "\n";
    const std::string msg = "CephWriter() Engine constructor.  m_Name=" + m_Name;
    
    MPI_Comm_rank(mpiComm, &m_WriterRank);
    Init();
    if (m_Verbosity == 5)
    {
        std::cout << msg << ".  m_WriterRank=" << m_WriterRank << std::endl;
    }
    
}

CephWriter::~CephWriter() = default;

StepStatus CephWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    const std::string msg = " in call to CephWriter::BeginStep(StepMode mode, const float timeoutSeconds) \n";   
    return StepStatus::OK;
}

void CephWriter::PerformPuts()
{
    const std::string msg = " in call to CephWriter::PerformPuts() \n";   
    //m_BP3Serializer.ResizeBuffer(m_BP3Serializer.m_DeferredVariablesDataSize, "in call to PerformPuts");

    //~ for (const auto &variableName : m_BP3Serializer.m_DeferredVariables)
    //~ {
        //~ PutSync(variableName);
    //~ }

    //m_BP3Serializer.m_DeferredVariables.clear();
}

void CephWriter::EndStep()
{
    const std::string msg = " in call to CephWriter::EndStep() \n";
    //~ if (m_BP3Serializer.m_DeferredVariables.size() > 0)
    //~ {
        //~ PerformPuts();
    //~ }

    //~ m_BP3Serializer.SerializeData(m_IO, true); // true: advances step

    //~ const size_t currentStep = m_BP3Serializer.m_MetadataSet.TimeStep - 1;
    //~ const size_t flushStepsCount = m_BP3Serializer.m_FlushStepsCount;

    //~ if (currentStep % flushStepsCount)
    //~ {
        //~ m_BP3Serializer.SerializeData(m_IO);
        //~ m_FileDataManager.WriteFiles(m_BP3Serializer.m_Data.m_Buffer.data(),
                                     //~ m_BP3Serializer.m_Data.m_Position);
        //~ m_BP3Serializer.ResetBuffer(m_BP3Serializer.m_Data);
        //~ WriteCollectiveMetadataFile();
    //~ }
}

void CephWriter::DoClose(const int transportIndex)
{
    const std::string msg = " in call to CephWriter::Close(const int transportIndex) \n";
    //~ if (m_BP3Serializer.m_DeferredVariables.size() > 0)
    //~ {
        //~ PerformPuts();
    //~ }

    //~ // close bp buffer by serializing data and metadata
    //~ m_BP3Serializer.CloseData(m_IO);
    //~ // send data to corresponding transports
    //~ m_FileDataManager.WriteFiles(m_BP3Serializer.m_Data.m_Buffer.data(),
                                 //~ m_BP3Serializer.m_Data.m_Position,
                                 //~ transportIndex);

    //~ m_FileDataManager.CloseFiles(transportIndex);

    //~ if (m_BP3Serializer.m_Profiler.IsActive &&
        //~ m_FileDataManager.AllTransportsClosed())
    //~ {
        //~ WriteProfilingJSONFile();
    //~ }

    //~ if (m_BP3Serializer.m_CollectiveMetadata &&
        //~ m_FileDataManager.AllTransportsClosed())
    //~ {
        //~ WriteCollectiveMetadataFile();
    //~ }
}

// PRIVATE FUNCTIONS
// PRIVATE
void CephWriter::Init()
{
    const std::string msg = " in call to CephWriter::Init() \n";
    InitParameters();
    InitTransports();
    InitBuffer();
}

#define declare_type(T)                                                        \
    void CephWriter::DoPutSync(Variable<T> &variable, const T *values)       \
    {                                                                          \
        PutSyncCommon(variable, values);                                       \
    }                                                                          \
    void CephWriter::DoPutDeferred(Variable<T> &variable, const T *values)   \
    {                                                                          \
        PutDeferredCommon(variable, values);                                   \
    }                                                                          \
    void CephWriter::DoPutDeferred(Variable<T> &, const T &value) {}
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void CephWriter::InitParameters()
{
    const std::string msg = " in call to CephWriter::InitParameters() \n";
    //m_BP3Serializer.InitParameters(m_IO.m_Parameters);
}

void CephWriter::InitTransports()
{
    
    const std::string msg = " in call to CephWriter::InitTransports() \n";

    // TODO need to add support for aggregators here later
    if (m_IO.m_TransportsParameters.empty())
    {
        Params defaultTransportParameters;
        defaultTransportParameters["transport"] = "CephObjTrans";
        m_IO.m_TransportsParameters.push_back(defaultTransportParameters);
    }
    
    auto itParams = m_IO.m_Parameters.find("verbose");
    if (itParams != m_IO.m_Parameters.end())
    {
        m_Verbosity = std::stoi(itParams->second);
        if (m_DebugMode)
        {
            if (m_Verbosity < 0 || m_Verbosity > 5)
                throw std::invalid_argument(
                    "ERROR: Method verbose argument must be an "
                    "integer in the range [0,5], in call to "
                    "Open or Engine constructor\n");
        }
    }


}

void CephWriter::InitBuffer()
{
    const std::string msg = "CephWriter::InitBuffer() \n";
    
    if (m_Verbosity == 5)
    {
        //std::cout << "Skeleton Writer " << m_WriterRank << "   EndStep()\n";
        std::cout << msg << std::endl;
    }
    

}



} // end namespace adios2






