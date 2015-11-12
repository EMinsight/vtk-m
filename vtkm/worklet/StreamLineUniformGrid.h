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

#ifndef vtk_m_worklet_StreamLineUniformGrid_h
#define vtk_m_worklet_StreamLineUniformGrid_h

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/Field.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/exec/ExecutionWholeArray.h>

#include <iostream>
#include <fstream>
#include <math.h>
#include <algorithm>
#include <vector>
#include <unistd.h>

namespace vtkm {

// Take this out when defined in CellShape.h
const vtkm::UInt8 CELL_SHAPE_POLY_LINE = 4;

namespace worklet {

namespace internal {

  enum StreamLineMode
  {
    FORWARD  = 0,
    BACKWARD = 1,
    BOTH     = 2
  };

  // Trilinear interpolation to calculate vector data at position
  template <typename FieldType, typename PortalType>
  VTKM_EXEC_EXPORT
  vtkm::Vec<FieldType, 3> VecDataAtPos(
                                 vtkm::Vec<FieldType, 3> pos, 
                                 const vtkm::Id3 &vdims, 
                                 const vtkm::Id &planesize, 
                                 const vtkm::Id &rowsize, 
                                 const PortalType &vecdata)
  {
    // Adjust initial position to be within bounding box of grid
    for (vtkm::Id d = 0; d < 3; d++)
    {
      if (pos[d] < 0.0f)
        pos[d] = 0.0f;
      if (pos[d] > static_cast<FieldType>(vdims[d] - 1))
        pos[d] = static_cast<FieldType>(vdims[d] - 1);
    }

    // Set the eight corner indices with no wraparound
    vtkm::Id3 idx000, idx001, idx010, idx011, idx100, idx101, idx110, idx111;
    idx000[0] = static_cast<vtkm::Id>(floor(pos[0]));
    idx000[1] = static_cast<vtkm::Id>(floor(pos[1]));
    idx000[2] = static_cast<vtkm::Id>(floor(pos[2]));

    idx001 = idx000; idx001[0] = (idx001[0] + 1) <= vdims[0] - 1 ? idx001[0] + 1 : vdims[0] - 1;
    idx010 = idx000; idx010[1] = (idx010[1] + 1) <= vdims[1] - 1 ? idx010[1] + 1 : vdims[1] - 1;
    idx011 = idx010; idx011[0] = (idx011[0] + 1) <= vdims[0] - 1 ? idx011[0] + 1 : vdims[0] - 1;
    idx100 = idx000; idx100[2] = (idx100[2] + 1) <= vdims[2] - 1 ? idx100[2] + 1 : vdims[2] - 1;
    idx101 = idx100; idx101[0] = (idx101[0] + 1) <= vdims[0] - 1 ? idx101[0] + 1 : vdims[0] - 1;
    idx110 = idx100; idx110[1] = (idx110[1] + 1) <= vdims[1] - 1 ? idx110[1] + 1 : vdims[1] - 1;
    idx111 = idx110; idx111[0] = (idx111[0] + 1) <= vdims[0] - 1 ? idx111[0] + 1 : vdims[0] - 1;

    // Get the vecdata at the eight corners
    vtkm::Vec<FieldType, 3> v000, v001, v010, v011, v100, v101, v110, v111;
    v000 = vecdata.Get(idx000[2] * planesize + idx000[1] * rowsize + idx000[0]);
    v001 = vecdata.Get(idx001[2] * planesize + idx001[1] * rowsize + idx001[0]);
    v010 = vecdata.Get(idx010[2] * planesize + idx010[1] * rowsize + idx010[0]);
    v011 = vecdata.Get(idx011[2] * planesize + idx011[1] * rowsize + idx011[0]);
    v100 = vecdata.Get(idx100[2] * planesize + idx100[1] * rowsize + idx100[0]);
    v101 = vecdata.Get(idx101[2] * planesize + idx101[1] * rowsize + idx101[0]);
    v110 = vecdata.Get(idx110[2] * planesize + idx110[1] * rowsize + idx110[0]);
    v111 = vecdata.Get(idx111[2] * planesize + idx111[1] * rowsize + idx111[0]);

    // Interpolation in X
    vtkm::Vec<FieldType, 3> v00, v01, v10, v11;
    FieldType a = pos[0] - static_cast<FieldType>(floor(pos[0]));
    v00[0] = (1.0f - a) * v000[0] + a * v001[0];
    v00[1] = (1.0f - a) * v000[1] + a * v001[1];
    v00[2] = (1.0f - a) * v000[2] + a * v001[2];

    v01[0] = (1.0f - a) * v010[0] + a * v011[0];
    v01[1] = (1.0f - a) * v010[1] + a * v011[1];
    v01[2] = (1.0f - a) * v010[2] + a * v011[2];

    v10[0] = (1.0f - a) * v100[0] + a * v101[0];
    v10[1] = (1.0f - a) * v100[1] + a * v101[1];
    v10[2] = (1.0f - a) * v100[2] + a * v101[2];

    v11[0] = (1.0f - a) * v110[0] + a * v111[0];
    v11[1] = (1.0f - a) * v110[1] + a * v111[1];
    v11[2] = (1.0f - a) * v110[2] + a * v111[2];

    // Interpolation in Y
    vtkm::Vec<FieldType, 3> v0, v1;
    a = pos[1] - static_cast<FieldType>(floor(pos[1]));
    v0[0] = (1.0f - a) * v00[0] + a * v01[0];
    v0[1] = (1.0f - a) * v00[1] + a * v01[1];
    v0[2] = (1.0f - a) * v00[2] + a * v01[2];

    v1[0] = (1.0f - a) * v10[0] + a * v11[0];
    v1[1] = (1.0f - a) * v10[1] + a * v11[1];
    v1[2] = (1.0f - a) * v10[2] + a * v11[2];

    // Interpolation in Z
    vtkm::Vec<FieldType, 3> v;
    a = pos[2] - static_cast<FieldType>(floor(pos[2]));
    v[0] = (1.0f - a) * v0[0] + v1[0];
    v[1] = (1.0f - a) * v0[1] + v1[1];
    v[2] = (1.0f - a) * v0[2] + v1[2];
    return v;
  }
}

/// \brief Compute the streamline
template <typename FieldType, typename DeviceAdapter>
class StreamLineUniformGridFilter
{
public:
  typedef vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> > FieldHandle;
  typedef typename FieldHandle::template ExecutionTypes<DeviceAdapter>::PortalConst FieldPortalConstType;

