//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleAny_cxx
#define vtk_m_cont_ArrayHandleAny_cxx

#include <vtkm/cont/ArrayHandleAny.hxx>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>

namespace vtkm
{
namespace cont
{
// clang-format off
template class VTKM_CONT_TEMPLATE_EXPORT StorageAny<vtkm::UInt8,       vtkm::cont::StorageTagBasic>;
template class VTKM_CONT_TEMPLATE_EXPORT StorageAny<vtkm::IdComponent, vtkm::cont::StorageTagBasic>;
template class VTKM_CONT_TEMPLATE_EXPORT StorageAny<vtkm::Id,          vtkm::cont::StorageTagBasic>;


// template class VTKM_CONT_TEMPLATE_EXPORT StorageAny< vtkm::Vec<float,3>,  vtkm::cont::StorageTagBasic>;
// template class VTKM_CONT_TEMPLATE_EXPORT StorageAny< vtkm::Vec<double,3>, vtkm::cont::StorageTagBasic>;
// template class VTKM_CONT_TEMPLATE_EXPORT StorageAny<vtkm::Vec<vtkm::FloatDefault, 3>,
//                                                                           vtkm::cont::StorageTagImplicit<vtkm::internal::ArrayPortalUniformPointCoordinates>>;
// clang-format on
}
}

#endif
