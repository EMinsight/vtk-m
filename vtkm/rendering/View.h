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
#ifndef vtk_m_rendering_View_h
#define vtk_m_rendering_View_h
#include <vtkm/Math.h>
#include <vtkm/Matrix.h>
#include <vtkm/VectorAnalysis.h>

namespace vtkm {
namespace rendering {

class View
{
  class View3D
  {
  public:
    VTKM_CONT_EXPORT
    View3D() : FieldOfView(0.f), XPan(0), YPan(0), Zoom(1)
    {}

    VTKM_CONT_EXPORT
    vtkm::Matrix<vtkm::Float32,4,4> CreateViewMatrix()
    {
      return View::ViewMatrix(this->Position, this->LookAt, this->Up);
    }

    VTKM_CONT_EXPORT
    vtkm::Matrix<vtkm::Float32,4,4> CreateProjectionMatrix(vtkm::Int32 &width,
                                                           vtkm::Int32 &height,
                                                           vtkm::Float32 &nearPlane,
                                                           vtkm::Float32 &farPlane)
    {
      vtkm::Matrix<vtkm::Float32,4,4> matrix;
      vtkm::MatrixIdentity(matrix);

      vtkm::Float32 AspectRatio = vtkm::Float32(width) / vtkm::Float32(height);
      vtkm::Float32 fovRad = (this->FieldOfView * 3.1415926f)/180.f;
      fovRad = vtkm::Tan( fovRad * 0.5f);
      vtkm::Float32 size = nearPlane * fovRad;
      vtkm::Float32 left = -size * AspectRatio;
      vtkm::Float32 right = size * AspectRatio;
      vtkm::Float32 bottom = -size;
      vtkm::Float32 top = size;

      matrix(0,0) = 2.f * nearPlane / (right - left);
      matrix(1,1) = 2.f * nearPlane / (top - bottom);
      matrix(0,2) = (right + left) / (right - left);
      matrix(1,2) = (top + bottom) / (top - bottom);
      matrix(2,2) = -(farPlane + nearPlane)  / (farPlane - nearPlane);
      matrix(3,2) = -1.f;
      matrix(2,3) = -(2.f * farPlane * nearPlane) / (farPlane - nearPlane);
      matrix(3,3) = 0.f;

      vtkm::Matrix<vtkm::Float32,4,4> T, Z;
      T = View::TranslateMatrix(this->XPan, this->YPan, 0);
      Z = View::ScaleMatrix(this->Zoom, this->Zoom, 1);
      matrix = vtkm::MatrixMultiply(Z, vtkm::MatrixMultiply(T, matrix));
      return matrix;
    }


    vtkm::Vec<vtkm::Float32,3> Up;
    vtkm::Vec<vtkm::Float32,3> LookAt;
    vtkm::Vec<vtkm::Float32,3> Position;
    vtkm::Float32 FieldOfView;
    vtkm::Float32 XPan;
    vtkm::Float32 YPan;
    vtkm::Float32 Zoom;
  };

  class View2D
  {
  public:
    VTKM_CONT_EXPORT
    View2D() : Left(0.f), Right(0.f), Top(0.f), Bottom(0.f), XScale(1.f)
    {}

    VTKM_CONT_EXPORT
    vtkm::Matrix<vtkm::Float32,4,4> CreateViewMatrix()
    {
      vtkm::Vec<vtkm::Float32,3> lookAt((this->Left + this->Right)/2.f,
                                        (this->Top + this->Bottom)/2.f,
                                        0.f);
      vtkm::Vec<vtkm::Float32,3> position = lookAt;
      position[2] = 1.f;
      vtkm::Vec<vtkm::Float32,3> up(0,1,0);
      return View::ViewMatrix(position, lookAt, up);
    }

