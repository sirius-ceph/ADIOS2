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
#include <utility>

namespace adios2
{

template <class T>
void CephWriter::PrintVarInfo(Variable<T> &variable) 
{       
    std::cout << "(" << variable.m_Name << ")"
        << "\n\tvar.m_Name=" << variable.m_Name 
        << "\n\tvar.m_Type=" << variable.m_Type
        << "\n\tvar.m_Min=" << variable.m_Min
        << "\n\tvar.m_Max=" << variable.m_Max
        << "\n\tvar.m_Value=" << variable.m_Value;

    std::string msg = "";
    if(variable.m_ShapeID==ShapeID::GlobalValue) msg = "GlobalValue";
    if(variable.m_ShapeID==ShapeID::GlobalArray) msg = "GlobalArray";
    if(variable.m_ShapeID==ShapeID::JoinedArray) msg = "JoinedArray";
    if(variable.m_ShapeID==ShapeID::LocalArray) msg = "LocalArray";
    if(variable.m_ShapeID==ShapeID::LocalValue) msg = "LocalValue";
    std::cout << "\n\tvar.m_ShapeID="<< msg;
    
    std::cout \
        << "\n\tvar.m_ElementSize=" << variable.m_ElementSize 
        << "\n\tvar.m_ConstantDims=" << (variable.m_ConstantDims?"true":"false")
        << "\n\tvar.m_SingleValue=" << (variable.m_SingleValue ?"true":"false");
    
    std::cout << "\n\tvar.m_Shape=";
    Dims shape = variable.m_Shape;
    for (auto d: shape) std::cout << d << ",";
    
    std::cout << "\n\tvar.m_Start=";
    Dims start = variable.m_Start;
    for (auto d: start) std::cout << d << ",";

    std::cout << "\n\tvar.m_Count=";
    Dims count = variable.m_Count;
    for (auto d: count) std::cout << d << ",";
    std::cout << "\n\tvar.m_IndexStepBlockStarts:keys=";
    
    for(auto it:variable.m_IndexStepBlockStarts) 
    {
        std::cout << it.first << ",";
    }

    std::cout << "\n\tvar.m_AvailableStepsStart=" << 
        variable.m_AvailableStepsStart;
    
    std::cout << "\n\tvar.m_AvailableStepsCount=" << 
        variable.m_AvailableStepsCount;
    
    std::cout << "\n\tvar.m_StepsStart=" << 
        variable.m_StepsStart;
    
    std::cout << "\n\tvar.m_StepsCount=" << 
        variable.m_StepsCount;
    
    std::cout << "\n\tvar.TotalSize(num_elements)=" << 
        variable.TotalSize();
    
    std::cout << "\n\tvar.PayloadSize=" << 
        variable.PayloadSize();
    
    
    TypeInfo<T> ti;
    librados::bufferlist bl = *(m_BuffsIdx[m_CurrentStep]->at(variable.m_Name));    
    
    PrintVariableVals(variable.TotalSize(), variable.GetData());
    PrintBufferlistVals(ti, bl); 
    //BLTesting(ti, bl); 
    
    std::cout << std::endl;
}


void CephWriter::SetMinMax(Variable<std::string> &variable, const std::string& val) 
{
    if(variable.m_Value.size() == 0)
    {
        variable.m_Value.append(val.data());
        variable.m_Min.append(val.data());
        variable.m_Max.append(val.data());
        
        // we manually reset this just to avoid ptr inconsitent state later with 
        // string types due to payload size (element size) issues.
        variable.SetData(&variable.m_Value);   

        // reset elemsize to correct strlen, due to above str ptr size issue
        variable.m_ElementSize = variable.m_Value.size();
    }
}

template < class T, class U > 
void CephWriter::SetMinMax(Variable<T> &variable, const U& val ) 
{ 
    if(variable.m_SingleValue) 
    {
        variable.m_Value = variable.m_Min = variable.m_Max = val;
    }
    else 
    {
        if(val<variable.m_Min) variable.m_Min= val; 
        if(val>variable.m_Max) variable.m_Max= val;
    }
}

template <class T>
void CephWriter::CheckMinMax(Variable<T> &variable)
{
    const T* p = variable.GetData();
    if(p)
    {
        if(variable.m_SingleValue)
        {
            SetMinMax(variable, *p);
        }
        else 
        {
            for (int i = 0; i < variable.TotalSize(); i++, p++)
            { 
                SetMinMax(variable, *p);
            }
        }
    }
    std::cout << std::endl;
}


template <class T>
void CephWriter::AppendBufferlist(Variable<T> &variable)
{
    // our template functions are overloaded, so no chance of getting here 
    // with a complex type variable.
    const char* p = (const char*)variable.GetData();  
    if (p) 
    {
        m_BuffsIdx[m_CurrentStep]->at(variable.m_Name)->append(p, variable.PayloadSize());
    }
}

void CephWriter::AppendBufferlist(Variable<std::string> &variable)
{
    // Special case for string var types.
    const char* p = (const char*)variable.GetData();  
    if (p) 
    {    
        m_BuffsIdx[m_CurrentStep]->at(variable.m_Name)->append(variable.m_Value.c_str(), variable.m_Value.size());
    }
}

template <class T>
void CephWriter::PrintVariableVals(size_t num_elems, const T* p)
{ 
    if(p)
    {
        std::cout << "\n\tvar.GetData=";
        for (int i = 0; i < num_elems ; i++, p++)
            std::cout << *p << ",";
    }
    else 
    {
        std::cout << "\n\tvar.data is empty";
    }
}

template <class T>
void CephWriter::PrintBufferlistVals(const TypeInfo<T>, librados::bufferlist  &bl)
{ 
    // print out elems in bufferlist
    std::cout << "\n\tbl.length=" << bl.length();
    
    // use a bufferlist iterator and we treat string type as separate case.
    // since adios2 string type is always value sized as str ptr,
    // so we cannot iterate over the bufferlist.
    if(std::is_same<T, std::string>::value)
    {
        const std::string s(bl.c_str(), bl.length());
        std::cout << "\n\tbl.data=" << s;  
    } 
    else 
    {
        std::cout << "\n\tbl.data=";
        librados::bufferlist::iterator it = bl.begin();   
        size_t pos = 0;
        
        // this is also not safe, and should catch end of buffer error.
        while (it != bl.end()) 
        {
            const char* p;
            pos += it.get_ptr_and_advance(sizeof(T), &p);
            const T* val = reinterpret_cast<const T*>(p);
            std::cout << *val << ", ";
        }   
    }
    std::cout << std::endl;
}

template <class T> 
void CephWriter::BLTesting(const TypeInfo<T> ti, librados::bufferlist& bl)
{
    if (bl.length() == 0) return;
    std::cout << "START BL Copy Testing\n";   
    int total_elems = bl.length()/sizeof(T);
    int len = bl.length()/2;
    librados::bufferlist::iterator itr(&bl);
    
    // copy via bl.copy
    librados::bufferlist out;
    bl.copy(0, len, out);
    std::cout << "\n\tcopy half via bl.copy:";
    PrintBufferlistVals(ti, out);

    // copy shallow via itr and bufferptr.   
    librados::bufferlist copy;   
    itr.seek(0);  //  itr.get_off()
    ceph::bufferptr bufptr = itr.get_current_ptr(); //   ceph::bufferptr bufptr(len);   
    itr.copy_shallow(len, bufptr);  
    copy.append(bufptr);
    std::cout << "\n\tcopy first half via itr.copy_shallow(len, bufptr):";  
    PrintBufferlistVals(ti, copy);  
    
    // copy shallow via itr and bufferptr. 
    librados::bufferlist copy2;
    ceph::bufferptr ptr = itr.get_current_ptr();
    std::cout << " itr.get_off()=" <<  itr.get_off() << std::endl;
    itr.copy_shallow(len, ptr);
    copy2.append(ptr);
    std::cout << "\n\tcopy second half via itr.copy_shallow(len, bufptr): copy2";  
    PrintBufferlistVals(ti, copy2); 
    
    // copy all
    librados::bufferlist copy3;
    itr = bl.begin();
    itr.copy_all(copy3);
    std::cout << "\n\tcopy_all to copy3";  
    PrintBufferlistVals(ti, copy3); 
    
    // copy both halves via bufferptrs
{
    librados::bufferlist copy;
    ceph::bufferptr ptr1(len), ptr2(len);
    librados::bufferlist::iterator itr(&bl);
    itr.seek(0);
    itr.copy_shallow(len, ptr1);
    int offset = itr.get_off();
    itr.seek(0);
    itr.seek(offset);
    itr.seek(0);
    itr.advance(offset);
    itr.copy_shallow(len,ptr2);
    copy.append(ptr1);
    copy.append(ptr2);
    std::cout << "\n\tcopy both halves via 2 bufferptrs";   
    PrintBufferlistVals(ti, copy); 
}
    
    std::cout << "END BL Copy Testing!!!\n";    
}


template <class T>
void CephWriter::PutSyncCommon(Variable<T> &variable, const T *values)
{   
    const size_t currentStep = CurrentStep();    
    const int varVersion = 0; // will be used later with EMPRESS
    
    // TODO: get actual Dims per variable.
    std::vector<int> dimOffsets = {0,0,0};
    
    // always add the data to the variable, can this ever be null?
    if(values != nullptr)     
        variable.SetData(values); 
    
    // need to keep track of ongoing min max for this var, per obj, and global.
    CheckMinMax(variable); 
    
    librados::bufferlist bl = *(m_BuffsIdx[m_CurrentStep]->at(variable.m_Name));
    
    // TODO: get remaining bytes in buffer for this variable.
    // we should try to allocate new bufferlist or bufptr of size=var.PayloadSize()
    // then append/claim that to the actual bufferlist.
    const int BUF_SZ_AVAIL = adios2::DefaultMaxBufferSize;
    if ((BUF_SZ_AVAIL - bl.length()) > variable.PayloadSize())
    {
        AppendBufferlist(variable); 
        
        // now we clear the var data so its not re-added to bufferlist
        // this is appropriate since variable is not the place for data storage
        variable.SetData(nullptr);
    }
    
    if (currentStep % m_FlushStepsCount == 0)  // prescribed by EMPRESS
    {
        //FlushObjData();
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
            std::cout << "oid=" << oid << std::endl;
            std::cout << "\nCephWriter::PutSyncCommon:rank("  << m_WriterRank 
                    << ") ts=: " << currentStep << ";  ";
            PrintVarInfo(variable); 
            std::cout << std::endl;
        }

#ifdef USE_CEPH_OBJ_TRANS
        transport->Write(oid, bl);
#endif /* USE_CEPH_OBJ_TRANS */
        
        //bl.clear();
    }

}

