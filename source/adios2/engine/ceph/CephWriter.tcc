/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CephWriter.tcc implementation of template functions with known type
 *
 *  Created on: 
 *      Author: 
 */
#ifndef ADIOS2_ENGINE_CEPH_CEPHWRITER_TCC_
#define ADIOS2_ENGINE_CEPH_CEPHWRITER_TCC_

#include "CephWriter.h"
#include <iostream>

namespace adios2
{

template <class T>
void CephWriter::PutSyncCommon(Variable<T> &variable, const T *values)
{
    
    if (m_Verbosity == 5)
    {
        std::cout << "CephWriter " << m_WriterRank << "     PutSync("
                  << variable.m_Name << ")\n";
    }
    
    const size_t varsize = variable.PayloadSize();
    
    // set variable
    variable.SetData(values);    
    
    // CephWriter
    // 0. if current object size = desried size
    //      0a. write current BL as obj to ceph.
    //      0b. clear BL
    // 1. append vals to BL

    
    // BPFileWriter: try to resize buffer to hold new varsize if needed
    // format::BP3Base::ResizeResult resizeResult = m_BP3Serializer.ResizeBuffer(
    
    // BPFileWriter: resize result is to flush
    // 1. serialize data:  m_BP3Serializer.SerializeData(m_IO);
    // 2. write files:   m_FileDataManager.WriteFiles(m_BP3Serializer.m_Data.m_Buffer.data(), m_BP3Serializer.m_Data.m_Position);
    // 3. reset buffer: m_BP3Serializer.ResetBuffer(m_BP3Serializer.m_Data);
    
    // BPFileWriter: addtest var metadata and data to buffer.
    // m_BP3Serializer.PutVariableMetadata(variable);
    // m_BP3Serializer.PutVariablePayload(variable);
    
}

template <class T>
void CephWriter::PutDeferredCommon(Variable<T> &variable, const T *values)
{
    variable.SetData(values);
    if (m_Verbosity == 5)
    {
        std::cout << "CephWriter " << m_WriterRank << "     PutDeferred("
                  << variable.m_Name << ")\n";
    }
    m_NeedPerformPuts = true;
}

} // end namespace adios2

#endif /* ADIOS2_ENGINE_CEPH_CEPHWRITER_TCC_ */