    VTKM_CONT_EXPORT
    vtkm::Matrix<vtkm::Float32,4,4> CreateProjectionMatrix(vtkm::Float32 &size,
                                                           vtkm::Float32 &near,
                                                           vtkm::Float32 &far,
                                                           vtkm::Float32 &aspect)
    {
      vtkm::Matrix<vtkm::Float32,4,4> matrix(0.f);
      vtkm::Float32 left = -size/2.f * aspect;
      vtkm::Float32 right = size/2.f * aspect;
      vtkm::Float32 bottom = -size/2.f;
      vtkm::Float32 top = size/2.f;

      matrix(0,0) = 2.f/(right-left);
      matrix(1,1) = 2.f/(top-bottom);
      matrix(2,2) = -2.f/(far-near);
      matrix(0,3) = -(right+left)/(right-left);
      matrix(1,3) = -(top+bottom)/(top-bottom);
      matrix(2,3) = -(far+near)/(far-near);
      matrix(3,3) = 1.f;
      return matrix;
    }

    vtkm::Float32 Left;
    vtkm::Float32 Right;
    vtkm::Float32 Top;
    vtkm::Float32 Bottom;
    vtkm::Float32 XScale;
  };

private:
  static VTKM_CONT_EXPORT
  vtkm::Matrix<vtkm::Float32,4,4> ViewMatrix(const vtkm::Vec<vtkm::Float32,3> &position,
                                             const vtkm::Vec<vtkm::Float32,3> &lookAt,
                                             const vtkm::Vec<vtkm::Float32,3> &up)
  {
    vtkm::Vec<vtkm::Float32,3> viewDir = position-lookAt;
    vtkm::Vec<vtkm::Float32,3> right = vtkm::Cross(up,viewDir);
    vtkm::Vec<vtkm::Float32,3> ru = vtkm::Cross(viewDir,right);

    vtkm::Normalize(viewDir);
    vtkm::Normalize(right);
    vtkm::Normalize(ru);

    vtkm::Matrix<vtkm::Float32,4,4> matrix;
    vtkm::MatrixIdentity(matrix);

    matrix(0,0) = right[0];
    matrix(0,1) = right[1];
    matrix(0,2) = right[2];
    matrix(1,0) = ru[0];
    matrix(1,1) = ru[1];
    matrix(1,2) = ru[2];
    matrix(2,0) = viewDir[0];
    matrix(2,1) = viewDir[1];
    matrix(2,2) = viewDir[2];

    matrix(0,3) = -vtkm::dot(right,position);
    matrix(1,3) = -vtkm::dot(ru,position);
    matrix(2,3) = -vtkm::dot(viewDir,position);

    return matrix;
  }

  static VTKM_CONT_EXPORT
  vtkm::Matrix<vtkm::Float32,4,4> ScaleMatrix(const vtkm::Vec<vtkm::Float32,3> &v)
  {
    return ScaleMatrix(v[0], v[1], v[2]);
  }
  static VTKM_CONT_EXPORT
  vtkm::Matrix<vtkm::Float32,4,4> ScaleMatrix(const vtkm::Float32 &s)
  {
    return ScaleMatrix(s,s,s);
  }

  static VTKM_CONT_EXPORT
  vtkm::Matrix<vtkm::Float32,4,4> ScaleMatrix(const vtkm::Float32 &x,
                                              const vtkm::Float32 &y,
                                              const vtkm::Float32 &z)
  {
    vtkm::Matrix<vtkm::Float32,4,4> scaleMatrix(0.0f);
    scaleMatrix(0,0) = x;
    scaleMatrix(1,1) = y;
    scaleMatrix(2,2) = z;
    scaleMatrix(3,3) = 1.0f;
    return scaleMatrix;
  }

  static VTKM_CONT_EXPORT
  vtkm::Matrix<vtkm::Float32,4,4> TranslateMatrix(const vtkm::Vec<vtkm::Float32,3> &v)
  {
    return TranslateMatrix(v[0], v[1], v[2]);
  }

  static VTKM_CONT_EXPORT
  vtkm::Matrix<vtkm::Float32,4,4> TranslateMatrix(const vtkm::Float32 &x,
                                                  const vtkm::Float32 &y,
                                                  const vtkm::Float32 &z)
  {
    vtkm::Matrix<vtkm::Float32,4,4> translateMatrix;
    vtkm::MatrixIdentity(translateMatrix);
    translateMatrix(0,3) = x;
    translateMatrix(1,3) = y;
    translateMatrix(2,3) = z;
    return translateMatrix;
  }