  class MakeStreamLines : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<IdType> seedId,
                                  FieldIn<> position,
                                  ExecObject numIndices,
                                  ExecObject streamLines);
    typedef void ExecutionSignature(_1, _2, _3, _4);
    typedef _1 InputDomain;

    FieldPortalConstType field;
    const vtkm::Id3 vdims;
    const vtkm::Id maxsteps;
    const FieldType timestep;
    const vtkm::Id planesize;
    const vtkm::Id rowsize;
    const vtkm::Id mode;

    VTKM_CONT_EXPORT
    MakeStreamLines(const vtkm::Id streamMode,
                    const FieldType timeStep, 
                    const vtkm::Id maxSteps, 
                    const vtkm::Id3 dims, 
                    FieldPortalConstType fieldArray) :
                                  mode(streamMode), 
                                  timestep(timeStep), 
                                  maxsteps(maxSteps), 
                                  vdims(dims), 
                                  planesize(dims[0] * dims[1]),
                                  rowsize(dims[0]),
                                  field(fieldArray) 
    {
    }

    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id &seedId, 
                    vtkm::Vec<FieldType, 3> &seedPos,
                    vtkm::exec::ExecutionWholeArray<vtkm::IdComponent> &numIndices,
                    vtkm::exec::ExecutionWholeArray<vtkm::Vec<FieldType, 3> > &slLists) const
    {
      // Set offset information based on one direction of stream or both
      vtkm::Id streamfactor = 1;
      vtkm::Id streamincrement = 0;
      if (mode == vtkm::worklet::internal::BOTH)
      {
        streamfactor = 2;
        streamincrement = 1;
      }

      // Set initial offset into the output streams array
      vtkm::Id index = seedId * maxsteps * streamfactor;

      vtkm::Vec<FieldType, 3> pos = seedPos;
      vtkm::Vec<FieldType, 3> pre_pos = seedPos;

      bool done = false;
      vtkm::Id step = 0;

      // Forward tracing
      if (mode == vtkm::worklet::internal::FORWARD ||
          mode == vtkm::worklet::internal::BOTH)
      {
       slLists.Set(index++, pos);
       while (done != true && step < maxsteps)
       {
        vtkm::Vec<FieldType, 3> vdata, adata, bdata, cdata, ddata;
        vdata = internal::VecDataAtPos<FieldType, FieldPortalConstType>
                                      (pos, vdims, planesize, rowsize, field);
        for (vtkm::Id d = 0; d < 3; d++)
        {
          adata[d] = timestep * vdata[d];
          pos[d] += adata[d] / 2.0f;
        }

        vdata = internal::VecDataAtPos<FieldType, FieldPortalConstType>
                                      (pos, vdims, planesize, rowsize, field);
        for (vtkm::Id d = 0; d < 3; d++)
        {
          bdata[d] = timestep * vdata[d];
          pos[d] += bdata[d] / 2.0f;
        }

        vdata = internal::VecDataAtPos<FieldType, FieldPortalConstType>
                                      (pos, vdims, planesize, rowsize, field);
        for (vtkm::Id d = 0; d < 3; d++)
        {
          cdata[d] = timestep * vdata[d];
          pos[d] += cdata[d] / 2.0f;
        }

        vdata = internal::VecDataAtPos<FieldType, FieldPortalConstType>
                                      (pos, vdims, planesize, rowsize, field);
        for (vtkm::Id d = 0; d < 3; d++)
        {
          ddata[d] = timestep * vdata[d];
          pos[d] += (adata[d] + (2.0f * bdata[d]) + (2.0f * cdata[d]) + ddata[d]) / 6.0f;
        }

        if (pos[0] < 0.0f || pos[0] > vdims[0] || 
            pos[1] < 0.0f || pos[1] > vdims[1] || 
            pos[2] < 0.0f || pos[2] > vdims[2])
        {
          pos = pre_pos;
          done = true;
        } else {
          slLists.Set(index++, pos);
          pre_pos = pos;
        }
        step++;
       }
       numIndices.Set(seedId * streamfactor, static_cast<vtkm::IdComponent>(step));
      }

      // Set initial seed position for backward tracing
      pre_pos = seedPos;
      pos = seedPos;

      done = false;
      step = 0;

      // Backward tracing
      if (mode == vtkm::worklet::internal::BACKWARD ||
          mode == vtkm::worklet::internal::BOTH)
      {
       slLists.Set(index++, pos);
       while (done != true && step < maxsteps)
       {
        vtkm::Vec<FieldType, 3> vdata, adata, bdata, cdata, ddata;
        vdata = internal::VecDataAtPos<FieldType, FieldPortalConstType>
                                      (pos, vdims, planesize, rowsize, field);
        for (vtkm::Id d = 0; d < 3; d++)
        {
          adata[d] = timestep * (0.0f - vdata[d]);
          pos[d] += adata[d] / 2.0f;
        }

        vdata = internal::VecDataAtPos<FieldType, FieldPortalConstType>
                                      (pos, vdims, planesize, rowsize, field);
        for (vtkm::Id d = 0; d < 3; d++)
        {
          bdata[d] = timestep * (0.0f - vdata[d]);
          pos[d] += bdata[d] / 2.0f;
        }

        vdata = internal::VecDataAtPos<FieldType, FieldPortalConstType>
                                      (pos, vdims, planesize, rowsize, field);
        for (vtkm::Id d = 0; d < 3; d++)
        {
          cdata[d] = timestep * (0.0f - vdata[d]);
          pos[d] += cdata[d] / 2.0f;
        }

        vdata = internal::VecDataAtPos<FieldType, FieldPortalConstType>
                                      (pos, vdims, planesize, rowsize, field);
        for (vtkm::Id d = 0; d < 3; d++)
        {
          ddata[d] = timestep * (0.0f - vdata[d]);
          pos[d] += (adata[d] + (2.0f * bdata[d]) + (2.0f * cdata[d]) + ddata[d]) / 6.0f;
        }

        if (pos[0] < 0.0f || pos[0] > vdims[0] || 
            pos[1] < 0.0f || pos[1] > vdims[1] || 
            pos[2] < 0.0f || pos[2] > vdims[2])
        {
          pos = pre_pos;
          done = true;
        } else {
          slLists.Set(index++, pos);
          pre_pos = pos;
        }
        step++;
       }
       numIndices.Set((seedId * streamfactor) + streamincrement, static_cast<vtkm::IdComponent>(step));
      }
    }
  };

  StreamLineUniformGridFilter(
                   const vtkm::cont::DataSet &inDataSet,
                   vtkm::cont::DataSet &outDataSet,
                   vtkm::Id streammode,
                   vtkm::Id numseeds,
                   vtkm::Id maxsteps,
                   const FieldType timestep) :
                     InDataSet(inDataSet),
                     OutDataSet(outDataSet),
                     streamMode(streammode),
                     timeStep(timestep),
                     numSeeds(numseeds),
                     maxSteps(maxsteps)
  {
  }

  vtkm::cont::DataSet InDataSet;   // input dataset with structured cell set
  vtkm::cont::DataSet OutDataSet;  // output dataset with explicit cell set
  vtkm::Id streamMode;
  vtkm::Id numSeeds;
  vtkm::Id maxSteps;
  FieldType timeStep;

  void Run()
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithm;

    // Get information from input dataset
    vtkm::cont::CellSetStructured<3> &inCellSet =
      InDataSet.GetCellSet(0).template CastTo<vtkm::cont::CellSetStructured<3> >();
    vtkm::Id3 vdims= inCellSet.GetSchedulingRange(vtkm::TopologyElementTagPoint());

    vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> > fieldArray =
      InDataSet.GetField("vecData").GetData().
        CastToArrayHandle<vtkm::Vec<FieldType, 3>, VTKM_DEFAULT_STORAGE_TAG>();

    // Get the cell set from the output dataset
    vtkm::cont::CellSetExplicit<> &cellSet =
      OutDataSet.GetCellSet(0).template CastTo<vtkm::cont::CellSetExplicit<> >();

    // Generate random seeds for starting streamlines
    std::vector<vtkm::Vec<FieldType, 3> > seeds;
    for (vtkm::Id i = 0; i < numSeeds; i++)
    {
      vtkm::Vec<FieldType, 3> seed;
      seed[0] = static_cast<FieldType>(rand() % vdims[0]);
      seed[1] = static_cast<FieldType>(rand() % vdims[1]);
      seed[2] = static_cast<FieldType>(rand() % vdims[2]);
      seeds.push_back(seed);
    }
    vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> > seedPosArray = 
                vtkm::cont::make_ArrayHandle(&seeds[0], seeds.size());
    vtkm::cont::ArrayHandleCounting<vtkm::Id> seedIdArray(0, 1, numSeeds);

    // Number of streams * number of steps * [forward, backward]
    vtkm::Id numCells = numSeeds;
    if (streamMode == vtkm::worklet::internal::BOTH)
      numCells *= 2;
    vtkm::Id maxConnectivityLen = numCells * maxSteps;

    // Declare the empty stream array which will be used to fill the output cell set
    vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> > streamArray;
    streamArray.Allocate(maxConnectivityLen);

    // Set up output dataset components
    vtkm::cont::ArrayHandle<vtkm::UInt8> cellTypes;
    vtkm::cont::ArrayHandle<vtkm::IdComponent> numIndices;
    cellTypes.Allocate(numCells);
    numIndices.Allocate(numCells);

    // Worklet to make the streamlines
    MakeStreamLines makeStreamLines(streamMode,
                                    timeStep,
                                    maxSteps,
                                    vdims,
                                    fieldArray.PrepareForInput(DeviceAdapter()));
    typedef typename vtkm::worklet::DispatcherMapField<MakeStreamLines> MakeStreamLinesDispatcher;
    MakeStreamLinesDispatcher makeStreamLinesDispatcher(makeStreamLines);
    makeStreamLinesDispatcher.Invoke(
              seedIdArray, 
              seedPosArray,
              vtkm::exec::ExecutionWholeArray<vtkm::IdComponent>(numIndices, numCells),
              vtkm::exec::ExecutionWholeArray<vtkm::Vec<FieldType, 3> >(streamArray, maxConnectivityLen));

    for (vtkm::Id cell = 0; cell < numCells; cell++)
    {
      vtkm::IdComponent numIndicesInCell = numIndices.GetPortalConstControl().Get(cell);
printf("Cell %ld Count %ld\n", cell, numIndicesInCell);
    }

    // Size of connectivity based on size of returned streamlines
    vtkm::cont::ArrayHandle<vtkm::IdComponent> numIndicesOut;
    vtkm::IdComponent connectivityLen = DeviceAlgorithm::ScanExclusive(numIndices, numIndicesOut);
