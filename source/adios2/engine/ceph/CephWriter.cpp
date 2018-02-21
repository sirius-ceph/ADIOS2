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
    
    MPI_Comm_rank(mpiComm, &m_WriterRank);
    Init();
#ifdef USE_CEPH_OBJ_TRANS
    InitTransports(mpiComm);
#endif /* USE_CEPH_OBJ_TRANS */
        
    if (m_DebugMode)
    {
        std::cout << "CephWriter::constructor:rank("  << m_WriterRank 
            << ") m_Name=" << m_Name << std::endl;
    }
    
}

CephWriter::~CephWriter() = default;

StepStatus CephWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    if (m_CurrentStep < 0) m_CurrentStep++; // 0 is the first step
    if (m_TimestepStart < 0) m_TimestepStart = m_CurrentStep;
    if (m_DebugMode)
    {
        std::cout << "CephWriter::BeginStep:rank("  << m_WriterRank 
                << ") m_CurrentStep++=" << m_CurrentStep << std::endl;
    }
    return StepStatus::OK;
}

void CephWriter::PerformPuts()
{
    if (m_DebugMode)
    {
        std::cout << "CephWriter::PerformPuts:rank("  << m_WriterRank 
                << ") " << std::endl;
    }
    m_NeedPerformPuts = false;
    
    // BPFileWriter:
    //~ for (const auto &variableName : m_BP3Serializer.m_DeferredVariables)
    //~ {
        //~ PutSync(variableName);
    //~ }

}

void CephWriter::EndStep()
{
    // CephWriter call to PutSyncCommon() here
    int prev = m_CurrentStep;
    m_CurrentStep++;  // advances timesteps
    
    if (m_DebugMode)
    {
        std::cout << "CephWriter::EndStep:rank("  << m_WriterRank 
                << ") prevStep:" << prev << "; advanced to:" << m_CurrentStep 
                << std::endl;
    }
    
    if (m_NeedPerformPuts)
    {
        PerformPuts();
    }

}

size_t CephWriter::CurrentStep() 
{
    // only advanced during EndStep()
    return m_CurrentStep;
}

void CephWriter::DoClose(const int transportIndex)
{
    // forces a write, (DoClose and flush both force a write)
    if (m_DebugMode)
    {
        std::cout << "CephWriter::DoClose:rank("  << m_WriterRank 
                << "). transportIndex=" << transportIndex << std::endl;
    }
    
    // if there are deferred vars: puts then write and close

    #ifdef USE_CEPH_OBJ_TRANS
    transport->Close();
#endif /* USE_CEPH_OBJ_TRANS */

}


// PRIVATE
void CephWriter::Init()
{
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
    auto it = m_IO.m_Parameters.find("verbose");
    if (it == m_IO.m_Parameters.end())
        it = m_IO.m_Parameters.find("Verbose");
    if (it != m_IO.m_Parameters.end())
    {
        m_Verbosity = std::stoi(it->second);
        if (m_DebugMode)
        {
            if (m_Verbosity < 0 || m_Verbosity > 5)
            throw std::invalid_argument(
                        "ERROR: Method verbose argument must be an "
                        "integer in the range [0,5], in call to "
                        "Open or Engine constructor\n");
        }
    }
    
    it = m_IO.m_Parameters.find("ExpName");
    if (it == m_IO.m_Parameters.end())
        it = m_IO.m_Parameters.find("expname");
    if (it != m_IO.m_Parameters.end())
    {
        Params p = {{it->first, it->second}};
        m_IO.m_TransportsParameters.push_back(p);
    }
    
    it = m_IO.m_Parameters.find("JobId");
    if (it == m_IO.m_Parameters.end())
        it = m_IO.m_Parameters.find("jobid");
    if (it != m_IO.m_Parameters.end())
    {
        Params p = {{it->first, it->second}};
        m_IO.m_TransportsParameters.push_back(p);
    }
    
    it = m_IO.m_Parameters.find("TargetObjSize");
    if (it == m_IO.m_Parameters.end())
        it = m_IO.m_Parameters.find("targetobjsize");
    if (it != m_IO.m_Parameters.end())
    {
        Params p = {{it->first, it->second}};
        m_IO.m_TransportsParameters.push_back(p);
    }
    
    if (m_DebugMode)
    {
        std::cout << "CephWriter::InitParameters:rank("  << m_WriterRank 
                << ")" << std::endl;
    }
}

 
void CephWriter::InitTransports(MPI_Comm mpiComm)
{
    if (m_DebugMode)
    {
        std::cout << "CephWriter::InitTransports:rank("  << m_WriterRank 
                << ")" << std::endl;
    }
    
    transport = std::shared_ptr<transport::CephObjTrans>(
        new transport::CephObjTrans(
                mpiComm, m_IO.m_TransportsParameters, true));
    const std::string name = "fname-rank-" + std::to_string(m_WriterRank);
    const Mode mode = Mode::Write;
    transport->Open(name, mode);
}
  
void CephWriter::InitBuffer()
{
    if (m_DebugMode)
    {
        std::cout << "CephWriter::InitBuffer:rank("  << m_WriterRank 
                << ")" << std::endl;
    }
    
    //m_bl = new librados::bufferlist(m_CephTargetObjSize*2); 
#ifdef USE_CEPH_OBJ_TRANS
    m_bl = new librados::bufferlist(m_CephTargetObjSize);  // ?good size?
    m_bl->clear();
    m_bl->zero();
#endif /* USE_CEPH_OBJ_TRANS */
}

// Generator for unique oids in the experimental space.
std::string CephWriter::Objector(std::string prefix, std::string varInfo, 
    int rank, int timestepStart, int timestepEnd) 
{
    if (m_DebugMode)
    {
        std::cout << "CephWriter::Objector:rank("  << m_WriterRank 
                << ")" << std::endl;
    }
    // TODO:  Implement per Margaret's prototype.
    std::string oid = (
            prefix + 
            varInfo + 
            std::to_string(rank) + 
            std::to_string(timestepStart) + 
            std::to_string(timestepEnd)
    );
    
    return oid;    
}


} // end namespace adios2
