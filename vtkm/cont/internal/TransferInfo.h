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
#ifndef vtk_m_cont_internal_TransferInfo_h
#define vtk_m_cont_internal_TransferInfo_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/Types.h>
#include <vtkm/cont/internal/DeviceAdapterTag.h>

#include <memory>

namespace vtkm
{

namespace internal{
class PortalVirtualBase;
}

namespace cont
{
namespace internal
{

template<typename T>
struct VTKM_ALWAYS_EXPORT TransferInfo
{

  bool valid(vtkm::cont::DeviceAdapterId tagValue) const noexcept;

  void updateHost(std::unique_ptr<T>&& host) noexcept;
  void updateDevice(vtkm::cont::DeviceAdapterId id,
                    std::unique_ptr<T>&& host_copy, //NOT the same as host version
                    const T* device,
                    const std::shared_ptr<void>& state) noexcept;
  void releaseDevice();
  void releaseAll();

  const T* hostPtr() const noexcept { return this->Host.get(); }
  const T* devicePtr() const noexcept { return this->Device; }

  std::shared_ptr<void>& state() noexcept { return this->DeviceTransferState; }

private:
  vtkm::cont::DeviceAdapterId DeviceId = vtkm::cont::DeviceAdapterTagUndefined{};
  std::unique_ptr<T> Host = nullptr;
  std::unique_ptr<T> HostCopyOfDevice = nullptr;
  const T* Device = nullptr;
  std::shared_ptr<void> DeviceTransferState = nullptr;
};

extern template struct VTKM_CONT_TEMPLATE_EXPORT TransferInfo<vtkm::internal::PortalVirtualBase>;

using TransferInfoArray = TransferInfo<vtkm::internal::PortalVirtualBase>;
}
}
}

#endif
