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
        std::cout << "variable. " 
            << " m_Name=" << variable.m_Name 
            << " m_Type=" << variable.m_Type;
    
        std::string msg = "";
        if(variable.m_ShapeID==ShapeID::GlobalValue) msg = "GlobalValue";
        if(variable.m_ShapeID==ShapeID::GlobalArray) msg = "GlobalArray";
        if(variable.m_ShapeID==ShapeID::JoinedArray) msg = "JoinedArray";
        if(variable.m_ShapeID==ShapeID::LocalArray) msg = "LocalArray";
        if(variable.m_ShapeID==ShapeID::LocalValue) msg = "LocalValue";
        std::cout << " m_ShapeID="<< msg;
        
        std::cout \
            << " m_ElementSize=" << variable.m_ElementSize 
            << " m_ConstantDims=" << (variable.m_ConstantDims?"true":"false")
            << " m_SingleValue=" << (variable.m_SingleValue ?"true":"false");
        
        std::cout << " m_Shape=";
        Dims shape = variable.m_Shape;
        for (auto d: shape) std::cout << d << ",";
        
        std::cout << " m_Start=";
        Dims start = variable.m_Start;
        for (auto d: start) std::cout << d << ",";
    
        std::cout << " m_Count=";
        Dims count = variable.m_Count;
        for (auto d: count) std::cout << d << ",";
        std::cout << " m_IndexStepBlockStarts:keys=" << std::endl;
        
        for(auto it:variable.m_IndexStepBlockStarts) 
        {
            std::cout << it.first << ",";
        }

        std::cout << " m_AvailableStepsStart=" << 
            variable.m_AvailableStepsStart;
        
        std::cout << " m_AvailableStepsCount=" << 
            variable.m_AvailableStepsCount;
        
        std::cout << " m_StepsStart=" << 
            variable.m_StepsStart;
        
        std::cout << " m_StepsCount=" << 
            variable.m_StepsCount;
        
        std::cout << " TotalSize(num_elements)=" << 
            variable.TotalSize();
        
        std::cout << " PayloadSize=" << 
            variable.PayloadSize();

        std::cout << std::endl;
        
}

template<typename T>
void CephWriter::printVal(T val)
{
    std::cout << val << ',';
}

// Prints out the variable's elements from var data[] and bufferlist.
// TODO: add these DIMS type.<< ": m_Count=" << variable.m_Count
template <class T>
void CephWriter::PrintVarData(std::string msg, Variable<T> &variable, librados::bufferlist& bl)
{ 
    std::cout << msg << ":type:"<< variable.m_Type << ": .m_ElementSize=" << variable.m_ElementSize  << "; bl addr=" << &bl;

    // variable details
    const int paysize = variable.PayloadSize();
    const int num_elems = variable.TotalElems();
    const int elemsize = variable.m_ElementSize;
    
    // used to access elems in var data[] or bufferlist
    const char *ptr;
    
    // print out bufferlist elems
    std::cout << "\nbl.length=" << bl.length() << "\nbl  data=";
    ptr = bl.c_str();
    for (int i = 0; i < num_elems; i++, ptr+=elemsize)
    {
        void* tptr;
        if(variable.m_Type.find("int") != std::string::npos) 
        {            
            printVal<int>(*(int*)ptr);            
        }
        else if(variable.m_Type.find("float") != std::string::npos) 
        {            
            printVal<float>(*(float*)ptr);
        }
        else if(variable.m_Type.find("double") != std::string::npos) 
        {            
            printVal<double>(*(double*)ptr);
        }
        else if(variable.m_Type.find("string") != std::string::npos) 
        {           
            printVal<std::string>(*(std::string*)ptr);
        }
        else 
        {
            std::cout << "unhandled type <" << variable.m_Type << "> for variable name " << variable.m_Name << std::endl;
        }
    }
    
    // print out var data[] elems
    std::cout << "\nvar payload size=" << paysize  << "; num_elems=" << num_elems << "\nvar data=";
    ptr = (const char*)variable.GetData();
    for (int i = 0; i < num_elems; i++, ptr+=elemsize)
    {  
        if(variable.m_Type.find("int") != std::string::npos) 
        {            
            printVal<int>(*ptr);
        }
        else if(variable.m_Type.find("float") != std::string::npos) 
        {      
            printVal<float>(*ptr);            
        }
        else if(variable.m_Type.find("double") != std::string::npos) 
        {            
            printVal<double>(*ptr);
        }
        else if(variable.m_Type.find("string") != std::string::npos) 
        {           
            printVal<std::string>(*(std::string*)ptr);
        }
        else 
        {
            std::cout << "unhandled type <" << variable.m_Type << "> for variable name " << variable.m_Name << std::endl;
        }
    }
    std::cout << std::endl;
}

template <class T>
void CephWriter::PutSyncCommon(Variable<T> &variable, const T *values)
{
    if (0 && m_DebugMode)
    {
        std::cout << "\nCephWriter::PutSyncCommon:BEGIN:rank("  << m_WriterRank 
            << ")";
        
        PrintVarInfo(variable, values);
    }
    

      
    // CephWriter
    // 0. if prescribed steps 
    //      0a. write current BL as obj to ceph.
    //      0b. clear BL
    // 1. append vals to BL
    
    const size_t currentStep = CurrentStep();    
    const int varVersion = 0; // will be used later with EMPRESS
    
    // TODO: get actual Dims per variable.
    std::vector<int> dimOffsets = {0,0,0};
    
    // always add the data to the variable.
    variable.SetData(values); 
    
    // TODO: get remaining bytes in buffer for this variable.
    const int BUF_SZ_AVAIL = adios2::DefaultMaxBufferSize;
    if (BUF_SZ_AVAIL > variable.PayloadSize()) 
    {
        const char* vdata_ptr = (const char*)variable.GetData();
        const int vdata_size = variable.PayloadSize();
        m_Buffs.at(variable.m_Name)->append(vdata_ptr, vdata_size);
    }
    
#ifdef USE_CEPH_OBJ_TRANS
    if (currentStep % m_FlushStepsCount == 0)  // prescribed by EMPRESS
    {
        std::string oid = GetOid(
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
                    << "): FLUSHING oid=" << oid << "; varname=" << variable.m_Name 
                    << "; m_Buffs.at(" << variable.m_Name << " ) addr=" 
                    << m_Buffs.at(variable.m_Name) << "; ts=" << currentStep;
                    PrintVarData(" values:", variable, *m_Buffs.at(variable.m_Name));
                    std::cout << std::endl;
        }

        size_t size = m_Buffs.at(variable.m_Name)->length();
        size_t start = 0;  // zero for write full bl, otherwise get offset for last object append.
        //transport->Write(oid, m_Buffs.at(variable.m_Name), size, start, variable.m_ElementSize, variable.m_Type);
        m_Buffs.at(variable.m_Name)->clear();

    }
    
    
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
}

} // end namespace adios2

#endif /* ADIOS2_ENGINE_CEPH_CEPHWRITER_TCC_ */
