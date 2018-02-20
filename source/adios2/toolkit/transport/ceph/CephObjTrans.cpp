/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CephObjTrans.cpp writes Ceph objects using metadata from SIRIUS and EMPRESS
 *
 *  Created on: 
 *      Author: 
 */
#include "CephObjTrans.h"
#include "CephObjMover.h"
#include <rados/librados.hpp>
#include <rados/rados_types.hpp>
#include <iostream>
#include <string>


#include <fcntl.h>     // open
#include <stddef.h>    // write output
#include <sys/stat.h>  // open, fstat
#include <sys/types.h> // open
#include <unistd.h>    // write, close

/// \cond EXCLUDE_FROM_DOXYGEN
#include <ios> //std::ios_base::failure
/// \endcond

namespace adios2
{
namespace transport
{

// static
std::string CephObjTrans::ParamsToLower(std::string s) 
{
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}   

CephObjTrans::CephObjTrans(MPI_Comm mpiComm, const std::vector<Params> &params,
    const bool debugMode)
: Transport("CephObjTrans", "cephlibrados", mpiComm, debugMode), 
  m_CephStorageTier(CephStorageTier::FAST),
  m_CephClusterName("ceph"),
  m_CephUserName("client.admin"),
  m_CephConfFilePath("/share/ceph.conf"),
  m_TargetObjSize(8388608),
  m_ExpName("MyExperimentName"),
  m_JobId(112)
{
    // fyi: this->m_MPIComm; // (is avail in the transport class)
    
    // set the private vars using Transport Params
    for (auto it = params.begin(); it != params.end(); ++it)
    {
        int count = 0;
        for (auto elem : *it)
        {
            if (0) 
            {
                std::cout << "CephObjTrans::constructor:rank("
                        << m_RankMPI << "):" << elem.first<< "=>" 
                        << elem.second << std::endl;
            }
            
            if(ParamsToLower(elem.first) == "verbose") 
            {
                m_Verbosity = std::stoi(elem.second);
            }
            else if(ParamsToLower(elem.first) == "cephusername") 
            {
                m_CephUserName = elem.second;
            }
            else if(ParamsToLower(elem.first) == "cephclustername") 
            {   
                m_CephClusterName = elem.second;
            }
            else if(ParamsToLower(elem.first) == "cephstoragetier") 
            { 
                if (ParamsToLower(elem.second)== "fast") 
                    m_CephStorageTier = CephStorageTier::FAST;
                else if (ParamsToLower(elem.second)== "slow") 
                    m_CephStorageTier = CephStorageTier::SLOW;
                else if (ParamsToLower(elem.second)== "archive") 
                    m_CephStorageTier = CephStorageTier::ARCHIVE;
                else if (ParamsToLower(elem.second)== "tape") 
                    m_CephStorageTier = CephStorageTier::ARCHIVE;
            }
            else if(ParamsToLower(elem.first) == "cephconffilepath") 
            {   
                m_CephConfFilePath = elem.second;
            }
            else if(ParamsToLower(elem.first) == "expname") 
            {   
                m_ExpName = elem.second;
            }
            else if(ParamsToLower(elem.first) == "jobid") 
            {   
                m_JobId = std::stoi(elem.second);
            }
            else if(ParamsToLower(elem.first) == "targetobjsize") 
            {   
                m_TargetObjSize = std::stoul(elem.second);
            }
            else 
            {
                if(debugMode) 
                {
                    DebugPrint("Constructor:rank(" + std::to_string(m_RankMPI) 
                            + "):Unrecognized parameter:" + elem.first + "=>" 
                            + elem.second, false);
                }
            }
        }
    
    }   
    
    if (debugMode) 
        DebugPrint("Constructor:rank(" + std::to_string(m_RankMPI) + ")", 
                true);
    
}

// todo:
// dont need file handle.  need to pass in oid not start offset.
// need write(*buf, size, oid)
// need chekcsize(oid)
// need check if exists(oid) : globally fatal if collision.
// skip append mode for now.


CephObjTrans::~CephObjTrans() {}

void CephObjTrans::Open(const std::string &name, const Mode openMode) 
{
    std::string mode;
    m_Name = name;
    m_OpenMode = openMode;

    switch (m_OpenMode)
    {

    case (Mode::Write):
        mode = "Write";
        break;

    case (Mode::Append):
        mode = "Append";
        // TODO: append to an existing object.
        // 1. check if obj exists
        // 2. check obj size.
        // 3. check objector params
        break;

    case (Mode::Read):
        mode = "Read";
        // TODO
        break;

    default:
        // noop
        break;
    }
    
    if(m_DebugMode)
    {
        DebugPrint("Open(const std::string &name=" + name 
                + ", const Mode OpenMode=" + mode + ")", false);
    }

    //  from Ken: https://github.com/kiizawa/siriusdev/blob/master/sample.cpp
    int ret = 0;
    uint64_t flags;
    
    /* Initialize the cluster handle with cluster name  user name */
    if(m_DebugMode)
    {
        DebugPrint("Open:m_RadosCluster.init2(" + m_CephUserName + "," 
                + m_CephClusterName + ")"  , false);
    }
    ret = m_RadosCluster.init2(m_CephUserName.c_str(), 
            m_CephClusterName.c_str(), flags);
    if (ret < 0) 
    {
         throw std::ios_base::failure("CephObjTrans::Open:Ceph Couldn't " \
                "initialize the cluster handle! error= "  + std::to_string(ret) + "\n");
    }
    
      /* Read a Ceph configuration file to configure the cluster handle. */
    if(m_DebugMode)
    {
        DebugPrint("Open:m_RadosCluster.conf_read_file(" + m_CephConfFilePath 
                + ")", false);
    }
    ret = m_RadosCluster.conf_read_file(m_CephConfFilePath.c_str());
    if (ret < 0) 
    {
        throw std::ios_base::failure("CephObjTrans::Open:Ceph Couldn't " \
                "read the Ceph configuration file! error= "  
                + std::to_string(ret) + "\n");
    }
        
      /* Connect to the cluster */
    if(m_DebugMode)
    {
        DebugPrint("Open:m_RadosCluster.connect()"  , false);
    }
    //ret = m_RadosCluster.connect();
    if (ret < 0) 
    {
        throw std::ios_base::failure("CephObjTrans::Open:Ceph Couldn't " \
                "connect to cluster! error= "  + std::to_string(ret) + "\n");
    }

     /* Set up the storage and archive pools for tieiring. */
    if(m_DebugMode)
    {
        DebugPrint("Open:m_RadosCluster.ioctx_create(" \
                "storage_pool, m_IoCtxStorage)" , false);
    }
    //ret = m_RadosCluster.ioctx_create("storage_pool", m_IoCtxStorage);
    if (ret < 0)
    {
        throw std::ios_base::failure("CephObjTrans::Open:Ceph Couldn't " \
                "set up ioctx! error= "  + std::to_string(ret) + "\n");
    }

    if(m_DebugMode)
    {
        DebugPrint("Open:CephObjTrans::Open:m_RadosCluster.ioctx_create(" \
                "archive_pool, m_IoCtxArchive)" , false);
    }
    //ret = m_RadosCluster.ioctx_create("archive_pool", m_IoCtxArchive);
    if (ret < 0)
   {
        throw std::ios_base::failure("CephObjTrans::Open:Ceph Couldn't " \
            "set up ioctx! error= "  + std::to_string(ret) + "\n");
    }
    
    m_IsOpen = true;
}
    
/* test if oid already exists in the current ceph cluster */
bool CephObjTrans::ObjExists(const std::string &oid) 
{    
    // http://docs.ceph.com/docs/master/rados/api/librados/#c.rados_ioctx_    
    // librados::IoCtx io_ctx_storage;    
    
    uint64_t psize;
    std::time_t pmtime;
      //rados_ioctx_t io;  
    return ( m_IoCtxStorage.stat(oid, &psize, &pmtime) == 0) ? true : false;
    //return (rados_stat(io, m_oname.c_str(), &psize, &pmtime) == 0) ? true : false;
    // could check -ENOENT
}

void CephObjTrans::Write(std::string oid, const librados::bufferlist *bl, 
    size_t size, size_t start)
{
    

}


void CephObjTrans::Read(char *buffer, size_t size, size_t start)
{
    std::string msg = ("ERROR:  CephObjTrans doesn't implement \n "\
            "Read(char *buffer, size_t size, size_t start) yet \n");
    throw std::invalid_argument(msg);
}

size_t CephObjTrans::GetSize()
{
    struct stat fileStat;
    //~ return static_cast<size_t>(fileStat.st_size);
}

void CephObjTrans::Flush() {}

void CephObjTrans::Close()
{
    m_IsOpen = false;
}


// private
void CephObjTrans::DebugPrint(std::string msg, bool printAll)
{
    if(!printAll) 
    {
        std::cout << "CephObjTrans::" << msg << std::endl;
    }
    else 
    {    
        std::cout << "CephObjTrans::" << msg << ":m_CephUserName="
                << m_CephUserName << std::endl;
        std::cout << "CephObjTrans::" << msg << ":m_CephClusterName="
                << m_CephClusterName << std::endl;
        std::cout << "CephObjTrans::" << msg << ":m_CephConfFilePath="
                << m_CephConfFilePath << std::endl;
        
        std::string tier = "EINVAL";
        if (m_CephStorageTier==CephStorageTier::FAST) tier = "fast";
        else if (m_CephStorageTier==CephStorageTier::SLOW) tier = "slow";
        else if (m_CephStorageTier==CephStorageTier::ARCHIVE) tier = "archive";
        std::cout << msg << ":m_CephStorageTier="<< tier << std::endl;    
        
        std::cout << "CephObjTrans::" << msg << ":m_JobId="<< m_JobId
                << std::endl;
        std::cout << "CephObjTrans::" << msg << ":m_ExpName="<< m_ExpName
                << std::endl;
        std::cout << "CephObjTrans::" << msg << ":m_TargetObjSize="
                << m_TargetObjSize << std::endl;
    }
}


} // end namespace transport
} // end namespace adios2

