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
//#include "adios2/toolkit/format/bp3/BP3.h"
#include "adios2/toolkit/transportman/TransportMan.h" //transport::TransportsMan
//#include "adios2/toolkit/interop/ceph/CephCommon.h"

namespace adios2
{

const unsigned int CEPH_CONF_OBJ_SZ = 10;  // MB
typedef struct _ObjStream *ObjStream;


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
    void PerformPuts() final;
    void EndStep() final;


private:
    /** Single object controlling BP buffering */
    // format::BP3Serializer m_BP3Serializer;

    int m_Verbosity = 0;
    int m_WriterRank = -1;       // my rank in the writers' comm
    int m_CurrentStep = -1; // steps start from 0

    // EndStep must call PerformPuts if necessary
    bool m_NeedPerformPuts = false;

    void Init() final;
    void InitParameters() final;
    void InitTransports() final;
    void InitBuffer();  // used in BPWriter but not part of engine class.

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
    
    void DoClose(const int transportIndex = -1) final;

};

} // end namespace adios2

#endif /* ADIOS2_ENGINE_CEPH_CEPHWRITER_H_ */
