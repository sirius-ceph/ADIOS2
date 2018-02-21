/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileDescriptor.h wrapper of POSIX library functions for file I/O
 *
 *  Created on: 
 *      Author: 
 */

#ifndef ADIOS2_TOOLKIT_TRANSPORT_CEPHOBJTRANS_H_
#define ADIOS2_TOOLKIT_TRANSPORT_CEPHOBJTRANS_H_

#include <algorithm>
#include <string>

#include <rados/librados.hpp>
#include <rados/rados_types.hpp>
#include "adios2/ADIOSConfig.h"
#include "adios2/toolkit/transport/Transport.h"

//#ifndef USE_CEPH_OBJ_TRANS
//#define USE_CEPH_OBJ_TRANS 0
//#endif

namespace adios2
{
namespace transport
{

    /** Used to set Ceph Storage tier for writes */
enum class CephStorageTier
{
    FAST,  // e.g. SSD
    SLOW,  // e.g., HDD
    ARCHIVE  // e.g., tape
};

class CephObjTrans : public Transport
{

public:
    CephObjTrans(MPI_Comm mpiComm, const std::vector<Params> &params, const bool debugMode);

    ~CephObjTrans();


    void Open(const std::string &name, const Mode openMode) final;
   // void Open(const std::string &name, const Mode openMode, const std::vector<Params> &parametersVector);

    void Write(const char *buffer, size_t size, size_t start = MaxSizeT) final {};
    void Write(std::string oid, librados::bufferlist *bl, size_t size, size_t start);


    void Read(char *buffer, size_t size, size_t start = MaxSizeT) final;

    size_t GetSize() final {};
    size_t GetObjSize(std::string oid);

    /** Does nothing, each write is supposed to flush */
    void Flush() final;
    void Close() final;


private:
    int m_Verbosity;

    // internal utils
    //void CheckFile(const std::string hint) const {};
    static std::string ParamsToLower(std::string s);
    void DebugPrint(std::string msg, bool printAll);

    // ceph config vars
    CephStorageTier m_CephStorageTier = CephStorageTier::FAST;
    std::string m_CephClusterName;
    std::string m_CephUserName;
    std::string m_CephConfFilePath;

    // ceph cluster vars
    bool ObjExists(const std::string &oid);
    librados::Rados m_RadosCluster;
    librados::IoCtx m_IoCtxStorage;
    librados::IoCtx m_IoCtxArchive;
    //rados.ioctx_create(pool_name, io_ctx);
    //librados::IoCtx io_ctx;

    // ceph objector function vars.
    std::string m_ExpName;
    std::string m_JobId;
    size_t m_TargetObjSize = 8388608;
    
};

} // end namespace transport
} // end namespace adios2

#endif /* ADIOS2_TRANSPORT_CEPHOBJTRANS_H_ */
