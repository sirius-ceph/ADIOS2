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
void CephWriter::PrintVarInfo(Variable<T> &variable, const T *values)
{       
       //<< this->m_IO.InquireVariableType(variable.m_Name) << ");";
        std::cout \
            << "variable.m_Name=" << variable.m_Name 
            << ";.m_Type=" << variable.m_Type 
            << ";.m_ShapeID=";
    
        std::string msg = "";
        if(variable.m_ShapeID==ShapeID::GlobalValue) msg = "GlobalValue";
        if(variable.m_ShapeID==ShapeID::GlobalArray) msg = "GlobalArray";
        if(variable.m_ShapeID==ShapeID::JoinedArray) msg = "JoinedArray";
        if(variable.m_ShapeID==ShapeID::LocalArray) msg = "LocalArray";
        if(variable.m_ShapeID==ShapeID::LocalValue) msg = "LocalValue";
        std::cout << msg;
        
        std::cout \
            << ";.m_ElementSize=" << variable.m_ElementSize 
            << "; m_ConstantDims=" << (variable.m_ConstantDims?"true":"false")
            << "; m_SingleValue=" << (variable.m_SingleValue ?"true":"false")
            << "; ";
        
        std::cout << ".m_Shape=";
        Dims shape = variable.m_Shape;
        for (auto d: shape) std::cout << d << ",";
        std::cout << ";";
        
        std::cout << ".m_Start=";
        Dims start = variable.m_Start;
        for (auto d: start) std::cout << d << ",";
        std::cout << ";";
    
        std::cout << ".m_Count=";
        Dims count = variable.m_Count;
        for (auto d: count) std::cout << d << ",";
        std::cout << ";.m_IndexStepBlockStarts:keys=" << std::endl;
        
        for(auto it:variable.m_IndexStepBlockStarts) 
        {
            std::cout << it.first << ",";
        }
        std::cout << ";";
        
//            GetAvailableStepsStart()

        std::cout << ".m_AvailableStepsStart=" << 
            variable.m_AvailableStepsStart << ";";
        
        std::cout << ".m_AvailableStepsCount=" << 
            variable.m_AvailableStepsCount << ";";
        
        std::cout << ".m_StepsStart=" << 
            variable.m_StepsStart << ";";
        
        std::cout << ".m_StepsCount=" << 
            variable.m_StepsCount << ";";
        
        std::cout << ".TotalSize=" << 
            variable.TotalSize() << ";";
        
        std::cout << ".PayloadSize=" << 
            variable.PayloadSize() << ";";
        
        std::cout << std::endl;
}
    
template <class T>
void CephWriter::PutSyncCommon(Variable<T> &variable, const T *values)
{
    if (m_DebugMode)
    {
        std::cout << "CephWriter::PutSyncCommon:rank("  << m_WriterRank 
            << ")";
        
        PrintVarInfo(variable, values);
    }
    
    const size_t varsize = variable.PayloadSize();
    
    // set variable
    variable.SetData(values);    
    
    // CephWriter
    // 0. if prescribed steps 
    //      0a. write current BL as obj to ceph.
    //      0b. clear BL
    // 1. append vals to BL
    
    const size_t currentStep = CurrentStep();    
    
    const int varVersion = 0; // will be used later with EMPRESS
    
    // TODO: get actual Dims per variable.
    std::vector<int> dimOffsets = {0,0,0};
    
#ifdef USE_CEPH_OBJ_TRANS
    if (currentStep % m_FlushStepsCount == 0)  // prescribed by EMPRESS
    {
        std::string oid = Objector(
                m_JobId,
                m_ExpName, 
                currentStep,
                variable.m_Name,
                varVersion,
                dimOffsets,
                m_WriterRank);
        if (m_DebugMode)
        {
            std::cout << "CephWriter::PutSyncCommon:rank("  << m_WriterRank 
                    << "): oid=" << oid << std::endl;
        }

        size_t size = m_bl->length();
        size_t offset = 0;  // zero for write full, get offset for object append.
        transport->Write(oid, m_bl, size, offset);
        m_bl->clear();
        m_bl->zero();
        
        // counters to keep track of number of steps in an object.
        m_TimestepStart = currentStep;
        m_TimestepEnd = -1;
    }
    
    // always add vals to buffer.
    m_bl->append((const char*)values, varsize);
    
#endif /* USE_CEPH_OBJ_TRANS */

    // BPFileWriter: try to resize buffer to hold new varsize if needed
    // format::BP3Base::ResizeResult resizeResult = m_BP3Serializer.ResizeBuffer(
    
    // BPFileWriter: resize result is to flush
    // 1. serialize data:  m_BP3Serializer.SerializeData(m_IO);
    // 2. write files:   m_FileDataManager.WriteFiles(m_BP3Serializer.m_Data.m_Buffer.data(), m_BP3Serializer.m_Data.m_Position);
    // 3. reset buffer: m_BP3Serializer.ResetBuffer(m_BP3Serializer.m_Data);
    
    // BPFileWriter: addtest var metadata and data to buffer.
    // m_BP3Serializer.PutVariableMetadata(variable);
    // m_BP3Serializer.PutVariablePayload(variable);s 
}

template <class T>
void CephWriter::PutDeferredCommon(Variable<T> &variable, const T *values)
{
    variable.SetData(values);

    if (m_DebugMode)
    {
        std::cout << "CephWriter::PutDeferredCommon:rank("  << m_WriterRank 
                << ") variable.m_Name=" << variable.m_Name << std::endl;
    }
    m_NeedPerformPuts = true;
}

} // end namespace adios2

#endif /* ADIOS2_ENGINE_CEPH_CEPHWRITER_TCC_ */
