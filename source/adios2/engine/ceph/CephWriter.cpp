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
#include "adios2/toolkit/transport/ceph/CephObjTrans.h"


namespace adios2
{

CephWriter::CephWriter(IO &io, const std::string &name, const Mode mode,
                           MPI_Comm mpiComm)
: Engine("CephWriter", io, name, mode, mpiComm)
{
    m_EndMessage = " in call to IO Open CephWriter " + m_Name + "\n";

    std::cout << "CephWriter::CephWriter() Engine constructor begin " 
            << "(Called from IO.Open)." << " m_Name=" << m_Name 
            << ".  m_WriterRank=" << m_WriterRank << std::endl;
    
    MPI_Comm_rank(mpiComm, &m_WriterRank);
    Init();
#ifdef USE_CEPH_OBJ_TRANS
    InitTransports2(mpiComm);
#endif /* USE_CEPH_OBJ_TRANS */
    
    if (m_Verbosity == 5)
    {
        std::cout << "CephWriter::CephWriter() Engine constructor end (Called from IO.Open)." 
            << " m_Name=" << m_Name << ".  m_WriterRank=" 
            << m_WriterRank << std::endl;
    }
    
}

CephWriter::~CephWriter() = default;

StepStatus CephWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    m_CurrentStep++; // 0 is the first step
    if (m_TimestepStart < 0) m_TimestepStart = m_CurrentStep;
    if (m_Verbosity == 5)
      {
        std::cout << "CephWriter " << m_WriterRank
                  << "   BeginStep() new step " << m_CurrentStep << "\n";
      }
    return StepStatus::OK;
}

void CephWriter::PerformPuts()
{
    if (m_Verbosity == 5)
    {
        std::cout << "CephWriter " << m_WriterRank
                  << "     PerformPuts()\n";
    }
    m_NeedPerformPuts = false;
    
    // BPFileWriter:
    //m_BP3Serializer.ResizeBuffer(m_BP3Serializer.m_DeferredVariablesDataSize, "in call to PerformPuts");

    //~ for (const auto &variableName : m_BP3Serializer.m_DeferredVariables)
    //~ {
        //~ PutSync(variableName);
    //~ }

    //m_BP3Serializer.m_DeferredVariables.clear();
}

void CephWriter::EndStep()
{
    // CephWriter call to PutSyncCommon() here
    
    if (m_NeedPerformPuts)
    {
        PerformPuts();
    }
    if (m_Verbosity == 5)
    {
        std::cout << "CephWriter " << m_WriterRank << "   EndStep()\n";
    }

    // BPFileWriter:
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
    // forces a write, (DoClose and flush both force a write)
    if (m_Verbosity == 5)
    {
        std::cout << "CephWriter " << m_WriterRank << " DoClose(" << m_Name
                  << ")\n";
    }
    
    // if there are deferred vars: puts then write and close
    
    // BPFileWriter:
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

#ifdef USE_CEPH_OBJ_TRANS
    transport->Close();
#endif /* USE_CEPH_OBJ_TRANS */

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


// PRIVATE
void CephWriter::Init()
{
    InitParameters();
#ifndef USE_CEPH_OBJ_TRANS
    InitTransports();
#endif /* USE_CEPH_OBJ_TRANS */
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
    auto itParams = m_IO.m_Parameters.find("verbose");
    if (itParams == m_IO.m_Parameters.end())
    {
        itParams = m_IO.m_Parameters.find("Verbose");
    }

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
    itParams = m_IO.m_Parameters.find("TargetObjSize");
    if (itParams != m_IO.m_Parameters.end())
    {
        m_TargetObjSize = std::stoi(itParams->second);
        if (m_Verbosity == 5)
        {
            std::cout << "CephWriter set TargetObjSize=" << m_TargetObjSize << std::endl;
        }
    }
    itParams = m_IO.m_Parameters.find("UniqueExperimentName");
    if (itParams != m_IO.m_Parameters.end())
    {
        m_UniqueExperimentName = itParams->second;
        if (m_Verbosity == 5)
        {
            std::cout << "CephWriter set UniqueExperimentName=" << m_UniqueExperimentName << std::endl;
        }
    }
    
    if (m_Verbosity == 5)
    {
        std::cout << "CephWriter " << m_WriterRank << " InitParameters(" << m_Name
            << ") done.\n";
    }
}

void CephWriter::InitTransports()
{
    
    if (m_Verbosity == 5)
    {
        std::cout << "CephWriter " << m_WriterRank << " InitTransports("
        << m_Name << ")\n";
    }

    // TODO need to add support for aggregators here later
    if (m_IO.m_TransportsParameters.empty())
    {
        Params defaultTransportParameters;
        defaultTransportParameters["transport"] = "CephObjTrans";
        m_IO.m_TransportsParameters.push_back(defaultTransportParameters);
    }
}
 
void CephWriter::InitTransports2(MPI_Comm mpiComm)
{
  if (m_Verbosity == 5)
    {
      std::cout << "CephWriter " << m_WriterRank << " InitTransports("
            << m_Name << ")\n";
    }

  // TODO need to add support for aggregators here later
  if (m_IO.m_TransportsParameters.empty())
    {
      Params defaultTransportParameters;
      defaultTransportParameters["transport"] = "CephObjTrans";
      m_IO.m_TransportsParameters.push_back(defaultTransportParameters);
    }
  transport = std::shared_ptr<transport::CephObjTrans>(new transport::CephObjTrans(mpiComm, true));
  //transport->Open(const std::string &name, const Mode openMode);
}
  
void CephWriter::InitBuffer()
{
    m_bl = new librados::bufferlist(m_TargetObjSize);
    m_bl->clear();
    m_bl->zero();
    
  
    if (m_Verbosity == 5)
    {
        std::cout << "CephWriter " << m_WriterRank << " InitBuffer(" 
            << m_Name << ").  m_bl->length()=" << m_bl->length()
            << ". m_bl->is_zero()=" << m_bl->is_zero() << ". m_bl->length()=" 
            << m_bl->length() << "\n";
    }    
}

// Generator for unique oids in the experimental space.
std::string CephWriter::Objector(
        std::string prefix, 
        std::string varInfo, 
        int rank, 
        int timestepStart, 
        int timestepEnd) 
{
    // TODO:  Implement per Margaret's prototype.
    std::string oid = (
            prefix + 
            varInfo + 
            std::to_string(rank) + 
            std::to_string(timestepStart) + 
            std::to_string(timestepEnd)
    );
    
    if (m_Verbosity == 5)
    {
        std::cout << "CephWriter " << m_WriterRank << "     Objector("
                << "prefix=" << prefix << " varInfo=" << varInfo 
                << "rank=" << rank << "timestepStart=" << timestepStart 
                << "timestepEnd=" << timestepEnd << "oid=" << oid << ")\n";
    }

    return oid;    
}


} // end namespace adios2
