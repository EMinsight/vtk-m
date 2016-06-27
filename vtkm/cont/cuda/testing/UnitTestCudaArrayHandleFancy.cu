//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifdef VTKM_DEVICE_ADAPTER
#undef VTKM_DEVICE_ADAPTER
#endif
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_ERROR

#ifndef BOOST_SP_DISABLE_THREADS
#define BOOST_SP_DISABLE_THREADS
#endif

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>

#include <vtkm/cont/testing/TestingFancyArrayHandles.h>
#include <vtkm/cont/cuda/internal/testing/Testing.h>

int UnitTestCudaArrayHandleFancy(int, char *[])
{
  int result = vtkm::cont::testing::TestingFancyArrayHandles
      <vtkm::cont::DeviceAdapterTagCuda>::Run();
  return vtkm::cont::cuda::internal::Testing::CheckCudaBeforeExit(result);
}

