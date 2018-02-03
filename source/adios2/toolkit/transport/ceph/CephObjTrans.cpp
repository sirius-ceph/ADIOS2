/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileDescriptor.cpp file I/O using POSIX I/O library
 *
 *  Created on: 
 *      Author: 
 */
#include "CephObjTrans.h"
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

CephObjTrans::CephObjTrans(MPI_Comm mpiComm, const bool debugMode)
: Transport("CephObjTrans", "cephlibrados", mpiComm, debugMode)
{
    
    // taken from Ken: https://github.com/kiizawa/siriusdev/blob/master/sample.cpp
    // need to get cluster handle, storage tier pool handle, archive tier pool handle here
    int ret = 0;
    uint64_t flags;

    /* Initialize the cluster handle with the "ceph" cluster name and "client.admin" user */
    ret = m_rcluster.init2(m_user_name.c_str(), m_rcluster_name.c_str(), flags);
    if (ret < 0) 
    {
         throw std::ios_base::failure("Transport::Ceph Couldn't initialize the cluster handle! error= "  + std::to_string(ret) + "\n");
    }
    
      /* Read a Ceph configuration file to configure the cluster handle. */
    ret = m_rcluster.conf_read_file("/share/ceph.conf");
    if (ret < 0) 
    {
        throw std::ios_base::failure("Transport::Ceph Couldn't read the Ceph configuration file! error= "  + std::to_string(ret) + "\n");
    }
        
      /* Connect to the cluster */
    ret = m_rcluster.connect();
    if (ret < 0) 
    {
        throw std::ios_base::failure("Transport::Ceph Couldn't connect to cluster! error= "  + std::to_string(ret) + "\n");
    }

     /* Set up the storage and archive pools for tieiring. */
    ret = m_rcluster.ioctx_create("storage_pool", m_io_ctx_storage);
    if (ret < 0)
    {
        throw std::ios_base::failure("Transport::Ceph Couldn't set up ioctx! error= "  + std::to_string(ret) + "\n");
    }

    ret = m_rcluster.ioctx_create("archive_pool", m_io_ctx_archive);
    if (ret < 0)
   {
        throw std::ios_base::failure("Transport::Ceph Couldn't set up ioctx! error= "  + std::to_string(ret) + "\n");
    }
      
}

// todo:
// dont need file handle.  need to pass in oid not start offset.
// need write(*buf, size, oid)
// need chekcsize(oid)
// need check if exists(oid) : globally fatal if collision.
// skip append mode for now.


CephObjTrans::~CephObjTrans()
{
    if (m_IsOpen)
    {
        close(m_FileDescriptor);
    }
}

void CephObjTrans::Open(const std::string &name, const Mode openMode)
{
    m_Name = name;
    m_oname= name;
    // CheckName();
    m_OpenMode = openMode;

    switch (m_OpenMode)
    {

    case (Mode::Write):
        // check if obj exists, fatal error 
        // m_FileDescriptor = open(m_Name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0777);
        if (ObjExists()) 
        {
             throw std::ios_base::failure("ERROR: Object exists.  oname=" + m_oname);
        }
        break;

    case (Mode::Append):
        // todo: append to an existing object.
        // 1. check if obj exists
        // 2. check obj size.
        // 3. check objector params
        break;

    case (Mode::Read):
    break;

    default:
        CheckFile("unknown open mode for file " + m_Name +
                  ", in call to POSIX open");
    }

    CheckFile("couldn't open file " + m_Name +
              ", check permissions or path existence, in call to POSIX open");

    m_IsOpen = true;
}

/* test if oid already exists in the current ceph cluster */
bool CephObjTrans::ObjExists() 
{    
    // http://docs.ceph.com/docs/master/rados/api/librados/#c.rados_ioctx_    
    // librados::IoCtx io_ctx_storage;    
    
    uint64_t psize;
    std::time_t pmtime;
    //rados_ioctx_t io;  
    return ( m_io_ctx_storage.stat(m_oname, &psize, &pmtime) == 0) ? true : false;
    
    //return (rados_stat(io, m_oname.c_str(), &psize, &pmtime) == 0) ? true : false;
}

