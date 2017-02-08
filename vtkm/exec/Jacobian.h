//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_exec_Jacobian_h
#define vtk_m_exec_Jacobian_h

#include <vtkm/Assert.h>
#include <vtkm/CellShape.h>
#include <vtkm/Math.h>
#include <vtkm/Matrix.h>
#include <vtkm/VectorAnalysis.h>

namespace vtkm {
namespace exec {
namespace internal {

template<typename T>
struct Space2D
{
  typedef vtkm::Vec<T,3> Vec3;
  typedef vtkm::Vec<T,2> Vec2;

  Vec3 Origin;
  Vec3 Basis0;
  Vec3 Basis1;

  VTKM_EXEC
  Space2D(const Vec3 &origin, const Vec3 &pointFirst, const Vec3 &pointLast)
  {
    this->Origin = origin;

    this->Basis0 = vtkm::Normal(pointFirst - this->Origin);

    Vec3 n = vtkm::Cross(this->Basis0, pointLast - this->Origin);
    this->Basis1 = vtkm::Normal(vtkm::Cross(this->Basis0, n));
  }

  VTKM_EXEC
  Vec2 ConvertCoordToSpace(const Vec3 coord) const {
    Vec3 vec = coord - this->Origin;
    return Vec2(vtkm::dot(vec, this->Basis0), vtkm::dot(vec, this->Basis1));
  }

