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
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/rendering/Window.h>
#include <vtkm/rendering/RenderSurfaceOSMesa.h>
#include <vtkm/rendering/WorldAnnotatorGL.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/Plot.h>
#include <vtkm/rendering/SceneRendererGL.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/testing/Testing.h>

namespace {

void Set3DView(vtkm::rendering::View &view,
               const vtkm::cont::CoordinateSystem &coords,
               vtkm::Int32 w, vtkm::Int32 h)
{
    vtkm::Float64 coordsBounds[6]; // Xmin,Xmax,Ymin..
    coords.GetBounds(coordsBounds,VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    //set up a default view
    vtkm::Vec<vtkm::Float32,3> totalExtent;
    totalExtent[0] = vtkm::Float32(coordsBounds[1] - coordsBounds[0]);
    totalExtent[1] = vtkm::Float32(coordsBounds[3] - coordsBounds[2]);
    totalExtent[2] = vtkm::Float32(coordsBounds[5] - coordsBounds[4]);
    vtkm::Float32 mag = vtkm::Magnitude(totalExtent);
    vtkm::Normalize(totalExtent);

    view = vtkm::rendering::View(vtkm::rendering::View::VIEW_3D);
    view.View3d.Position = totalExtent * (mag * 2.f);
    view.View3d.Up = vtkm::Vec<vtkm::Float32,3>(0.f, 1.f, 0.f);
    view.View3d.LookAt = totalExtent * (mag * .5f);
    view.View3d.FieldOfView = 60.f;
    view.NearPlane = 1.f;
    view.FarPlane = 100.f;
    view.Width = w;
    view.Height = h;
    /*
    std::cout<<"View3d:  pos: "<<view.view3d.pos<<std::endl;
    std::cout<<"      lookAt: "<<view.view3d.lookAt<<std::endl;
    std::cout<<"          up: "<<view.view3d.up<<std::endl;
    std::cout<<"near/far/fov: "<<view.nearPlane<<"/"<<view.farPlane<<" "<<view.view3d.fieldOfView<<std::endl;
    std::cout<<"         w/h: "<<view.width<<"/"<<view.height<<std::endl;
    */
}

void Set2DView(vtkm::rendering::View &view,
               const vtkm::cont::CoordinateSystem &coords,
               vtkm::Int32 w, vtkm::Int32 h)
{
    vtkm::Float64 coordsBounds[6]; // Xmin,Xmax,Ymin..
    coords.GetBounds(coordsBounds,VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    //set up a default view
    // totalExtent does not seem to be used
//    vtkm::Vec<vtkm::Float32,3> totalExtent;
//    totalExtent[0] = vtkm::Float32(coordsBounds[1] - coordsBounds[0]);
//    totalExtent[1] = vtkm::Float32(coordsBounds[3] - coordsBounds[2]);
//    totalExtent[2] = vtkm::Float32(coordsBounds[5] - coordsBounds[4]);
//    vtkm::Float32 mag = vtkm::Magnitude(totalExtent);
//    vtkm::Normalize(totalExtent);

    view = vtkm::rendering::View(vtkm::rendering::View::VIEW_2D);
    view.View2d.Left = static_cast<vtkm::Float32>(coordsBounds[0]);
    view.View2d.Right = static_cast<vtkm::Float32>(coordsBounds[1]);
    view.View2d.Bottom = static_cast<vtkm::Float32>(coordsBounds[2]);
    view.View2d.Top = static_cast<vtkm::Float32>(coordsBounds[3]);
    view.NearPlane = 1.f;
    view.FarPlane = 100.f;
    view.Width = w;
    view.Height = h;

    // Give it some space for other annotations like a color bar
    view.ViewportLeft = -.7f;
    view.ViewportRight = +.7f;
    view.ViewportBottom = -.7f;
    view.ViewportTop = +.7f;

    /*
    std::cout<<"View2d:  l/r: "<<view.view2d.left<<" "<<view.view2d.right<<std::endl;
    std::cout<<"View2d:  b/t: "<<view.view2d.bottom<<" "<<view.view2d.top<<std::endl;
    std::cout<<"    near/far: "<<view.nearPlane<<"/"<<view.farPlane<<std::endl;
    std::cout<<"         w/h: "<<view.width<<"/"<<view.height<<std::endl;
    */
}

void Render3D(const vtkm::cont::DataSet &ds,
              const std::string &fieldNm,
              const std::string &ctName,
              const std::string &outputFile)
{
    const vtkm::Int32 W = 512, H = 512;
    const vtkm::cont::CoordinateSystem coords = ds.GetCoordinateSystem();
    vtkm::rendering::SceneRendererGL<VTKM_DEFAULT_DEVICE_ADAPTER_TAG> sceneRenderer;

    vtkm::rendering::View view;
    Set3DView(view, coords, W, H);

    vtkm::rendering::Scene3D scene;
    vtkm::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);
    vtkm::rendering::RenderSurfaceOSMesa surface(W,H,bg);

    scene.Plots.push_back(vtkm::rendering::Plot(ds.GetCellSet(),
                                                ds.GetCoordinateSystem(),
                                                ds.GetField(fieldNm),
                                                vtkm::rendering::ColorTable(ctName)));

    //TODO: W/H in window.  bg in window (window sets surface/renderer).
    vtkm::rendering::Window3D<vtkm::rendering::SceneRendererGL<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>,
                              vtkm::rendering::RenderSurfaceOSMesa,
                              vtkm::rendering::WorldAnnotatorGL>
        w(scene, sceneRenderer, surface, view, bg);

    w.Initialize();
    w.Paint();
    w.SaveAs(outputFile);
}

void Render2D(const vtkm::cont::DataSet &ds,
              const std::string &fieldNm,
              const std::string &ctName,
              const std::string &outputFile)
{
    const vtkm::Int32 W = 512, H = 512;
    const vtkm::cont::CoordinateSystem coords = ds.GetCoordinateSystem();
    vtkm::rendering::SceneRendererGL<VTKM_DEFAULT_DEVICE_ADAPTER_TAG> sceneRenderer;

    vtkm::rendering::View view;
    Set2DView(view, coords, W, H);

    vtkm::rendering::Scene2D scene;
    vtkm::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);
    vtkm::rendering::RenderSurfaceOSMesa surface(W,H,bg);

    scene.Plots.push_back(vtkm::rendering::Plot(ds.GetCellSet(),
                                                ds.GetCoordinateSystem(),
                                                ds.GetField(fieldNm),
                                                vtkm::rendering::ColorTable(ctName)));
    vtkm::rendering::Window2D<vtkm::rendering::SceneRendererGL<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>,
                              vtkm::rendering::RenderSurfaceOSMesa,
                              vtkm::rendering::WorldAnnotatorGL>
        w(scene, sceneRenderer, surface, view, bg);

    w.Initialize();
    w.Paint();
    w.SaveAs(outputFile);

}

void RenderTests()
{
    vtkm::cont::testing::MakeTestDataSet maker;

    //3D tests.
    Render3D(maker.Make3DRegularDataSet0(),
             "pointvar", "thermal", "reg3D.pnm");
    Render3D(maker.Make3DRectilinearDataSet0(),
             "pointvar", "thermal", "rect3D.pnm");
    Render3D(maker.Make3DExplicitDataSet4(),
             "pointvar", "thermal", "expl3D.pnm");

    //2D tests.
    Render2D(maker.Make2DRectilinearDataSet0(),
             "pointvar", "thermal", "rect2D.pnm");
}
} //namespace

int UnitTestSceneRendererOSMesa(int, char *[])
{
    return vtkm::cont::testing::Testing::Run(RenderTests);
}