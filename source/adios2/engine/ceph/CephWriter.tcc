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
#include <type_traits>
#include <typeinfo>

namespace adios2
{
    
template <class T>
void CephWriter::PrintVarInfo(Variable<T> &variable) 
{       
       //<< this->m_IO.InquireVariableType(variable.m_Name) << ");";
        std::cout << "variable. " 
            << "\n\t.m_Name=" << variable.m_Name 
            << "\n\t.m_Type=" << variable.m_Type
            << "\n\t.m_Min=" << variable.m_Min
            << "\n\t.m_Max=" << variable.m_Max
            << "\n\t.m_Value=" << variable.m_Value;
    
        std::string msg = "";
        if(variable.m_ShapeID==ShapeID::GlobalValue) msg = "GlobalValue";
        if(variable.m_ShapeID==ShapeID::GlobalArray) msg = "GlobalArray";
        if(variable.m_ShapeID==ShapeID::JoinedArray) msg = "JoinedArray";
        if(variable.m_ShapeID==ShapeID::LocalArray) msg = "LocalArray";
        if(variable.m_ShapeID==ShapeID::LocalValue) msg = "LocalValue";
        std::cout << "\n\t.m_ShapeID="<< msg;
        
        std::cout \
            << "\n\t.m_ElementSize=" << variable.m_ElementSize 
            << "\n\t.m_ConstantDims=" << (variable.m_ConstantDims?"true":"false")
            << "\n\t.m_SingleValue=" << (variable.m_SingleValue ?"true":"false");
        
        std::cout << "\n\t.m_Shape=";
        Dims shape = variable.m_Shape;
        for (auto d: shape) std::cout << d << ",";
        
        std::cout << "\n\t.m_Start=";
        Dims start = variable.m_Start;
        for (auto d: start) std::cout << d << ",";
    
        std::cout << "\n\t.m_Count=";
        Dims count = variable.m_Count;
        for (auto d: count) std::cout << d << ",";
        std::cout << "\n\t.m_IndexStepBlockStarts:keys=";
        
        for(auto it:variable.m_IndexStepBlockStarts) 
        {
            std::cout << it.first << ",";
        }

        std::cout << "\n\t.m_AvailableStepsStart=" << 
            variable.m_AvailableStepsStart;
        
        std::cout << "\n\t.m_AvailableStepsCount=" << 
            variable.m_AvailableStepsCount;
        
        std::cout << "\n\t.m_StepsStart=" << 
            variable.m_StepsStart;
        
        std::cout << "\n\t.m_StepsCount=" << 
            variable.m_StepsCount;
        
        std::cout << "\n\t.TotalSize(num_elements)=" << 
            variable.TotalSize();
        
        std::cout << "\n\t.PayloadSize=" << 
            variable.PayloadSize();
        
        PrintVarData(variable);
        std::cout << std::endl;
}

// for printing a single templated value.
template<typename T>
void CephWriter::printVal(T val)
{
    std::cout << val << ',';
    
    std::boolalpha;    
    //~ bool t = false;
    //~ t = std::is_integral<T>::value;
    //~ std::cout << "(int?" << (t==true?"true":"false") << ")";
    //~ t = std::is_floating_point<T>::value;
    //~ std::cout << "(float?" << (t==true?"true":"false") << ")";
    //~ t = std::is_array<T>::value;
    //~ std::cout << "(array?" << (t==true?"true":"false") << ")";
}

// Prints out the variable's elements from var data[] and bufferlist.
// TODO: add these DIMS type.<< ": m_Count=" << variable.m_Count
template <class T>
void CephWriter::PrintVarData(Variable<T> &variable)
{ 
    librados::bufferlist& bl = *m_Buffs.at(variable.m_Name);
    std::cout << "\n\tbl addr=" << &bl << "\n\tbl.length=" << bl.length() << "\n\tbl  data=";

    // variable details
    const int paysize = variable.PayloadSize();
    const int num_elems = variable.TotalElems();
    const int elemsize = variable.m_ElementSize;
    
    // used to access elems in var data[] or bufferlist
    const char *ptr;
    
    // print out bufferlist elems
    ptr = bl.c_str();
    if(ptr &&  bl.length() > 0)
    {
        for (int i = 0; i < num_elems; i++, ptr+=elemsize)
        {
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
                std::cout << "unhandled type <" << variable.m_Type << "> for variable name " << variable.m_Name;
            }
        }
    }
    else 
    {
        std::cout << "empty";
    }

    
    // print out var data[] elems
    std::cout << "\n\tvar payload size=" << paysize  << "\n\tnum_elems=" << num_elems << "\n\tvar data=";
    ptr = (const char*)variable.GetData();
    if(ptr && variable.PayloadSize() > 0)
    {
        for (int i = 0; i < num_elems; i++, ptr+=elemsize)
        {
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
                std::cout << "unhandled type <" << variable.m_Type << "> for variable name " << variable.m_Name;
            }
        }
    }
    else 
    {
        std::cout << "empty";
    }
    std::cout << std::endl;
}

template <class T>
void CephWriter::PutSyncCommon(Variable<T> &variable, const T *values)
{
    if (m_DebugMode)
    {
        std::cout << "\nCephWriter::PutSyncCommon:BEGIN:rank("  << m_WriterRank 
            << ") PrintVarInfo:";
        
        PrintVarInfo(variable);
        std::cout << "\n";
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
    
    
// variable details
const int paysize = variable.PayloadSize();
const int num_elems = variable.TotalElems();
const int elemsize = variable.m_ElementSize;
    
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
            std::cout << "CephWriter::PutSyncCommon:MIDDLE:rank("  << m_WriterRank 
                    << "): oid=" << oid << "; varname=" << variable.m_Name 
                    << "; m_Buffs.at(" << variable.m_Name << " ) addr=" 
                    << m_Buffs.at(variable.m_Name) << "; ts=" << currentStep;
            
                    //PrintVarData(" ", variable, *m_Buffs.at(variable.m_Name));
                    PrintVarInfo(variable); 
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
