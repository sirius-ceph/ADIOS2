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
    
// Generator for unique oids in the experimental space.  
// This represents Domain specific object naming, where name is part of 
// the metadata regarding the contents (i.e., partitioning) of the dataset
// space. 
//
// static method.
std::string CephWriter::GetOid(std::string jobId, std::string expName, int timestep,
            std::string varName, int varVersion, std::vector<int> dimOffsets, int rank)
{
    std::string offsets = "";
    for (int n : dimOffsets) 
    {
        offsets += (std::to_string(n) + "-");
    }
    
    // TODO:  Implement per Margaret's prototype.
    std::string oid = (
            "JobId:" + jobId + "-" + 
            "ExpName:" + expName + "-" + 
            "Step:" + std::to_string(timestep) + "-" + 
            "Var:" + varName + "-" + 
            "VarVer:" + std::to_string(varVersion) + "-" + 
            "Dims:" + offsets + "-" + 
            "rank-" + 
            std::to_string(rank)
    );

    return oid;
}

CephWriter::CephWriter(IO &io, const std::string &name, const Mode mode,
                           MPI_Comm mpiComm)
: Engine("CephWriter", io, name, mode, mpiComm)
{
    m_EndMessage = " in call to IO Open CephWriter " + m_Name + "\n";
    
    MPI_Comm_rank(mpiComm, &m_WriterRank);
    Init();
        
    if (m_DebugMode)
    {
        std::cout << "CephWriter::constructor:rank("  << m_WriterRank 
            << ") m_Name=" << m_Name << std::endl;
    }
}

CephWriter::~CephWriter() = default;

StepStatus CephWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    
    m_CurrentStep++; 

    if (m_DebugMode)
    {
        std::cout << "CephWriter::BeginStep:rank("  << m_WriterRank 
                << ") m_CurrentStep is now: " << m_CurrentStep << std::endl;
    }
    return StepStatus::OK;
}

// calls putsync for each var, with a non-empty bl. putsync will decide to actually flush objects 
// to disk or wait based on other state info.
void CephWriter::PerformPuts()
{
    for(auto& var: m_IO.GetAvailableVariables())
    {
        // skip trying to write if there is no data for this var.
        if(m_Buffs.at(var.first)->length() > 0)
        {
            if (m_DebugMode)
            {        
                std::cout << "CephWriter::PerformPuts:rank("  << m_WriterRank 
                        << ")  calling putsync for var=" << var.first << std::endl;
            }
        
            PutSync(var.first);
        }
        else 
        {
            if (m_DebugMode)
            {        
                std::cout << "CephWriter::PerformPuts:rank("  << m_WriterRank 
                        << ")  nothing remaining to write for var=" << var.first << std::endl;
            }
        }
    }
}

void CephWriter::EndStep()
{
    if (m_DebugMode)
    {
        std::cout << "CephWriter::EndStep:rank("  << m_WriterRank 
                << ") current step:" << m_CurrentStep << std::endl;
    }
    
     // we do not consider objects to contain data across timestep boundaries
    // so force a disk flush (write objs) since this is the end of a timestep.
    m_ForceFlush = true; 
    
    // will call putsync for each var
    PerformPuts();    
    
    // reset state
    m_ForceFlush = false;
}

size_t CephWriter::CurrentStep() 
{
    return m_CurrentStep;
}


void CephWriter::DoClose(const int transportIndex)
{
    // NOTE: this forces a final write, (DoClose and flush both force a write)
    // TODO: flush all (delayed) metadata to EMPRESS here, that
    // saves comms during mpi run
    // however we should not have any data to flush, if we are not using any
    // delayed vars.  all data should be written during putsync.
    if (m_DebugMode)
    {
        std::cout << "CephWriter::DoClose:rank("  << m_WriterRank 
                << "). transportIndex=" << transportIndex << std::endl;
    }
    
    // we do not consider objects to contain data across timestep boundaries
    // so force a disk flush (write objs) since this is the end of a timestep.
    m_ForceFlush = true; 
    
    // will call putsync for each var
    PerformPuts();    
    
    // reset state
    m_ForceFlush = false;
    
    // TODO: move to putsync only: transport->Write(oid, *m_Buffs.at(varname), size, start, elemSize, "variable.m_Type");
        
#ifdef USE_CEPH_OBJ_TRANS
    transport->Close();   // essentially a no-op for us.  ? or 
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
        m_ExpName = it->second;  // also used in the transport
        Params p = {{it->first, it->second}};
        m_IO.m_TransportsParameters.push_back(p);
    }
    
    it = m_IO.m_Parameters.find("JobId");
    if (it == m_IO.m_Parameters.end())
        it = m_IO.m_Parameters.find("jobid");
    if (it != m_IO.m_Parameters.end())
    {
        m_JobId = it->second;  // also used in the transport
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

 
void CephWriter::InitTransports()
{
    if (m_DebugMode)
    {
        std::cout << "CephWriter::InitTransports:rank("  << m_WriterRank 
                << ")" << std::endl;
    }
    transport = std::shared_ptr<transport::CephObjTrans>(
        new transport::CephObjTrans(
                m_MPIComm, m_IO.m_TransportsParameters, true));
    const std::string name = "fname-rank-" + std::to_string(m_WriterRank);
    const Mode mode = Mode::Write;
    transport->Open(name, mode);
}
  
void CephWriter::InitBuffer()
{
    if (m_DebugMode)
    {
        std::cout << "CephWriter::InitBuffer:rank("  << m_WriterRank 
                << "): intializing bufferlists for variables:\n";
    }
    
#ifdef USE_CEPH_OBJ_TRANS
    
    // create an empty bufferlist per variable
    for(auto& var: m_IO.GetAvailableVariables())
    {
        if (m_DebugMode) 
        {
            Params p = var.second;
            std::cout << "\n\tVarname=" << var.first << ": Vartype=" 
                    << m_IO.InquireVariableType(var.first) << ". empty="
                    << ((p.empty())? "no":"false") << ".  Params map.size()=" << p.size() << std::endl;
            std::map<std::string, std::string>::iterator it;
            for (it = p.begin(); it!=p.end(); it++) 
            {   
                std::cout << "\t" << it->first << ":" << it->second << std::endl;
            }
        }

        m_Buffs[var.first] = new librados::bufferlist();
    }
    std::cout << std::endl;
    
#endif /* USE_CEPH_OBJ_TRANS */
    
}



} // end namespace adios2
