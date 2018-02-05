/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 *  Created on: 
 *      Author: 
 *   adapted from HelloSkeletonArgs.h
 */

#ifndef HELLOCEPHARGS_H_
#define HELLOCEPHARGS_H_

#include <string>

class HelloCephArgs
{
public:
    HelloCephArgs(bool isWriter, int argc, char *argv[], int rank,
                      int nproc);

    const std::string streamname = "ceph_writer";

    // user arguments
    // ex: ./bin/hello_skeletonWriter ../source/examples/hello/skeleton/hello_skeleton.xml  1 1 3 6  4  500
    std::string configfile;
    unsigned int npx;       // Number of processes in X (slow) dimension
    unsigned int npy;       // Number of processes in Y (fast) dimension
    unsigned int ndx;       // Local array size in X dimension per process
    unsigned int ndy;       // Local array size in y dimension per process
    unsigned int steps;     // Number of output steps
    unsigned int sleeptime; // Time to wait between steps (in millisec)

    // calculated values from those arguments and number of processes
    unsigned int gndx; // Global array size in slow dimension
    unsigned int gndy; // Global array size in fast dimension
    // X dim positions: rank 0, npx, 2npx... are in the same X position
    // Y dim positions: npx number of consecutive processes belong to one row
    // (npx
    // columns)
    unsigned int posx;  // Position of this process in X dimension
    unsigned int posy;  // Position of this process in Y dimension
    unsigned int offsx; // Offset of local array in X dimension on this process
    unsigned int offsy; // Offset of local array in Y dimension on this process

    int rank;           // MPI rank
    unsigned int nproc; // number of processors

    void DecomposeArray(int NX, int NY);
};

#endif /* HELLOCEPHARGS_H_ */