  template<typename U>
  VTKM_EXEC
  vtkm::Vec<U,3> ConvertVecFromSpace(const vtkm::Vec<U,2> vec) const {
    return vec[0]*this->Basis0 + vec[1]*this->Basis1;
  }
};

// Given a series of point values for a wedge, return a new series of point
// for a hexahedron that has the same interpolation within the wedge.
template<typename FieldVecType>
VTKM_EXEC
vtkm::Vec<typename FieldVecType::ComponentType,8>
PermuteWedgeToHex(const FieldVecType &field)
{
  vtkm::Vec<typename FieldVecType::ComponentType,8> hexField;

  hexField[0] = field[0];
  hexField[1] = field[2];
  hexField[2] = field[2] + field[1] - field[0];
  hexField[3] = field[1];
  hexField[4] = field[3];
  hexField[5] = field[5];
  hexField[6] = field[5] + field[4] - field[3];
  hexField[7] = field[4];

  return hexField;
}

// Given a series of point values for a pyramid, return a new series of point
// for a hexahedron that has the same interpolation within the pyramid.
template<typename FieldVecType>
VTKM_EXEC
vtkm::Vec<typename FieldVecType::ComponentType,8>
PermutePyramidToHex(const FieldVecType &field)
{
  typedef typename FieldVecType::ComponentType T;

  vtkm::Vec<T,8> hexField;

  T baseCenter = T(0.25f)*(field[0]+field[1]+field[2]+field[3]);

  hexField[0] = field[0];
  hexField[1] = field[1];
  hexField[2] = field[2];
  hexField[3] = field[3];
  hexField[4] = field[4]+(field[0]-baseCenter);
  hexField[5] = field[4]+(field[1]-baseCenter);
  hexField[6] = field[4]+(field[2]-baseCenter);
  hexField[7] = field[4]+(field[3]-baseCenter);

  return hexField;
}

} //namespace internal

#define VTKM_DERIVATIVE_WEIGHTS_HEXAHEDRON(pc, rc, call) \
  call(0, -rc[1]*rc[2], -rc[0]*rc[2], -rc[0]*rc[1]); \
  call(1,  rc[1]*rc[2], -pc[0]*rc[2], -pc[0]*rc[1]); \
  call(2,  pc[1]*rc[2],  pc[0]*rc[2], -pc[0]*pc[1]); \
  call(3, -pc[1]*rc[2],  rc[0]*rc[2], -rc[0]*pc[1]); \
  call(4, -rc[1]*pc[2], -rc[0]*pc[2],  rc[0]*rc[1]); \
  call(5,  rc[1]*pc[2], -pc[0]*pc[2],  pc[0]*rc[1]); \
  call(6,  pc[1]*pc[2],  pc[0]*pc[2],  pc[0]*pc[1]); \
  call(7, -pc[1]*pc[2],  rc[0]*pc[2],  rc[0]*pc[1])

#define VTKM_DERIVATIVE_WEIGHTS_VOXEL(pc, rc, call) \
  call(0, -rc[1]*rc[2], -rc[0]*rc[2], -rc[0]*rc[1]); \
  call(1,  rc[1]*rc[2], -pc[0]*rc[2], -pc[0]*rc[1]); \
  call(2, -pc[1]*rc[2],  rc[0]*rc[2], -rc[0]*pc[1]); \
  call(3,  pc[1]*rc[2],  pc[0]*rc[2], -pc[0]*pc[1]); \
  call(4, -rc[1]*pc[2], -rc[0]*pc[2],  rc[0]*rc[1]); \
  call(5,  rc[1]*pc[2], -pc[0]*pc[2],  pc[0]*rc[1]); \
  call(6, -pc[1]*pc[2],  rc[0]*pc[2],  rc[0]*pc[1]); \
  call(7,  pc[1]*pc[2],  pc[0]*pc[2],  pc[0]*pc[1])

#define VTKM_DERIVATIVE_WEIGHTS_WEDGE(pc, rc, call) \
  call(0, -rc[2], -rc[2], -1.0f+pc[0]+pc[1]); \
  call(1,   0.0f,  rc[2],            -pc[1]); \
  call(2,  rc[2],   0.0f,            -pc[0]); \
  call(3, -pc[2], -pc[2],  1.0f-pc[0]-pc[1]); \
  call(4,   0.0f,  pc[2],             pc[1]); \
  call(5,  pc[2],   0.0f,             pc[0])

#define VTKM_DERIVATIVE_WEIGHTS_PYRAMID(pc, rc, call) \
  call(0, -rc[1]*rc[2], -rc[0]*rc[2], -rc[0]*rc[1]); \
  call(1,  rc[1]*rc[2], -pc[0]*rc[2], -pc[0]*rc[1]); \
  call(2,  pc[1]*rc[2],  pc[0]*rc[2], -pc[0]*pc[1]); \
  call(3, -pc[1]*rc[2],  rc[0]*rc[2], -rc[0]*pc[1]); \
  call(3,         0.0f,         0.0f,         1.0f)


#define VTKM_DERIVATIVE_WEIGHTS_QUAD(pc, rc, call) \
  call(0, -rc[1], -rc[0]); \
  call(1,  rc[1], -pc[0]); \
  call(2,  pc[1],  pc[0]); \
  call(3, -pc[1],  rc[0])

#define VTKM_DERIVATIVE_WEIGHTS_PIXEL(pc, rc, call) \
  call(0, -rc[1], -rc[0]); \
  call(1,  rc[1], -pc[0]); \
  call(2, -pc[1],  rc[0]); \
  call(3,  pc[1],  pc[0])

//-----------------------------------------------------------------------------
// This returns the Jacobian of a hexahedron's (or other 3D cell's) coordinates
// with respect to parametric coordinates. Explicitly, this is (d is partial
// derivative):
//
//   |                     |
//   | dx/du  dx/dv  dx/dw |
//   |                     |
//   | dy/du  dy/dv  dy/dw |
//   |                     |
//   | dz/du  dz/dv  dz/dw |
//   |                     |
//

#define VTKM_ACCUM_JACOBIAN_3D(pointIndex, weight0, weight1, weight2) \
  jacobian(0,0) += static_cast<JacobianType>(wCoords[pointIndex][0] * (weight0)); \
  jacobian(1,0) += static_cast<JacobianType>(wCoords[pointIndex][1] * (weight0)); \
  jacobian(2,0) += static_cast<JacobianType>(wCoords[pointIndex][2] * (weight0)); \
  jacobian(0,1) += static_cast<JacobianType>(wCoords[pointIndex][0] * (weight1)); \
  jacobian(1,1) += static_cast<JacobianType>(wCoords[pointIndex][1] * (weight1)); \
  jacobian(2,1) += static_cast<JacobianType>(wCoords[pointIndex][2] * (weight1)); \
  jacobian(0,2) += static_cast<JacobianType>(wCoords[pointIndex][0] * (weight2)); \
  jacobian(1,2) += static_cast<JacobianType>(wCoords[pointIndex][1] * (weight2)); \
  jacobian(2,2) += static_cast<JacobianType>(wCoords[pointIndex][2] * (weight2))

template<typename WorldCoordType,
         typename ParametricCoordType,
         typename JacobianType>
VTKM_EXEC
void JacobianFor3DCell(const WorldCoordType &wCoords,
                       const vtkm::Vec<ParametricCoordType,3> &pcoords,
                       vtkm::Matrix<JacobianType,3,3> &jacobian,
                       vtkm::CellShapeTagHexahedron)
{
  vtkm::Vec<JacobianType,3> pc(pcoords);
  vtkm::Vec<JacobianType,3> rc = vtkm::Vec<JacobianType,3>(JacobianType(1)) - pc;

  jacobian = vtkm::Matrix<JacobianType,3,3>(JacobianType(0));
  VTKM_DERIVATIVE_WEIGHTS_HEXAHEDRON(pc, rc, VTKM_ACCUM_JACOBIAN_3D);
}

template<typename WorldCoordType,
         typename ParametricCoordType,
         typename JacobianType>
VTKM_EXEC
void JacobianFor3DCell(const WorldCoordType &wCoords,
                       const vtkm::Vec<ParametricCoordType,3> &pcoords,
                       vtkm::Matrix<JacobianType,3,3> &jacobian,
                       vtkm::CellShapeTagWedge)
{
#if 0
  // This is not working. Just leverage the hexahedron code that is working.
  vtkm::Vec<JacobianType,3> pc(pcoords);
  vtkm::Vec<JacobianType,3> rc = vtkm::Vec<JacobianType,3>(1) - pc;

  jacobian = vtkm::Matrix<JacobianType,3,3>(0);
  VTKM_DERIVATIVE_WEIGHTS_WEDGE(pc, rc, VTKM_ACCUM_JACOBIAN_3D);
#else
  JacobianFor3DCell(internal::PermuteWedgeToHex(wCoords),
                    pcoords,
                    jacobian,
                    vtkm::CellShapeTagHexahedron());
#endif
}

template<typename WorldCoordType,
         typename ParametricCoordType,
         typename JacobianType>
VTKM_EXEC
void JacobianFor3DCell(const WorldCoordType &wCoords,
                       const vtkm::Vec<ParametricCoordType,3> &pcoords,
                       vtkm::Matrix<JacobianType,3,3> &jacobian,
                       vtkm::CellShapeTagPyramid)
{
#if 0
  // This is not working. Just leverage the hexahedron code that is working.
  vtkm::Vec<JacobianType,3> pc(pcoords);
  vtkm::Vec<JacobianType,3> rc = vtkm::Vec<JacobianType,3>(1) - pc;

  jacobian = vtkm::Matrix<JacobianType,3,3>(0);
  VTKM_DERIVATIVE_WEIGHTS_PYRAMID(pc, rc, VTKM_ACCUM_JACOBIAN_3D);
#else
  JacobianFor3DCell(internal::PermutePyramidToHex(wCoords),
                    pcoords,
                    jacobian,
                    vtkm::CellShapeTagHexahedron());
#endif
}

// Derivatives in quadrilaterals are computed in much the same way as
// hexahedra.  Review the documentation for hexahedra derivatives for details
// on the math.  The major difference is that the equations are performed in
// a 2D space built with make_SpaceForQuadrilateral.

#define VTKM_ACCUM_JACOBIAN_2D(pointIndex, weight0, weight1) \
  wcoords2d = space.ConvertCoordToSpace(wCoords[pointIndex]); \
  jacobian(0,0) += wcoords2d[0] * (weight0); \
  jacobian(1,0) += wcoords2d[1] * (weight0); \
  jacobian(0,1) += wcoords2d[0] * (weight1); \
  jacobian(1,1) += wcoords2d[1] * (weight1)

template<typename WorldCoordType,
         typename ParametricCoordType,
         typename SpaceType,
         typename JacobianType>
VTKM_EXEC
void JacobianFor2DCell(const WorldCoordType &wCoords,
                       const vtkm::Vec<ParametricCoordType,3> &pcoords,
                       const vtkm::exec::internal::Space2D<SpaceType> &space,
                       vtkm::Matrix<JacobianType,2,2> &jacobian,
                       vtkm::CellShapeTagQuad)
{
  vtkm::Vec<JacobianType,2> pc(static_cast<JacobianType>(pcoords[0]),
                               static_cast<JacobianType>(pcoords[1]));
  vtkm::Vec<JacobianType,2> rc = vtkm::Vec<JacobianType,2>(JacobianType(1)) - pc;

  vtkm::Vec<SpaceType,2> wcoords2d;
  jacobian = vtkm::Matrix<JacobianType,2,2>(JacobianType(0));
  VTKM_DERIVATIVE_WEIGHTS_QUAD(pc, rc, VTKM_ACCUM_JACOBIAN_2D);
}

#if 0
// This code doesn't work, so I'm bailing on it. Instead, I'm just grabbing a
// triangle and finding the derivative of that. If you can do better, please
// implement it.
template<typename WorldCoordType,
         typename ParametricCoordType,
         typename JacobianType>
VTKM_EXEC
void JacobianFor2DCell(const WorldCoordType &wCoords,
                       const vtkm::Vec<ParametricCoordType,3> &pcoords,
                       const vtkm::exec::internal::Space2D<JacobianType> &space,
                       vtkm::Matrix<JacobianType,2,2> &jacobian,
                       vtkm::CellShapeTagPolygon)
{
  const vtkm::IdComponent numPoints = wCoords.GetNumberOfComponents();
  vtkm::Vec<JacobianType,2> pc(pcoords[0], pcoords[1]);
  JacobianType deltaAngle = static_cast<JacobianType>(2*vtkm::Pi()/numPoints);

  jacobian = vtkm::Matrix<JacobianType,2,2>(0);
  for (vtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
  {
    JacobianType angle = pointIndex*deltaAngle;
    vtkm::Vec<JacobianType,2> nodePCoords(0.5f*(vtkm::Cos(angle)+1),
                                          0.5f*(vtkm::Sin(angle)+1));

    // This is the vector pointing from the user provided parametric coordinate
    // to the node at pointIndex in parametric space.
    vtkm::Vec<JacobianType,2> pvec = nodePCoords - pc;

    // The weight (the derivative of the interpolation factor) happens to be
    // pvec scaled by the cube root of pvec's magnitude.
    JacobianType magSqr = vtkm::MagnitudeSquared(pvec);
    JacobianType invMag = vtkm::RSqrt(magSqr);
    JacobianType scale = invMag*invMag*invMag;
    vtkm::Vec<JacobianType,2> weight = scale*pvec;

    vtkm::Vec<JacobianType,2> wcoords2d =
        space.ConvertCoordToSpace(wCoords[pointIndex]);
    jacobian(0,0) += wcoords2d[0] * weight[0];
    jacobian(1,0) += wcoords2d[1] * weight[0];
    jacobian(0,1) += wcoords2d[0] * weight[1];
    jacobian(1,1) += wcoords2d[1] * weight[1];
  }
}
#endif

#undef VTKM_ACCUM_JACOBIAN_3D
#undef VTKM_ACCUM_JACOBIAN_2D

#undef VTKM_DERIVATIVE_WEIGHTS_HEXAHEDRON
#undef VTKM_DERIVATIVE_WEIGHTS_VOXEL
#undef VTKM_DERIVATIVE_WEIGHTS_WEDGE
#undef VTKM_DERIVATIVE_WEIGHTS_PYRAMID
#undef VTKM_DERIVATIVE_WEIGHTS_QUAD
#undef VTKM_DERIVATIVE_WEIGHTS_PIXEL


}
} // namespace vtkm::exec
#endif //vtk_m_exec_Jacobian_h