  VTKM_CONT_EXPORT
  vtkm::Matrix<vtkm::Float32,4,4>
  CreateTrackball(vtkm::Float32 p1x, vtkm::Float32 p1y, vtkm::Float32 p2x, vtkm::Float32 p2y)
  {
    const vtkm::Float32 RADIUS = 0.80f; //z value lookAt x = y = 0.0
    const vtkm::Float32 COMPRESSION = 3.5f; // multipliers for x and y.
    const vtkm::Float32 AR3 = RADIUS*RADIUS*RADIUS;

    vtkm::Matrix<vtkm::Float32,4,4> matrix;

    vtkm::MatrixIdentity(matrix);
    if (p1x==p2x && p1y==p2y) { return matrix; }

    vtkm::Vec<vtkm::Float32, 3> p1(p1x,p1y, AR3/((p1x*p1x+p1y*p1y)*COMPRESSION+AR3));
    vtkm::Vec<vtkm::Float32, 3> p2(p2x,p2y, AR3/((p2x*p2x+p2y*p2y)*COMPRESSION+AR3));
    vtkm::Vec<vtkm::Float32, 3> axis = vtkm::Normal(vtkm::Cross(p2,p1));
    //std::cout<<"Axis: "<<axis[0]<<" "<<axis[1]<<" "<<axis[2]<<std::endl;

    vtkm::Vec<vtkm::Float32, 3> p2_p1(p2[0]-p1[0], p2[1]-p1[1], p2[2]-p1[2]);
    vtkm::Float32 t = vtkm::Magnitude(p2_p1);
    t = vtkm::Min(vtkm::Max(t, -1.0f), 1.0f);
    vtkm::Float32 phi = static_cast<vtkm::Float32>(-2.0f*asin(t/(2.0f*RADIUS)));
    vtkm::Float32 val = static_cast<vtkm::Float32>(sin(phi/2.0f));
    axis[0] *= val;
    axis[1] *= val;
    axis[2] *= val;

    //quaternion
    vtkm::Float32 q[4] = {axis[0], axis[1], axis[2], static_cast<vtkm::Float32>(cos(phi/2.0f))};

    // normalize quaternion to unit magnitude
    t =  1.0f / static_cast<vtkm::Float32>(sqrt(q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]));
    q[0] *= t;
    q[1] *= t;
    q[2] *= t;
    q[3] *= t;

    /*
    std::cout<<"P1: "<<p1[0]<<" "<<p1[1]<<" "<<p1[2]<<std::endl;
    std::cout<<"P2: "<<p2[0]<<" "<<p2[1]<<" "<<p2[2]<<std::endl;
    std::cout<<"T= "<<t<<std::endl;
    std::cout<<"PHI= "<<phi<<std::endl;
    std::cout<<"QUAT: "<<q[0]<<" "<<q[1]<<" "<<q[2]<<" "<<q[3]<<std::endl;
    */

    matrix(0,0) = 1 - 2 * (q[1]*q[1] + q[2]*q[2]);
    matrix(0,1) = 2 * (q[0]*q[1] + q[2]*q[3]);
    matrix(0,2) = (2 * (q[2]*q[0] - q[1]*q[3]) );

    matrix(1,0) = 2 * (q[0]*q[1] - q[2]*q[3]);
    matrix(1,1) = 1 - 2 * (q[2]*q[2] + q[0]*q[0]);
    matrix(1,2) = (2 * (q[1]*q[2] + q[0]*q[3]) );

    matrix(2,0) = (2 * (q[2]*q[0] + q[1]*q[3]) );
    matrix(2,1) = (2 * (q[1]*q[2] - q[0]*q[3]) );
    matrix(2,2) = (1 - 2 * (q[1]*q[1] + q[0]*q[0]) );

    return matrix;
  }


public:
  enum ViewTypeEnum { VIEW_2D, VIEW_3D };
  ViewTypeEnum ViewType;
  View3D View3d;
  View2D View2d;

