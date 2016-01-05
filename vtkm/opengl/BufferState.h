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
#ifndef vtk_m_opengl_BufferState_h
#define vtk_m_opengl_BufferState_h

//gl headers needs to be buffer anything to do with buffer's
#include <vtkm/opengl/internal/OpenGLHeaders.h>
#include <vtkm/opengl/internal/BufferTypePicker.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/smart_ptr/scoped_ptr.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm{
namespace opengl{


namespace internal
{
  /// \brief Device backend and opengl interop resources management
  ///
  /// \c TransferResource manages a context for a given device backend and a
  /// single OpenGL buffer as efficiently as possible.
  ///
  /// Default implementation is a no-op
  class TransferResource
  {
  public:
    virtual ~TransferResource() {}
  };
}

/// \brief Manages the state for transferring an ArrayHandle to opengl.
///
/// \c BufferState holds all the relevant data information for a given ArrayHandle
/// mapping into OpenGL. Reusing the state information for all renders of an
/// ArrayHandle will allow for the most efficient interop between backends and
/// OpenGL ( especially for CUDA ).
///
///
/// The interop code in vtk-m uses a lazy buffer re-allocation.
///
class BufferState
{
public:
  /// Construct a BufferState using an existing GLHandle
  BufferState(GLuint& gLHandle):
    OpenGLHandle(&gLHandle),
    BufferType(GL_INVALID_VALUE),
    SizeOfActiveSection(0),
    CapacityOfBuffer(0),
    DefaultGLHandle(0),
    Resource(NULL)
  {
  }

  /// Construct a BufferState using an existing GLHandle and type
  BufferState(GLuint& gLHandle, GLenum type):
    OpenGLHandle(&gLHandle),
    BufferType(type),
    SizeOfActiveSection(0),
    CapacityOfBuffer(0),
    DefaultGLHandle(0),
    Resource(NULL)
  {
  }

  BufferState():
    OpenGLHandle(NULL),
    BufferType(GL_INVALID_VALUE),
     SizeOfActiveSection(0),
     CapacityOfBuffer(0),
     DefaultGLHandle(0),
     Resource(NULL)
  {
    this->OpenGLHandle = &this->DefaultGLHandle;
  }

  ~BufferState()
  {
    //don't delete this as it points to user memory, or stack allocated
    //memory inside this object instance
    this->OpenGLHandle = NULL;
  }

  /// \brief get the OpenGL buffer handle
  ///
  GLuint* GetHandle() const { return this->OpenGLHandle; }

  /// \brief return if this buffer has a valid OpenGL buffer type
  ///
  bool HasType() const { return this->BufferType != GL_INVALID_VALUE; }

  /// \brief return what OpenGL buffer type we are bound to
  ///
  /// will return GL_INVALID_VALUE if we don't have a valid type set
  GLenum GetType() const { return this->BufferType; }

  /// \brief Set what type of OpenGL buffer type we should bind as
  ///
  void SetType(GLenum type) { this->BufferType = type; }

  /// \brief deduce the buffer type from the template value type that
  /// was passed in, and set that as our type
  ///
  /// Will be GL_ELEMENT_ARRAY_BUFFER for
  /// vtkm::Int32, vtkm::UInt32, vtkm::Int64, vtkm::UInt64, vtkm::Id, and vtkm::IdComponent
  /// will be GL_ARRAY_BUFFER for everything else.
  template<typename T>
  void DeduceAndSetType(T t)
    { this->BufferType = vtkm::opengl::internal::BufferTypePicker(t); }

  /// \brief Get the size of the buffer in bytes
  ///
  /// Get the size of the active section of the buffer
  ///This will always be <= the capacity of the buffer
  vtkm::Int64 GetSize() const { return this->SizeOfActiveSection; }

  //Set the size of buffer in bytes
  //This will always needs to be <= the capacity of the buffer
  //Note: This call should only be used internally by vtk-m
  void SetSize(vtkm::Int64 size) { this->SizeOfActiveSection = size; }

  /// \brief Get the capacity of the buffer in bytes
  ///
  /// The buffers that vtk-m allocate in OpenGL use lazy resizing. This allows
  /// vtk-m to not have to reallocate a buffer while the size stays the same
  /// or shrinks. This allows allows the cuda to OpenGL to perform significantly
  /// better as we than don't need to call cudaGraphicsGLRegisterBuffer as
  /// often
  vtkm::Int64 GetCapacity() const { return this->CapacityOfBuffer; }

  // Helper function to compute when we should resize  the capacity of the
  // buffer
  bool ShouldRealloc(vtkm::Int64 desiredSize) const
  {
    const bool haveNotEnoughRoom = this->GetCapacity() < desiredSize;
    const bool haveTooMuchRoom = this->GetCapacity() > (desiredSize*2);
    return (haveNotEnoughRoom || haveTooMuchRoom);
  }

  //Set the capacity of buffer in bytes
  //The capacity of a buffer can be larger than the active size of buffer
  //Note: This call should only be used internally by vtk-m
  void SetCapacity(vtkm::Int64 capacity) { this->CapacityOfBuffer = capacity; }

  //Note: This call should only be used internally by vtk-m
  vtkm::opengl::internal::TransferResource* GetResource()
    { return this->Resource.get(); }

  //Note: This call should only be used internally by vtk-m
  void SetResource( vtkm::opengl::internal::TransferResource* resource)
    { this->Resource.reset(resource); }


private:
  //explicitly state the BufferState doesn't support copy or move semantics
  BufferState(const BufferState&);
  void operator=(const BufferState&);

  GLuint* OpenGLHandle;
  GLenum BufferType;
  vtkm::Int64 SizeOfActiveSection; //must be Int64 as size can be over 2billion
  vtkm::Int64 CapacityOfBuffer; //must be Int64 as size can be over 2billion
  GLuint DefaultGLHandle;
  boost::scoped_ptr<vtkm::opengl::internal::TransferResource> Resource;
};

}}

#endif //vtk_m_opengl_BufferState_h