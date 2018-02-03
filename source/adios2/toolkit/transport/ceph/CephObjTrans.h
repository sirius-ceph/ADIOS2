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

#include <rados/librados.hpp>
#include <rados/rados_types.hpp>
#include "adios2/ADIOSConfig.h"
#include "adios2/toolkit/transport/Transport.h"

namespace adios2
{
namespace transport
{

class CephObjTrans : public Transport
{

public:
    CephObjTrans(MPI_Comm mpiComm, const bool debugMode);

    ~CephObjTrans();

    void Open(const std::string &name, const Mode openMode) final;

    void Write(const char *buffer, size_t size, size_t start = MaxSizeT) final;

    void Read(char *buffer, size_t size, size_t start = MaxSizeT) final;

    size_t GetSize() final;

    /** Does nothing, each write is supposed to flush */
    void Flush() final;
    void Close() final;


private:
    /** POSIX file handle returned by Open */
    int m_FileDescriptor = -1;
    void CheckFile(const std::string hint) const;

    std::string m_oname =  "";
    bool ObjExists();
    librados::Rados m_rcluster;
    librados::IoCtx m_io_ctx_storage;
    librados::IoCtx m_io_ctx_archive;
//rados.ioctx_create(pool_name, io_ctx);
//librados::IoCtx io_ctx;

    const std::string m_rcluster_name = "ceph";
    const std::string m_user_name = "client.admin";

};

} // end namespace transport
} // end namespace adios2

#endif /* ADIOS2_TRANSPORT_CEPHOBJTRANS_H_ */