printf("connectivityLen %ld\n", connectivityLen);

    // Allocate output dataset components
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
    connectivity.Allocate(connectivityLen);
    vtkm::Vec<FieldType, 3> coordinates[connectivityLen];

    // Fill in output data set using stream array
    vtkm::Id sindex = 0;
    for (vtkm::Id cell = 0; cell < numCells; cell++)
    {
      vtkm::Id numIndicesInCell = numIndices.GetPortalConstControl().Get(cell);
      cellTypes.GetPortalControl().Set(cell, static_cast<vtkm::UInt8>(vtkm::CELL_SHAPE_POLY_LINE));
      for (vtkm::Id step = 0; step < numIndicesInCell; step++)
      {
        coordinates[sindex] = streamArray.GetPortalConstControl().Get(sindex);
        connectivity.GetPortalControl().Set(sindex, sindex);
        sindex++;
      }
    }
    cellSet.Fill(cellTypes, numIndices, connectivity);

    OutDataSet.AddCoordinateSystem(
               vtkm::cont::CoordinateSystem("coordinates", 1, coordinates, connectivityLen));

    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<FieldType,3> >::PortalConstControl PortalConstType;
    std::ofstream out;
    out.open("sl_trace", std::ofstream::out);
    for (int i = 0; i < connectivityLen; i++)
    {
      vtkm::Vec<FieldType,3> pos = coordinates[i];
      out << pos[0] << " " << pos[1] << " " << pos[2] << std::endl;
    }
  }
};

}
}

#endif // vtk_m_worklet_StreamLineUniformGrid_h
