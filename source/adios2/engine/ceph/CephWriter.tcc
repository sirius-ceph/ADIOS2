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
    // set variable
    variable.SetData(values);
    if (m_Verbosity == 5)
    {
        std::cout << "CephWriter " << m_WriterRank << "     PutSync("
                  << variable.m_Name << ")\n";
    }
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
