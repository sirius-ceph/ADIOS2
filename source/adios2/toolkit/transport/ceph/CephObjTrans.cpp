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

// private
void CephObjTrans::DebugPrint(std::string msg)
{
    std::cout << "CephObjTrans::" << msg << ": m_CephUserName="<< m_CephUserName << std::endl;
    std::cout << "CephObjTrans::" << msg << ": m_CephClusterName="<< m_CephClusterName << std::endl;
    std::cout << "CephObjTrans::" << msg << ": m_CephConfFilePath="<< m_CephConfFilePath << std::endl;
    
    std::string tier = "EINVAL";
    if (m_CephStorageTier==CephStorageTier::FAST) tier = "fast";
    else if (m_CephStorageTier==CephStorageTier::SLOW) tier = "slow";
    else if (m_CephStorageTier==CephStorageTier::ARCHIVE) tier = "archive";
    std::cout << "CephObjTrans::" << msg << ": m_CephStorageTier="<< tier << std::endl;    
}

CephObjTrans::CephObjTrans(MPI_Comm mpiComm,  const std::vector<Params> &params, const bool debugMode)
: Transport("CephObjTrans", "cephlibrados", mpiComm, debugMode)
{
        // fyi: this->m_MPIComm; // (is avail in the transport class)
    //std::cout << "rank=" << this->m_WriterRank << std::endl; 
    
    m_CephStorageTier = CephStorageTier::FAST;
    m_CephClusterName = "ceph";
    m_CephUserName = "client.admin";
    m_CephConfFilePath = "/share/ceph.conf";

    // set the private vars using Transport Params
    for (auto it = params.begin(); it != params.end(); ++it)
    {
        int count = 0;
        for (auto elem : *it)
        {
            //std::cout << "CephObjTrans:: constructor: Transp Params("<< count++ << "):" << elem.first<< "=>" << elem.second << std::endl;
            
            if(ParamsToLower(elem.first) == "cephusername") 
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
        }
    
    }   
    
    if (debugMode) DebugPrint("constructor:rank:");
    
   
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
    std::cout << "CephObjTrans::Open(const std::string &name, const Mode OpenMode" << std::endl;
    
    m_Name = name;
    m_OpenMode = openMode;

    switch (m_OpenMode)
    {

    case (Mode::Write):
        std::cout << "CephObjTrans::Open:Mode::Write" << std::endl;
        break;

    case (Mode::Append):
        std::cout << "CephObjTrans::Open:Mode::Append" << std::endl;
        // TODO: append to an existing object.
        // 1. check if obj exists
        // 2. check obj size.
        // 3. check objector params
        break;

    case (Mode::Read):
        std::cout << "CephObjTrans::Open:Mode::Read" << std::endl;
        // TODO
        break;

    default:
        // noop
        break;
    }
    
    //~ //  from Ken: https://github.com/kiizawa/siriusdev/blob/master/sample.cpp
    //~ // need to get cluster handle, storage tier pool handle, archive tier pool handle here   

    int ret = 0;
    uint64_t flags;
    
    //~ /* Initialize the cluster handle with the "ceph" cluster name and "client.admin" user */
    std::cout << "CephObjTrans::Open:m_RadosCluster.init2(" << m_CephUserName << "," << m_CephClusterName << ") now:" << std::endl;
    ret = m_RadosCluster.init2(m_CephUserName.c_str(), m_CephClusterName.c_str(), flags);
    if (ret < 0) 
    {
         throw std::ios_base::failure("CephObjTrans::Open:Ceph Couldn't initialize the cluster handle! error= "  + std::to_string(ret) + "\n");
    }
    
      /* Read a Ceph configuration file to configure the cluster handle. */
    std::cout << "m_RadosCluster.conf_read_file(" << m_CephConfFilePath << ") now:" << std::endl;
    ret = m_RadosCluster.conf_read_file(m_CephConfFilePath.c_str());
    if (ret < 0) 
    {
        throw std::ios_base::failure("CephObjTrans::Open:Ceph Couldn't read the Ceph configuration file! error= "  + std::to_string(ret) + "\n");
    }
        
      /* Connect to the cluster */
    std::cout << "CephObjTrans::Open:m_RadosCluster.connect() now:" << std::endl;
    ret = m_RadosCluster.connect();
    if (ret < 0) 
    {
        throw std::ios_base::failure("CephObjTrans::Open:Ceph Couldn't connect to cluster! error= "  + std::to_string(ret) + "\n");
    }

     /* Set up the storage and archive pools for tieiring. */
    std::cout << "CephObjTrans::Open:m_RadosCluster.ioctx_create(" << "storage_pool" << "," << "m_IoCtxStorage" << ") now:" << std::endl;
    ret = m_RadosCluster.ioctx_create("storage_pool", m_IoCtxStorage);
    if (ret < 0)
    {
        throw std::ios_base::failure("CephObjTrans::Open:Ceph Couldn't set up ioctx! error= "  + std::to_string(ret) + "\n");
    }

    std::cout << "CephObjTrans::Open:CephObjTrans::Open:m_RadosCluster.ioctx_create(" << "archive_pool" << "," << "m_IoCtxArchive" << ") now:" << std::endl;
    ret = m_RadosCluster.ioctx_create("archive_pool", m_IoCtxArchive);
    if (ret < 0)
   {
        throw std::ios_base::failure("CephObjTrans::Open:Ceph Couldn't set up ioctx! error= "  + std::to_string(ret) + "\n");
    }
    
    std::cout << "CephObjTrans::Open:m_RadosCluster init done.(" << "archive_pool" << "," << "m_IoCtxArchive" << ") now:" << std::endl;
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

void CephObjTrans::Write(std::string oid, const librados::bufferlist *bl, size_t size, size_t start)
{
    

}


void CephObjTrans::Write(const char *buffer, size_t size, size_t start)
{
    std::string msg = ("ERROR:  CephObjTrans doesn't implement \n" \
            "Write(const char *buffer, size_t size, size_t start), use \n" \
            "Write(std::string oid, const librados::bufferlist *bl, size_t size, size_t start) \n");
    throw std::invalid_argument(msg);
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

void CephObjTrans::CheckFile(const std::string hint) const
{
}


} // end namespace transport
} // end namespace adios2


//~ std::vector<adios2::Params> v = parametersVector;
//~ for (std::vector<adios2::Params>::iterator it = v.begin(); it != v.end(); ++it)
//~ {
    //~ adios2::Params p = *it;
    //~ for (std::map<std::string,std::string>::iterator i=p.begin(); i!=p.end(); ++i)
    //~ { 
        //~ std::cout << "CephObjTrans::Open:Transp Params: " << i->first << " => " << i->second << '\n';
    //~ }
//~ }
    
