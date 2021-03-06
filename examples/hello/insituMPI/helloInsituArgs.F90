module helloInsituArgs
    implicit none
    character(len=256), parameter :: streamname = "insitu_stream"
    ! arguments
    character(len=256) :: xmlfile
    integer :: npx, npy    ! # of processors in x-y direction
    integer :: ndx, ndy    ! size of array per processor (without ghost cells)
    integer :: steps       ! number of steps to write
    integer :: sleeptime   ! wait time between steps in seconds
    
contains

!!***************************
subroutine usage(isWriter)
    logical, intent(in) :: isWriter 
    if (isWriter) then 
        print *, "Usage: helloInsituMPIWriter  config  N  M   nx  ny   steps  sleeptime"
    else
        print *, "Usage: helloInsituMPIReader  config  N  M"
    endif
    print *, " "
    print *, "config:    path of XML config file"
    print *, "N:         number of processes in X dimension"
    print *, "M:         number of processes in Y dimension"
    if (isWriter) then 
        print *, "nx:        local array size in X dimension per processor"
        print *, "ny:        local array size in Y dimension per processor"
        print *, "steps:     the total number of steps to output" 
        print *, "sleeptime: wait time between steps in seconds"
    endif
end subroutine usage 
    
!!***************************
subroutine processArgs(rank, nproc, isWriter)

#if defined(_CRAYFTN) || !defined(__GFORTRAN__) && !defined(__GNUC__)
    interface
         integer function iargc()
         end function iargc
    end interface
#endif

    integer, intent(in) :: rank
    integer, intent(in) :: nproc
    logical, intent(in) :: isWriter 
    character(len=256) :: npx_str, npy_str, ndx_str, ndy_str
    character(len=256) :: steps_str,time_str
    integer :: numargs, expargs

    !! expected number of arguments
    if (isWriter) then 
        expargs = 7
    else 
        expargs = 3
    endif

    !! process arguments
    numargs = iargc()
    !print *,"Number of arguments:",numargs
    if ( numargs < expargs ) then
        call usage(isWriter)
        call exit(1)
    endif
    call getarg(1, xmlfile)
    call getarg(2, npx_str)
    call getarg(3, npy_str)
    if (isWriter) then 
        call getarg(4, ndx_str)
        call getarg(5, ndy_str)
        call getarg(6, steps_str)
        call getarg(7, time_str)
    endif
    read (npx_str,'(i5)') npx
    read (npy_str,'(i5)') npy
    if (isWriter) then 
        read (ndx_str,'(i6)') ndx
        read (ndy_str,'(i6)') ndy
        read (steps_str,'(i6)') steps
        read (time_str,'(i6)') sleeptime
    endif

    if (npx*npy /= nproc) then
        print *,"ERROR: N*M must equal the number of processes"
        call usage(isWriter)
        call exit(1)
    endif

end subroutine processArgs

    
end module helloInsituArgs 