  vtkm::Int32 Width;
  vtkm::Int32 Height;
  vtkm::Float32 NearPlane;
  vtkm::Float32 FarPlane;

  vtkm::Float32 ViewportLeft;
  vtkm::Float32 ViewportRight;
  vtkm::Float32 ViewportBottom;
  vtkm::Float32 ViewportTop;

  VTKM_CONT_EXPORT
  View(ViewTypeEnum vtype=View::VIEW_3D)
    : ViewType(vtype),
      Width(-1),
      Height(-1),
      NearPlane(0.f),
      FarPlane(1.f),
      ViewportLeft(-1.f),
      ViewportRight(1.f),
      ViewportBottom(-1.f),
      ViewportTop(1.f)
  {}

  VTKM_CONT_EXPORT
  vtkm::Matrix<vtkm::Float32,4,4> CreateViewMatrix()
  {
    if (this->ViewType == View::VIEW_3D)
    {
      return this->View3d.CreateViewMatrix();
    }
    else
    {
      return this->View2d.CreateViewMatrix();
    }
  }

  VTKM_CONT_EXPORT
  vtkm::Matrix<vtkm::Float32,4,4> CreateProjectionMatrix()
  {
    if (this->ViewType == View::VIEW_3D)
    {
      return this->View3d.CreateProjectionMatrix(
            this->Width, this->Height, this->NearPlane, this->FarPlane);
    }
    else
    {
      vtkm::Float32 size = vtkm::Abs(this->View2d.Top - this->View2d.Bottom);
      vtkm::Float32 left,right,bottom,top;
      this->GetRealViewport(left,right,bottom,top);
      vtkm::Float32 aspect =
          (static_cast<vtkm::Float32>(this->Width)*(right-left)) /
          (static_cast<vtkm::Float32>(this->Height)*(top-bottom));

      return this->View2d.CreateProjectionMatrix(
            size, this->NearPlane, this->FarPlane, aspect);
    }
  }

  VTKM_CONT_EXPORT
  void GetRealViewport(vtkm::Float32 &left, vtkm::Float32 &right,
                       vtkm::Float32 &bottom, vtkm::Float32 &top)
  {
    if (this->ViewType == View::VIEW_3D)
    {
      left = this->ViewportLeft;
      right = this->ViewportRight;
      bottom = this->ViewportBottom;
      top = this->ViewportTop;
    }
    else
    {
      vtkm::Float32 maxvw = (this->ViewportRight-this->ViewportLeft) * static_cast<vtkm::Float32>(this->Width);
      vtkm::Float32 maxvh = (this->ViewportTop-this->ViewportBottom) * static_cast<vtkm::Float32>(this->Height);
      vtkm::Float32 waspect = maxvw / maxvh;
      vtkm::Float32 daspect = (this->View2d.Right - this->View2d.Left) / (this->View2d.Top - this->View2d.Bottom);
      daspect *= this->View2d.XScale;
      //cerr << "waspect="<<waspect << "   \tdaspect="<<daspect<<endl;
      const bool center = true; // if false, anchor to bottom-left
      if (waspect > daspect)
      {
        vtkm::Float32 new_w = (this->ViewportRight-this->ViewportLeft) * daspect / waspect;
        if (center)
        {
          left = (this->ViewportLeft+this->ViewportRight)/2.f - new_w/2.f;
          right = (this->ViewportLeft+this->ViewportRight)/2.f + new_w/2.f;
        }
        else
        {
          left = this->ViewportLeft;
          right = this->ViewportLeft + new_w;
        }
        bottom = this->ViewportBottom;
        top = this->ViewportTop;
      }
      else
      {
        vtkm::Float32 new_h = (this->ViewportTop-this->ViewportBottom) * waspect / daspect;
        if (center)
        {
          bottom = (this->ViewportBottom+this->ViewportTop)/2.f - new_h/2.f;
          top = (this->ViewportBottom+this->ViewportTop)/2.f + new_h/2.f;
        }
        else
        {
          bottom = this->ViewportBottom;
          top = this->ViewportBottom + new_h;
        }
        left = this->ViewportLeft;
        right = this->ViewportRight;
      }
    }
  }

