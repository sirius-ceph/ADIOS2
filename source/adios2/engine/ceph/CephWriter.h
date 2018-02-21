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


private:
    
    // Engine vars
    int m_Verbosity = 0;
    int m_WriterRank = -1;       // my rank in the writers' comm
    int m_CurrentStep = -1;     // steps start from 0

    // Ceph vars
    int m_CephTargetObjSize = 8388608; // default object size 8MB 
    librados::bufferlist *m_bl = NULL;
    int m_TimestepStart = -1;
    int m_TimestepEnd = -1;

    // EMPRESS vars
    std::string m_ExpName;
    int m_FlushStepsCount = 1;
    std::string Objector(std::string prefix, std::string vars, int rank, 
        int timestepStart, int timestepEnd);

    // EndStep must call PerformPuts if necessary
    bool m_NeedPerformPuts = false;

    void Init() final;
    void InitParameters() final;
    void InitTransports() final {};
    void InitTransports(MPI_Comm mpiComm);
    void InitBuffer(); 

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
    
    void DoClose(const int transportIndex = -1) final;

};

} // end namespace adios2

#endif /* ADIOS2_ENGINE_CEPH_CEPHWRITER_H_ */
