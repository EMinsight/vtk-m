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
#ifndef vtk_m_internal_ArrayPortalVirtual_h
#define vtk_m_internal_ArrayPortalVirtual_h


#include <vtkm/VecTraits.h>
#include <vtkm/VirtualObjectBase.h>
#include <vtkm/internal/ExportMacros.h>

namespace vtkm
{
namespace internal
{

class VTKM_ALWAYS_EXPORT PortalVirtualBase
{
public:
  VTKM_EXEC_CONT PortalVirtualBase() {}

  VTKM_EXEC_CONT virtual ~PortalVirtualBase(){
    //we implement this as we need a destructor with cuda markup.
    //Using =default causes cuda free errors inside VirtualObjectTransferCuda
  };
};

template<typename PortalType>
struct PortalSupportsGets
{
  template<typename U, typename S = decltype(std::declval<U>().Get(vtkm::Id{}))>
  static std::true_type has(int);
  template<typename U>
  static std::false_type has(...);
  using type = decltype(has<PortalType>(0));
};

template<typename PortalType>
struct PortalSupportsSets
{
  template<typename U,
           typename S = decltype(std::declval<U>().Set(vtkm::Id{},
                                                       std::declval<typename U::ValueType>()))>
  static std::true_type has(int);
  template<typename U>
  static std::false_type has(...);
  using type = decltype(has<PortalType>(0));
};

} // namespace internal

template<typename T>
class VTKM_ALWAYS_EXPORT ArrayPortalVirtual : public internal::PortalVirtualBase
{
public:
  using ValueType = T;

  //use parents constructor
  using PortalVirtualBase::PortalVirtualBase;

  VTKM_EXEC_CONT virtual ~ArrayPortalVirtual<T>() = default;

  VTKM_EXEC_CONT virtual T Get(vtkm::Id index) const = 0;

  VTKM_EXEC_CONT virtual void Set(vtkm::Id, const T&) const {}
};


template<typename PortalT>
class VTKM_ALWAYS_EXPORT ArrayPortalWrapper final
  : public vtkm::ArrayPortalVirtual<typename PortalT::ValueType>
{
  using T = typename PortalT::ValueType;

public:
  ArrayPortalWrapper(const PortalT& p)
    : ArrayPortalVirtual<T>()
    , Portal(p)
  {
  }

  VTKM_EXEC
  T Get(vtkm::Id index) const
  {
    using call_supported_t = typename internal::PortalSupportsGets<PortalT>::type;
    return this->Get(call_supported_t(), index);
  }

  VTKM_EXEC
  void Set(vtkm::Id index, const T& value) const
  {
    using call_supported_t = typename internal::PortalSupportsSets<PortalT>::type;
    this->Set(call_supported_t(), index, value);
  }

private:
  // clang-format off
  VTKM_EXEC inline T Get(std::true_type, vtkm::Id index) const { return this->Portal.Get(index); }
  VTKM_EXEC inline T Get(std::false_type, vtkm::Id) const { return T{}; }
  VTKM_EXEC inline void Set(std::true_type, vtkm::Id index, const T& value) const { this->Portal.Set(index, value); }
  VTKM_EXEC inline void Set(std::false_type, vtkm::Id, const T&) const {}
  // clang-format on


  PortalT Portal;
};


template<typename T>
class VTKM_ALWAYS_EXPORT ArrayPortalRef
{
public:
  using ValueType = T;

  ArrayPortalRef()
    : Portal(nullptr)
    , NumberOfValues(0)
  {
  }

  ArrayPortalRef(const ArrayPortalVirtual<T>* portal, vtkm::Id numValues)
    : Portal(portal)
    , NumberOfValues(numValues)
  {
  }

  //Currently this needs to be valid on both the host and device for cuda, so we can't
  //call the underlying portal as that uses device virtuals and the method will fail.
  //We need to seriously look at the interaction of portals and iterators for device
  //adapters and determine a better approach as iterators<Portal> are really fat
  VTKM_EXEC_CONT inline vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  //This isn't valid on the host for cuda
  VTKM_EXEC_CONT inline T Get(vtkm::Id index) const { return this->Portal->Get(index); }

  //This isn't valid on the host for
  VTKM_EXEC_CONT inline void Set(vtkm::Id index, const T& t) const { this->Portal->Set(index, t); }

  const ArrayPortalVirtual<T>* Portal;
  vtkm::Id NumberOfValues;
};

template<typename T>
inline ArrayPortalRef<T> make_ArrayPortalRef(const ArrayPortalVirtual<T>* portal,
                                             vtkm::Id numValues)
{
  return ArrayPortalRef<T>(portal, numValues);
}


} // namespace vtkm

#endif