template <class T>
void CephWriter::PutDeferredCommon(Variable<T> &variable, const T *values)
{
    // always add the data to the variable, can this ever be null?
    if(values != nullptr)     
        variable.SetData(values); 
    
    // need to keep track of ongoing min max for this var, per obj, and global.
    CheckMinMax(variable); 
    
    librados::bufferlist bl = *(m_BuffsIdx[m_CurrentStep]->at(variable.m_Name));
    
    // TODO: get remaining bytes in buffer for this variable.
    // we should try to allocate new bufferlist or bufptr of size=var.PayloadSize()
    // then append/claim that to the actual bufferlist.
    const int BUF_SZ_AVAIL = adios2::DefaultMaxBufferSize;
    if ((BUF_SZ_AVAIL - bl.length()) > variable.PayloadSize())
    {
        AppendBufferlist(variable); 
        
        // now we clear the var data so its not re-added to bufferlist
        // this is appropriate since variable is not the place for data storage
        variable.SetData(nullptr);
    }

    m_NeedPerformPuts = true;

    if (m_DebugMode)
    {
        std::cout << "CephWriter::PutDeferredCommon:rank("  << m_WriterRank 
                << ") variable.m_Name=" << variable.m_Name << std::endl;
    }
}

} // end namespace adios2

#endif /* ADIOS2_ENGINE_CEPH_CEPHWRITER_TCC_ */
