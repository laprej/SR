## From https://github.com/rpavlik/cmake-modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules")

PROJECT(ROSS_TOP)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

INCLUDE(CheckTypeSize)
CHECK_TYPE_SIZE("short"         SIZEOF_SHORT       )
CHECK_TYPE_SIZE("int"           SIZEOF_INT         )
CHECK_TYPE_SIZE("float"         SIZEOF_FLOAT       )
CHECK_TYPE_SIZE("double"        SIZEOF_DOUBLE      )
CHECK_TYPE_SIZE("long"          SIZEOF_LONG        )
CHECK_TYPE_SIZE("long int"      SIZEOF_LONG_INT    )
CHECK_TYPE_SIZE("long long"     SIZEOF_LONG_LONG   )
CHECK_TYPE_SIZE("uint"          SIZEOF_UINT        )
CHECK_TYPE_SIZE("void *"        SIZEOF_VPTR        )

# ROSS Configuration Options

# Data Structure for Unprocessed Event List
#SET(QUEUE calendar) #Calendar Queue
#SET(QUEUE heap) #Push/Down Heap
SET(QUEUE splay) #Splay Tree
#SET(QUEUE kp_splay) #Splay Tree in KPs

# Random Library
SET(RAND clcg4)

# Network option
# tcp option removed
SET(NETWORK mpi)

# GVT algorithm option
#SET(GVT 7oclock)
SET(GVT mpi_allreduce)

# Architecture setting and management
SET(VALID_ARCH NO)
SET(ARCH $ENV{ARCH} CACHE STRING "Target architecture")
MARK_AS_ADVANCED(ARCH)
IF(NOT ARCH)
  MESSAGE(FATAL_ERROR "ARCH not defined!\n"
    "Please set ARCH to be one of (i386 | bgl | bgp | x86_64 | ppc64)")
ENDIF(NOT ARCH)

# C Compiler
SET(CC $ENV{CC} CACHE STRING "MPI C Compiler")
MARK_AS_ADVANCED(CC)
IF(NOT CC)
  MESSAGE(WARNING "MPI Compiler, \$CC, not set.")
ENDIF(NOT CC)

# C++ Compiler for CODES
SET(CXX $ENV{CXX} CACHE STRING "MPI C++ Compiler")
MARK_AS_ADVANCED(CXX)

IF(ARCH STREQUAL i386)
  SET(VALID_ARCH YES)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall")
  ADD_DEFINITIONS(-D_GNU_SOURCE)
  SET(CLOCK i386)
ENDIF(ARCH STREQUAL i386)

IF(ARCH STREQUAL bgl)
  SET(VALID_ARCH YES)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -qflag=i:i -qattr=full -O5")
  SET(OPTIONS "-qtune=440 -qarch=440d")
  SET(CLOCK bgl)
ENDIF(ARCH STREQUAL bgl)

IF(ARCH STREQUAL bgp)
  SET(VALID_ARCH YES)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -qflag=i:i -qattr=full -O5")
  SET(OPTIONS "-qtune=450 -qarch=450d")
  SET(CLOCK bgl)
ENDIF(ARCH STREQUAL bgp)

IF(ARCH STREQUAL bgq)
  SET(VALID_ARCH YES)
  OPTION(USE_BGPM "Blue Gene/Q specific Performance Counters" OFF)
  SET(BGPM_INCLUDE /bgsys/drivers/ppcfloor/bgpm/include/)
  ADD_LIBRARY(imp_bgpm STATIC IMPORTED)
  SET_PROPERTY(TARGET imp_bgpm PROPERTY IMPORTED_LOCATION /bgsys/drivers/ppcfloor/bgpm/lib/libbgpm.a)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${BGPM_INCLUDE} -O5 -qstrict -qprefetch=aggressive -qarch=qp -qtune=qp -qmaxmem=-1 -qsimd=noauto -qreport -qhot")
  SET(CLOCK bgq)
ENDIF(ARCH STREQUAL bgq)

IF(ARCH STREQUAL ppc64)
  SET(VALID_ARCH YES)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall")
  ADD_DEFINITIONS(-D_GNU_SOURCE)
  SET(CLOCK ppc)
ENDIF(ARCH STREQUAL ppc64)

IF(ARCH STREQUAL x86_64)
  SET(VALID_ARCH YES)
  SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
  ADD_DEFINITIONS(-D_GNU_SOURCE)
  SET(CLOCK amd64)
ENDIF(ARCH STREQUAL x86_64)

IF(NOT VALID_ARCH)
  MESSAGE(FATAL_ERROR "ARCH not valid!\n"
    "Please set ARCH to be one of (i386 | bgl | bgp | x86_64 | ppc64)")
ENDIF(NOT VALID_ARCH)

# ROSS Core code
ADD_SUBDIRECTORY(core)

# Submodules
OPTION(WITH_ROSS_IO "Build ROSS IO libraries?" OFF)
IF(WITH_ROSS_IO)
  ADD_SUBDIRECTORY(IO)
ENDIF(WITH_ROSS_IO)

# ROSS Models directory
OPTION(ROSS_BUILD_MODELS "Build ROSS models?" OFF)
IF(ROSS_BUILD_MODELS)
  ADD_SUBDIRECTORY(models)
ENDIF(ROSS_BUILD_MODELS)
