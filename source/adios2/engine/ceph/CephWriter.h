/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CephWriter.h
 *
 *  Created on: 
 *      Author: 
 */

#ifndef ADIOS2_ENGINE_CEPH_CEPHWRITER_H_
#define ADIOS2_ENGINE_CEPH_CEPHWRITER_H_

#include "adios2/ADIOSConfig.h"
#include "adios2/core/Engine.h"
#include "adios2/helper/adiosFunctions.h"   // DimsToCSV,
#include "adios2/toolkit/transport/ceph/CephObjTrans.h"

#include <rados/librados.hpp>

namespace adios2
{


class CephWriter : public Engine
{

public:
    /**
     * Constructor for file Writer in ceph object format
     * @param name unique name given to the engine
     * @param openMode w (supported), r, a from OpenMode in ADIOSTypes.h
     * @param mpiComm MPI communicator
     */
    CephWriter(IO &io, const std::string &name, const Mode mode,
                 MPI_Comm mpiComm);

    ~CephWriter();

    StepStatus BeginStep(StepMode mode, const float timeoutSeconds = 0.f) final;
    size_t CurrentStep();
    void PerformPuts() final;
    void EndStep() final;

    // TODO: add moveTier public method to writer engine, remove from IO param.
    // cephWriter.moveTier("VarName", TIER::SLOW)
    // actually this should probabaly be SetTier(var), 

private:
    
    // Engine vars
    int m_Verbosity = 0;
    int m_WriterRank = -1;       // my rank in the writers' comm
    int m_CurrentStep = -1;     // steps start from 0

    // Keeps track of outstading (async) write data <varname, bufferlist ptr>
    std::unordered_map<std::string, librados::bufferlist*> m_Buffs;

    // TODO: we may need this later if deferring writes across timesteps.
    std::map<std::string, std::vector<int>> timeStepsBuffered; 

    // EMPRESS vars
    std::string m_ExpName;
    std::string m_JobId;
    int m_FlushStepsCount = 1;  // default for now
    static std::string GetOid(std::string jobId, std::string expName, int timestep,
            std::string varName, int varVersion, std::vector<int> dimOffsets, int rank);
    
    // only set on DoClose(), to force write during final putsync call.
    bool m_ForceFlush = false;
    bool m_NeedPerformPuts =false;

    void Init() final;
    void InitParameters() final;
    void InitTransports() final;
    void InitBuffer(); 
    void DoClose(const int transportIndex = -1) final;

    // layer that writes to ceph tiers
    // note: we apparently do not need a transport mgr yet.
    std::shared_ptr<transport::CephObjTrans> transport;

#define declare_type(T)                                                        \
    void DoPutSync(Variable<T> &, const T *) final;                            \
    void DoPutDeferred(Variable<T> &, const T *) final;                        \
    void DoPutDeferred(Variable<T> &, const T &) final;
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    /**
     * Common function for primitive PutSync, puts variables in buffer
     * @param variable
     * @param values
     */
    template <class T>
    void PutSyncCommon(Variable<T> &variable, const T *values);

    template <class T>
    void PutDeferredCommon(Variable<T> &variable, const T *values);

    template <class T>
    void PrintVarInfo(Variable<T> &variable); 
        
    template <class T>
    void PrintVariableVals(size_t num_elems, const T* p); 
    void PrintVariableVals(size_t num_elems, const std::complex<float>* p) { /*not supported*/ }
    void PrintVariableVals(size_t num_elems, const std::complex<double>* p) { /*not supported*/ }
    void PrintVariableVals(size_t num_elems,const std::complex<long double>* p) { /*not supported*/ }
     
    template <class T>
    void CheckMinMax(Variable<T> &variable); 
    
    template <class T>
    void AppendBufferlist(Variable<T> &variable);
    void AppendBufferlist(Variable<std::string> &variable);
    void AppendBufferlist(Variable<std::complex<float>> &variable) { /*not supported*/ }
    void AppendBufferlist(Variable<std::complex<double>> &variable) { /*not supported*/ }
    void AppendBufferlist(Variable<std::complex<long double>> &variable) { /*not supported*/ }
    
    template < class T, class U > 
    void SetMinMax(Variable<T> &variable, const U& val );
    void SetMinMax(Variable<std::string> &variable, const std::string& val );
    void SetMinMax(Variable<std::complex<float>> &variable, const std::complex<float>& val ) { /*not supported*/ }
    void SetMinMax(Variable<std::complex<double>> &variable, const std::complex<double>& val ) { /*not supported*/ }
    void SetMinMax(Variable<std::complex<long double>> &variable, const std::complex<long double>& val ) { /*not supported*/ }
    
    
    void TestBL();  // jpl testing only.
    
    
    template <class T>
    void BLTesting(const TypeInfo<T> ti, librados::bufferlist  &bl); // print librados::bufferlist 
    void BLTesting(const TypeInfo<std::complex<float>> ti, librados::bufferlist  &bl) { /*not supported*/ }
    void BLTesting(const TypeInfo<std::complex<double>> ti, librados::bufferlist  &bl) { /*not supported*/ } 
    void BLTesting(const TypeInfo<std::complex<long double>> ti, librados::bufferlist  &bl) { /*not supported*/ } 
    
    template <class T>
    void PrintBufferlistVals(const TypeInfo<T> ti, librados::bufferlist  &bl);
    void PrintBufferlistVals(const TypeInfo<std::complex<float>> ti, librados::bufferlist  &bl) { /*not supported*/ }
    void PrintBufferlistVals(const TypeInfo<std::complex<double>> ti, librados::bufferlist  &bl) { /*not supported*/ } 
    void PrintBufferlistVals(const TypeInfo<std::complex<long double>> ti, librados::bufferlist  &bl) { /*not supported*/ } 

    };
} // end namespace adios2

#endif /* ADIOS2_ENGINE_CEPH_CEPHWRITER_H_ */
