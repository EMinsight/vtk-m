##=============================================================================
##
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2017 UT-Battelle, LLC.
##  Copyright 2017 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##
##=============================================================================

#Currently all we are going to build is a set of options that are possible
#based on the compiler. For now we are going on the presumption
#that x86 architecture is the only target for vectorization and therefore
#we don't need any system detect.
#
#Here is the breakdown of what each flag type means:
#
#  1. none:
#  Do not explicitly enable vectorization, but at the same don't explicitly disable
#  vectorization.
#
#  2. native:
#  Allow the compiler to use auto-detection based on the systems CPU to determine
#  the highest level of vectorization support that is allowed. This means that
#  libraries and executables built with this setting are non-portable.
#
#  3. avx
#  Compile with just AVX enabled, no AVX2 or AVX512 vectorization will be used.
#  This means that Sandy Bridge, Ivy Bridge, Haswell, and Skylake are supported,
#  but Haswell and newer will not use any AVX2 instructions
#
#  4. avx2
#  Compile with  AVX2/AVX enabled, no AVX512 vectorization will be used.
#  This means that Sandy Bridge, and Ivy Bridge can not run the code.
#
#  5. avx512
#  Compile with AVX512/AVX2/AVX options enabled.
#  This means that Sandy Bridge, Ivy Bridge, Haswell and can not run the code.
#  Only XeonPhi Knights Landing and Skylake processors can run the code.
#
#  AVX512 is designed to mix with avx/avx2 without any performance penalties,
#  so we enable AVX2 so that we get AVX2 support for < 32bit value types which
#  AVX512 has less support for
#
#
# I wonder if we should go towards a per platform cmake include that stores
# all this knowledge
#   include(gcc.cmake)
#   include(icc.cmake)
#   include(clang.cmake)
#
# This way we could also do compile warning flag detection at the same time
#
#
# Note: By default we use 'native' as the default option
#
#

# guard agaisnt building vectorization_flags more than once
if(TARGET vtkm_vectorization_flags)
    return()
endif()

add_library(vtkm_vectorization_flags INTERFACE)
target_link_libraries(vtkm_vectorization_flags INTERFACE vtkm_compiler_flags)

set(vec_levels none native)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    #for now we presume gcc > 4.6
    list(APPEND vec_levels avx)

    #common flags for the avx instructions for the gcc compiler
    set(native_flags -march=native)
    set(avx_flags -mavx)
    set(avx2_flags ${avx_flags} -mf16c -mavx2 -mfma -mlzcnt -mbmi -mbmi2)
    if (CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 4.7 OR
            CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.7)
        #if GNU is less than 4.9 you get avx, avx2
        list(APPEND vec_levels avx2)
    elseif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.1)
        #if GNU is less than 5.1 you get avx, avx2, and some avx512
        list(APPEND vec_levels avx2 avx512-knl)
        set(knl_flags ${avx2_flags} -mavx512f -mavx512pf -mavx512er -mavx512cd)
    else()
        #if GNU is 5.1+ you get avx, avx2, and more avx512
        list(APPEND vec_levels avx2 avx512-skx avx512-knl)
        set(knl_flags ${avx2_flags} -mavx512f -mavx512pf -mavx512er -mavx512cd)
        set(skylake_flags ${avx2_flags} -mavx512f -mavx512dq -mavx512cd -mavx512bw -mavx512vl)
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    list(APPEND vec_levels avx avx2 avx512-skx avx512-knl)
    set(native_flags -march=native)
    set(avx_flags -mavx)
    set(avx2_flags ${avx_flags} -mf16c -mavx2 -mfma -mlzcnt -mbmi -mbmi2)
    set(knl_flags ${avx2_flags} -avx512f -avx512cd -avx512dq -avx512bw -avx512vl)
    set(skylake_flags ${avx2_flags} -avx512f -avx512cd -avx512er -avx512pf)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    #While Clang support AVX512, no version of AppleClang has that support yet
    list(APPEND vec_levels avx avx2)
    set(native_flags -march=native)
    set(avx_flags -mavx)
    set(avx2_flags ${avx_flags} -mf16c -mavx2 -mfma -mlzcnt -mbmi -mbmi2)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "PGI")
    #I can't find documentation to explicitly state the level of vectorization
    #support I want from the PGI compiler
    #so for now we are going to do nothing
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    #Intel 15.X is the first version with avx512
    #Intel 16.X has way better vector generation compared to 15.X though

    set(native_flags -xHost)
    set(avx_flags  -xAVX)
    set(avx2_flags -xCORE-AVX2)

    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 15.0)
        message(STATUS "While Intel ICC 14.0 and lower support #pragma simd")
        message(STATUS "The code that is generated in testing has caused SIGBUS")
        message(STATUS "runtime signals to be thrown. We recommend you upgrade")
        message(STATUS "or disable vectorization.")
    elseif (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16.0)
        list(APPEND vec_levels avx avx2)
    else()
        list(APPEND vec_levels avx avx2 avx512-skx avx512-knl)
        set(knl_flags ${knl_flags} -xMIC-AVX512)
        set(skylake_flags ${skylake_flags} -xCORE-AVX512)
    endif()
endif()

set_property(GLOBAL PROPERTY VTKm_NATIVE_FLAGS ${native_flags})
set_property(GLOBAL PROPERTY VTKm_AVX_FLAGS ${avx_flags})
set_property(GLOBAL PROPERTY VTKm_AVX2_FLAGS ${avx2_flags})

set_property(GLOBAL PROPERTY VTKm_KNL_FLAGS ${knl_flags})
set_property(GLOBAL PROPERTY VTKm_SKYLAKE_FLAGS ${skylake_flags})

# Now that we have set up what levels the compiler lets setup the CMake option
# We use a combo box style property, so that ccmake and cmake-gui have a
# nice interface
#
set(VTKm_Vectorization "none" CACHE STRING "Level of compiler vectorization support")
set_property(CACHE VTKm_Vectorization PROPERTY STRINGS ${vec_levels})
if (NOT ${VTKm_Vectorization} STREQUAL "none")
    set(VTKM_VECTORIZATION_ENABLED "ON")
endif()

#
# Now that we have set up the options, lets setup the compile flags that
# we are going to require.
#
set(flags)
if(VTKm_Vectorization STREQUAL "native")
    get_property(flags GLOBAL PROPERTY VTKm_NATIVE_FLAGS)
elseif(VTKm_Vectorization STREQUAL "avx")
    get_property(flags GLOBAL PROPERTY VTKm_AVX_FLAGS)
elseif(VTKm_Vectorization STREQUAL "avx2")
    get_property(flags GLOBAL PROPERTY VTKm_AVX2_FLAGS)
elseif(VTKm_Vectorization STREQUAL "avx512-skx")
    get_property(flags GLOBAL PROPERTY VTKm_SKYLAKE_FLAGS)
elseif(VTKm_Vectorization STREQUAL "avx512-knl")
    get_property(flags GLOBAL PROPERTY VTKm_KNL_FLAGS)
endif()

#guard against adding the flags multiple times, which happens when multiple
#backends include this file
if(NOT VTKm_Vectorization_flags_added)
    set(VTKm_Vectorization_flags_added true)
    target_compile_options(vtkm_vectorization_flags INTERFACE ${flags})
endif()

install(TARGETS vtkm_vectorization_flags EXPORT ${VTKm_EXPORT_NAME})