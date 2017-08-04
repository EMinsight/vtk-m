//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 Sandia Corporation.
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/rendering/MapperRayTracer.h>

#include <vtkm/cont/Timer.h>
#include <vtkm/cont/TryExecute.h>

#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/internal/RunTriangulator.h>
#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/RayOperations.h>
#include <vtkm/rendering/raytracing/RayTracer.h>

namespace vtkm
{
namespace rendering
{

struct MapperRayTracer::InternalsType
{
  vtkm::rendering::CanvasRayTracer* Canvas;
  vtkm::rendering::raytracing::RayTracer Tracer;
  vtkm::rendering::raytracing::Camera RayCamera;
  vtkm::rendering::raytracing::Ray<vtkm::Float32> Rays;

  VTKM_CONT
  InternalsType()
    : Canvas(nullptr)
  {
  }
};

MapperRayTracer::MapperRayTracer()
  : Internals(new InternalsType)
{
}

MapperRayTracer::~MapperRayTracer()
{
}

void MapperRayTracer::SetCanvas(vtkm::rendering::Canvas* canvas)
{
  if (canvas != nullptr)
  {
    this->Internals->Canvas = dynamic_cast<CanvasRayTracer*>(canvas);
    if (this->Internals->Canvas == nullptr)
    {
      throw vtkm::cont::ErrorBadValue("Ray Tracer: bad canvas type. Must be CanvasRayTracer");
    }
  }
  else
  {
    this->Internals->Canvas = nullptr;
  }
}

vtkm::rendering::Canvas* MapperRayTracer::GetCanvas() const
{
  return this->Internals->Canvas;
}

void MapperRayTracer::RenderCells(const vtkm::cont::DynamicCellSet& cellset,
                                  const vtkm::cont::CoordinateSystem& coords,
                                  const vtkm::cont::Field& scalarField,
                                  const vtkm::rendering::ColorTable& vtkmNotUsed(colorTable),
                                  const vtkm::rendering::Camera& camera,
                                  const vtkm::Range& scalarRange)
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();
  logger->OpenLogEntry("mapper_ray_tracer");
  vtkm::cont::Timer<> tot_timer;
  vtkm::cont::Timer<> timer;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> indices;
  vtkm::Id numberOfTriangles;

  vtkm::rendering::internal::RunTriangulator(cellset, indices, numberOfTriangles);
  vtkm::Float64 time = timer.GetElapsedTime();
  logger->AddLogData("triangulator", time);
  vtkm::rendering::raytracing::Camera& cam = this->Internals->Tracer.GetCamera();
  cam.SetParameters(camera, *this->Internals->Canvas);
  this->Internals->RayCamera.SetParameters(camera, *this->Internals->Canvas);

  this->Internals->RayCamera.CreateRays(this->Internals->Rays, coords);
  raytracing::RayOperations::MapCanvasToRays(
    this->Internals->Rays, camera, *this->Internals->Canvas);

  vtkm::Bounds dataBounds = coords.GetBounds();

  this->Internals->Tracer.SetData(
    coords.GetData(), indices, scalarField, numberOfTriangles, scalarRange, dataBounds);

  this->Internals->Tracer.SetColorMap(this->ColorMap);
  this->Internals->Tracer.SetBackgroundColor(
    this->Internals->Canvas->GetBackgroundColor().Components);
  this->Internals->Tracer.Render(this->Internals->Rays);

  timer.Reset();
  this->Internals->Canvas->WriteToCanvas(
    this->Internals->Rays, this->Internals->Rays.Buffers.at(0).Buffer, camera);

  time = timer.GetElapsedTime();
  logger->AddLogData("write_to_canvas", time);
  time = tot_timer.GetElapsedTime();
  logger->CloseLogEntry(time);
}

void MapperRayTracer::StartScene()
{
  // Nothing needs to be done.
}

void MapperRayTracer::EndScene()
{
  // Nothing needs to be done.
}

vtkm::rendering::Mapper* MapperRayTracer::NewCopy() const
{
  return new vtkm::rendering::MapperRayTracer(*this);
}
}
} // vtkm::rendering