  VTKM_CONT_EXPORT
  vtkm::Vec<vtkm::Float32, 3>
  MultVector(const vtkm::Matrix<vtkm::Float32,4,4> &matrix, vtkm::Vec<vtkm::Float32, 3> &v)
  {
    vtkm::Vec<vtkm::Float32,4> v4(v[0],v[1],v[2], 1);
    v4 = vtkm::MatrixMultiply(matrix, v4);
    v[0] = v4[0];
    v[1] = v4[1];
    v[2] = v4[2];
    return v;
  }

  VTKM_CONT_EXPORT
  void Pan3D(vtkm::Float32 dx, vtkm::Float32 dy)
  {
    //std::cout<<"Pan3d: "<<dx<<" "<<dy<<std::endl;
    this->View3d.XPan += dx;
    this->View3d.YPan += dy;
  }

  VTKM_CONT_EXPORT
  void Zoom3D(vtkm::Float32 zoom)
  {
    vtkm::Float32 factor = powf(4, zoom);
    //std::cout<<"Zoom3D: "<<zoom<<" --> "<<factor<<std::endl;
    this->View3d.Zoom *= factor;
    this->View3d.XPan *= factor;
    this->View3d.YPan *= factor;
  }

  VTKM_CONT_EXPORT
  void TrackballRotate(vtkm::Float32 x1, vtkm::Float32 y1, vtkm::Float32 x2, vtkm::Float32 y2)
  {
    /*
    std::cout<<std::endl;
    std::cout<<"*****************************************************************"<<std::endl;
    std::cout<<x1<<" "<<y1<<" --> "<<x2<<" "<<y2<<std::endl;
    printVec("position", view3d.Position);
    printVec("lookAt", view3d.lookAt);
    printVec("up", view3d.up);
    std::cout<<"*****************************************************************"<<std::endl;
    */
    vtkm::Matrix<vtkm::Float32,4,4> R1 = CreateTrackball(x1,y1, x2,y2);

    //Translate matrix
    vtkm::Matrix<vtkm::Float32,4,4> T1 = View::TranslateMatrix(-this->View3d.LookAt);
    //vtkm::MatrixIdentity(T1);
    //T1(0,3) = -view3d.lookAt[0];
    //T1(1,3) = -view3d.lookAt[1];
    //T1(2,3) = -view3d.lookAt[2];

    //Translate matrix
    vtkm::Matrix<vtkm::Float32,4,4> T2 = View::TranslateMatrix(this->View3d.LookAt);
    //T2(0,3) = view3d.lookAt[0];
    //T2(1,3) = view3d.lookAt[1];
    //T2(2,3) = view3d.lookAt[2];

    vtkm::Matrix<vtkm::Float32,4,4> V1 = this->CreateViewMatrix();
    V1(0,3) = 0;
    V1(1,3) = 0;
    V1(2,3) = 0;

    vtkm::Matrix<vtkm::Float32,4,4> V2 = vtkm::MatrixTranspose(V1);

    //MM = T2 * V2 * R1 * V1 * T1;
    vtkm::Matrix<vtkm::Float32,4,4> MM;
    MM = vtkm::MatrixMultiply(T2,
                              vtkm::MatrixMultiply(V2,
                                                   vtkm::MatrixMultiply(R1,
                                                                        vtkm::MatrixMultiply(V1,T1))));
    this->View3d.Position = MultVector(MM, this->View3d.Position);
    this->View3d.LookAt = MultVector(MM, this->View3d.LookAt);
    this->View3d.Up = MultVector(MM, this->View3d.Up);

    /*
    printMtx("T1", T1);
    printMtx("T2", T2);
    printMtx("V1", V1);
    printMtx("V2", V2);
    printMtx("R1", R1);
    printMtx("MM", MM);
    printVec("position", this->View3d.Position);
    printVec("lookAt", this->View3d.LookAt);
    printVec("up", this->View3d.Up);
    */
  }
};

}} // namespace vtkm::rendering

#endif // vtk_m_rendering_View_h