void CephObjTrans::Write(const char *buffer, size_t size, size_t start)
{
    auto lf_Write = [&](const char *buffer, size_t size) {

        ProfilerStart("write");
        const auto writtenSize = write(m_FileDescriptor, buffer, size);
        ProfilerStop("write");

        if (writtenSize == -1)
        {
            throw std::ios_base::failure("ERROR: couldn't write to file " +
                                         m_Name +
                                         ", in call to FileDescriptor Write\n");
        }

        if (static_cast<size_t>(writtenSize) != size)
        {
            throw std::ios_base::failure(
                "ERROR: written size + " + std::to_string(writtenSize) +
                " is not equal to intended size " + std::to_string(size) +
                " in file " + m_Name + ", in call to FileDescriptor Write\n");
        }
    };

    if (start != MaxSizeT)
    {
        const auto newPosition = lseek(m_FileDescriptor, start, SEEK_SET);

        if (static_cast<size_t>(newPosition) != start)
        {
            throw std::ios_base::failure(
                "ERROR: couldn't move to start position " +
                std::to_string(start) + " in file " + m_Name +
                ", in call to POSIX lseek\n");
        }
    }

    if (size > DefaultMaxFileBatchSize)
    {
        const size_t batches = size / DefaultMaxFileBatchSize;
        const size_t remainder = size % DefaultMaxFileBatchSize;

        size_t position = 0;
        for (size_t b = 0; b < batches; ++b)
        {
            lf_Write(&buffer[position], DefaultMaxFileBatchSize);
            position += DefaultMaxFileBatchSize;
        }
        lf_Write(&buffer[position], remainder);
    }
    else
    {
        lf_Write(buffer, size);
    }
}

void CephObjTrans::Read(char *buffer, size_t size, size_t start)
{
    auto lf_Read = [&](char *buffer, size_t size) {

        ProfilerStart("read");
        const auto readSize = read(m_FileDescriptor, buffer, size);
        ProfilerStop("read");

        if (readSize == -1)
        {
            throw std::ios_base::failure("ERROR: couldn't read from file " +
                                         m_Name +
                                         ", in call to POSIX IO read\n");
        }

        if (static_cast<size_t>(readSize) != size)
        {
            throw std::ios_base::failure(
                "ERROR: read size + " + std::to_string(readSize) +
                " is not equal to intended size " + std::to_string(size) +
                " in file " + m_Name + ", in call to POSIX IO read\n");
        }
    };

    if (start != MaxSizeT)
    {
        const auto newPosition = lseek(m_FileDescriptor, start, SEEK_SET);

        if (static_cast<size_t>(newPosition) != start)
        {
            throw std::ios_base::failure(
                "ERROR: couldn't move to start position " +
                std::to_string(start) + " in file " + m_Name +
                ", in call to POSIX lseek errno " + std::to_string(errno) +
                "\n");
        }
    }

    if (size > DefaultMaxFileBatchSize)
    {
        const size_t batches = size / DefaultMaxFileBatchSize;
        const size_t remainder = size % DefaultMaxFileBatchSize;

        size_t position = 0;
        for (size_t b = 0; b < batches; ++b)
        {
            lf_Read(&buffer[position], DefaultMaxFileBatchSize);
            position += DefaultMaxFileBatchSize;
        }
        lf_Read(&buffer[position], remainder);
    }
    else
    {
        lf_Read(buffer, size);
    }
}

size_t CephObjTrans::GetSize()
{
    struct stat fileStat;
    if (fstat(m_FileDescriptor, &fileStat) == -1)
    {
        throw std::ios_base::failure("ERROR: couldn't get size of file " +
                                     m_Name + "\n");
    }
    return static_cast<size_t>(fileStat.st_size);
}

void CephObjTrans::Flush() {}

void CephObjTrans::Close()
{
    ProfilerStart("close");
    const int status = close(m_FileDescriptor);
    ProfilerStop("close");

    if (status == -1)
    {
        throw std::ios_base::failure("ERROR: couldn't close file " + m_Name +
                                     ", in call to POSIX IO close\n");
    }

    m_IsOpen = false;
}

void CephObjTrans::CheckFile(const std::string hint) const
{
    if (m_FileDescriptor == -1)
    {
        throw std::ios_base::failure("ERROR: " + hint + "\n");
    }
}


} // end namespace transport
} // end namespace adios2
