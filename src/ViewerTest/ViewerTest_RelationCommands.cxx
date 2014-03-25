// Created on: 1998-11-12
// Created by: Robert COUBLANC
// Copyright (c) 1998-1999 Matra Datavision
// Copyright (c) 1999-2014 OPEN CASCADE SAS
//
// This file is part of Open CASCADE Technology software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published
// by the Free Software Foundation, with special exception defined in the file
// OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in OCCT
// distribution for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of Open CASCADE
// commercial license or contractual agreement.

#include <ViewerTest.hxx>
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <AIS_AngleDimension.hxx>
#include <AIS_Circle.hxx>
#include <AIS_DiameterDimension.hxx>
#include <AIS_DisplayMode.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_LengthDimension.hxx>
#include <AIS_ListIteratorOfListOfInteractive.hxx>
#include <AIS_ListOfInteractive.hxx>
#include <AIS_MapOfInteractive.hxx>
#include <AIS_Point.hxx>
#include <AIS_RadiusDimension.hxx>
#include <AIS_Relation.hxx>
#include <AIS_Shape.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <Draw_Interpretor.hxx>
#include <Draw.hxx>
#include <Draw_Appli.hxx>
#include <Draw_Window.hxx>
#include <DBRep.hxx>
#include <ElSLib.hxx>
#include <GC_MakePlane.hxx>
#include <Geom_CartesianPoint.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Line.hxx>
#include <Geom_Plane.hxx>
#include <GeomAPI_IntCS.hxx>
#include <gce_MakeLin.hxx>
#include <gce_MakePln.hxx>
#include <gp_Circ.hxx>
#include <gp_Pln.hxx>
#include <IntAna_IntConicQuad.hxx>
#include <IntAna_Quadric.hxx>
#include <Precision.hxx>
#include <Select3D_Projector.hxx>
#include <StdSelect.hxx>
#include <string.h>
#include <TCollection_AsciiString.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TColStd_MapOfInteger.hxx>
#include <TopAbs.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Vertex.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <V3d.hxx>
#include <ViewerTest_DoubleMapOfInteractiveAndName.hxx>
#include <ViewerTest_DoubleMapIteratorOfDoubleMapOfInteractiveAndName.hxx>
#include <ViewerTest_EventManager.hxx>

extern Standard_Boolean VDisplayAISObject (const TCollection_AsciiString& theName,
                                           const Handle(AIS_InteractiveObject)& theAISObj,
                                           Standard_Boolean theReplaceIfExists = Standard_True);
extern ViewerTest_DoubleMapOfInteractiveAndName& GetMapOfAIS();
extern int ViewerMainLoop(Standard_Integer argc, const char** argv);
extern Handle(AIS_InteractiveContext)& TheAISContext ();

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#define VertexMask 0x01
#define EdgeMask 0x02
#define FaceMask 0x04

//=======================================================================
//function : Get3DPointAtMousePosition
//purpose  : Calculates the 3D points corresponding to the mouse position
//           in the plane of the view
//=======================================================================
static gp_Pnt Get3DPointAtMousePosition()
{
  Handle(V3d_View) aView = ViewerTest::CurrentView();
  static Select3D_Projector aProjector;
  aProjector.SetView (aView);

  Standard_Real xv,yv,zv;
  aView->Proj (xv,yv,zv);
  Standard_Real xat,yat,zat;
  aView->At(xat,yat,zat);
  gp_Pln aPlane (gp_Pnt(xat,yat,zat), gp_Dir(xv,yv,zv));
  
  Standard_Integer aPixX, aPixY;
  Standard_Real aX, aY, aZ, aDX, aDY, aDZ;

  ViewerTest::GetMousePosition (aPixX, aPixY);
  aView->ConvertWithProj (aPixX, aPixY, aX, aY, aZ, aDX, aDY, aDZ);
  gp_Lin aLine( gp_Pnt(aX, aY, aZ), gp_Dir(aDX, aDY, aDZ) );

  // Compute intersection
  Handle(Geom_Line) aGeomLine = new Geom_Line (aLine);
  Handle(Geom_Plane) aGeomPlane = new Geom_Plane (aPlane);
  GeomAPI_IntCS anIntersector (aGeomLine, aGeomPlane);
  if (!anIntersector.IsDone() || anIntersector.NbPoints() == 0)
  {
    return gp::Origin();
  }
  return anIntersector.Point (1);
}

//=======================================================================
//function : Get3DPointAtMousePosition
//purpose  : Calculates the 3D points corresponding to the mouse position
//           in the plane of the view
//=======================================================================
static Standard_Boolean Get3DPointAtMousePosition (const gp_Pnt& theFirstPoint,
                                                   const gp_Pnt& theSecondPoint,
                                                   gp_Pnt& theOutputPoint)
{
  theOutputPoint = gp::Origin();

  Handle(V3d_View) aView = ViewerTest::CurrentView();

  Standard_Integer aPixX, aPixY;
  Standard_Real aX, aY, aZ, aDx, aDy, aDz, aUx, aUy, aUz;

  // Get 3D point in view coordinates and projection vector from the pixel point.
  ViewerTest::GetMousePosition (aPixX, aPixY);
  aView->ConvertWithProj (aPixX, aPixY, aX, aY, aZ, aDx, aDy, aDz);
  gp_Lin aProjLin (gp_Pnt(aX, aY, aZ), gp_Dir(aDx, aDy, aDz));

  // Get plane
  gp_Vec aDimVec (theFirstPoint, theSecondPoint);
  aView->Up (aUx, aUy, aUz);
  gp_Vec aViewUp (aUx, aUy, aUz);

  if (aDimVec.IsParallel (aViewUp, Precision::Angular()))
  {
    theOutputPoint = Get3DPointAtMousePosition();
    return Standard_True;
  }

  gp_Vec aDimNormal = aDimVec ^ aViewUp;
  gp_Pln aViewPlane= gce_MakePln (theFirstPoint, aDimNormal);

  // Get intersection of view plane and projection line
  Handle(Geom_Plane) aPlane = new Geom_Plane (aViewPlane);
  Handle(Geom_Line) aProjLine = new Geom_Line (aProjLin);
  GeomAPI_IntCS anIntersector (aProjLine, aPlane);
  if (!anIntersector.IsDone() || anIntersector.NbPoints() == 0)
  {
    return Standard_False;
  }

  theOutputPoint = anIntersector.Point (1);
  return Standard_True;
}

//=======================================================================
//function : VDimBuilder
//purpose  : Command for building dimension presentations: angle,
//           length, radius, diameter
//=======================================================================
static int VDimBuilder(Draw_Interpretor& theDi, Standard_Integer theArgsNb, const char** theArgs)
{
  if (theArgsNb < 2)
  {
    std::cerr << theArgs[0] << ": command argument is required. Type help for more information.\n";
    return 1;
  }

  // Parse parameters
  TCollection_AsciiString aDimType(theArgs[1]);
  AIS_KindOfDimension aKindOfDimension;
  if (aDimType == "length")
  {
    aKindOfDimension = AIS_KOD_LENGTH;
  }
  else if (aDimType == "angle")
  {
    aKindOfDimension = AIS_KOD_PLANEANGLE;
  }
  else if (aDimType == "radius")
  {
    aKindOfDimension = AIS_KOD_RADIUS;
  }
  else if (aDimType == "diameter" || aDimType == "diam")
  {
    aKindOfDimension = AIS_KOD_DIAMETER;
  }
  else
  {
    std::cerr << theArgs[0] << ": wrong type of dimension. Type help for more information.\n";
    return 1;
  }
  NCollection_List<Handle(AIS_InteractiveObject)> aShapes;
  Handle(Prs3d_DimensionAspect) anAspect = new Prs3d_DimensionAspect;
  Standard_Boolean isPlaneCustom = Standard_False;
  TCollection_AsciiString aName;
  gp_Pln aWorkingPlane;
  Standard_Boolean isCustomFlyout = Standard_False;
  Standard_Real aCustomFlyout = 0.0;

  for (Standard_Integer anIt = 2; anIt < theArgsNb; ++anIt)
  {
    TCollection_AsciiString anArgString = theArgs[anIt];
    TCollection_AsciiString aParamName;
    TCollection_AsciiString aParamValue;
    if (ViewerTest::SplitParameter (anArgString, aParamName, aParamValue))
    {
      aParamName.LowerCase();
      aParamValue.LowerCase();

      if (aParamName == "text")
      {
        anAspect->MakeText3d (aParamValue == "3d");
      }
      else if (aParamName == "name")
      {
        if (aParamValue.IsEmpty())
        {
          std::cerr << theArgs[0] << ": no name for dimension.\n";
          return 1;
        }

        aName = aParamValue;
      }
      else if (aParamName == "plane")
      {
        if (aParamValue == "xoy")
        {
          aWorkingPlane = gp_Pln (gp_Ax3 (gp::XOY()));
        }
        else if (aParamValue == "zox")
        {
          aWorkingPlane = gp_Pln (gp_Ax3 (gp::ZOX()));
        }
        else if (aParamValue == "yoz")
        {
          aWorkingPlane = gp_Pln (gp_Ax3 (gp::YOZ()));
        }
        else
        {
          std::cerr << theArgs[0] << ": wrong plane.\n";
          return 1;
        }

        isPlaneCustom = Standard_True;
      }
      else if (aParamName == "label")
      {
        NCollection_List<TCollection_AsciiString> aListOfLabelVals;
        while (aParamValue.Length() > 0)
        {
          TCollection_AsciiString aValue = aParamValue;

          Standard_Integer aSeparatorPos = aParamValue.Search (",");
          if (aSeparatorPos >= 0)
          {
            aValue.Trunc (aSeparatorPos - 1);
            aParamValue.Remove (aSeparatorPos, 1);
          }

          aListOfLabelVals.Append (aValue);

          aParamValue.Remove (1, aValue.Length());
        }

        NCollection_List<TCollection_AsciiString>::Iterator aLabelValueIt (aListOfLabelVals);
        for ( ; aLabelValueIt.More(); aLabelValueIt.Next())
        {
          aParamValue = aLabelValueIt.Value();

          if (aParamValue == "left")
          {
            anAspect->SetTextHorizontalPosition (Prs3d_DTHP_Left);
          }
          else if (aParamValue == "right")
          {
            anAspect->SetTextHorizontalPosition (Prs3d_DTHP_Right);
          }
          else if (aParamValue == "hcenter")
          {
            anAspect->SetTextHorizontalPosition (Prs3d_DTHP_Center);
          }
          else if (aParamValue == "hfit")
          {
            anAspect->SetTextHorizontalPosition (Prs3d_DTHP_Fit);
          }
          else if (aParamValue == "above")
          {
            anAspect->SetTextVerticalPosition (Prs3d_DTVP_Above);
          }
          else if (aParamValue == "below")
          {
            anAspect->SetTextVerticalPosition (Prs3d_DTVP_Below);
          }
          else if (aParamValue == "vcenter")
          {
            anAspect->SetTextVerticalPosition (Prs3d_DTVP_Center);
          }
          else
          {
            std::cerr << theArgs[0] << ": invalid label position: \"" << aParamValue << "\".\n";
            return 1;
          }
        }
      }
      else if (aParamName == "flyout")
      {
        if (!aParamValue.IsRealValue())
        {
          std::cerr << theArgs[0] << ": numeric value expected for flyout.\n";
          return 1;
        }

        aCustomFlyout = aParamValue.RealValue();

        isCustomFlyout = Standard_True;
      }
      else if (aParamName == "arrows")
      {
        if (aParamValue == "external")
        {
          anAspect->SetArrowOrientation (Prs3d_DAO_External);
        }
        else if (aParamValue == "internal")
        {
          anAspect->SetArrowOrientation (Prs3d_DAO_Internal);
        }
        else if (aParamValue == "fit")
        {
          anAspect->SetArrowOrientation (Prs3d_DAO_Fit);
        }
      }
      else
      {
        std::cerr << theArgs[0] << ": unknow parameter: \"" << aParamName << "\".\n";
        return 1;
      }
    }
    else // Shape
    {
      if (!GetMapOfAIS().IsBound2 (anArgString))
      {
        std::cerr << theArgs[0] << ": wrong name of shape. May be here is a wrong parameter.\n";
        return 1;
      }

      Handle(AIS_InteractiveObject) aShape = Handle(AIS_InteractiveObject)::DownCast (GetMapOfAIS().Find2 (anArgString));
      if (aShape.IsNull())
      {
        std::cerr << theArgs[0] << ": wrong name of shape. Not a shape.\n";
        return 1;
      }

      aShapes.Append (aShape);
    }
  }

  if (aName.IsEmpty())
  {
    std::cerr << theArgs[0] << ": no name for dimension.\n";
    return 1;
  }

  // Build dimension
  Handle(AIS_Dimension) aDim;
  switch (aKindOfDimension)
  {
    case AIS_KOD_LENGTH:
    {
      if (!isPlaneCustom)
      {
        std::cerr << theArgs[0] << ": can not build dimension without working plane.\n";
        return 1;
      }
      if (aShapes.Extent() == 1)
      {
        if (aShapes.First()->Type() == AIS_KOI_Shape
            && (Handle(AIS_Shape)::DownCast(aShapes.First()))->Shape().ShapeType() != TopAbs_EDGE)
        {
          std::cerr << theArgs[0] << ": wrong shape type.\n";
          return 1;
        }
        aDim = new AIS_LengthDimension (TopoDS::Edge ((Handle(AIS_Shape)::DownCast(aShapes.First()))->Shape()), aWorkingPlane);
      }
      else if (aShapes.Extent() == 2)
      {
        if (aShapes.First()->Type() == AIS_KOI_Shape && aShapes.Last()->Type() == AIS_KOI_Shape)
          aDim = new AIS_LengthDimension ((Handle(AIS_Shape)::DownCast(aShapes.First ()))->Shape(),
                                          (Handle(AIS_Shape)::DownCast(aShapes.Last ()))->Shape(),
                                          aWorkingPlane);
        else// AIS_Point
        {
          Handle(AIS_Point) aPoint1 = Handle(AIS_Point)::DownCast(aShapes.First ());
          Handle(AIS_Point) aPoint2 = Handle(AIS_Point)::DownCast(aShapes.Last ());
          aDim = new AIS_LengthDimension (aPoint1->Component()->Pnt(),
                                          aPoint2->Component()->Pnt(),
                                          aWorkingPlane);
        }
      }
      else
      {
        std::cerr << theArgs[0] << ": wrong number of shapes to build dimension.\n";
        return 1;
      }

      break;
    }

    case AIS_KOD_PLANEANGLE:
    {
      if (aShapes.Extent() == 1 && aShapes.First()->Type()==AIS_KOI_Shape)
      {
        Handle(AIS_Shape) aShape = Handle(AIS_Shape)::DownCast(aShapes.First());
        if (aShape->Shape().ShapeType() == TopAbs_FACE)
          aDim = new AIS_AngleDimension (TopoDS::Face(aShape->Shape()));
      }
      if (aShapes.Extent() == 2)
      {
        Handle(AIS_Shape) aShape1 = Handle(AIS_Shape)::DownCast(aShapes.First());
        Handle(AIS_Shape) aShape2 = Handle(AIS_Shape)::DownCast(aShapes.Last());
        if (!aShape1.IsNull() && !aShape2.IsNull()
            && aShape1->Shape().ShapeType() == TopAbs_EDGE
            && aShape2->Shape().ShapeType() == TopAbs_EDGE)
          aDim = new AIS_AngleDimension (TopoDS::Edge(aShape1->Shape()),TopoDS::Edge(aShape2->Shape()));
        else
        {
          std::cerr << theArgs[0] << ": wrong shapes for angle dimension.\n";
          return 1;
        }
      }
      else if (aShapes.Extent() == 3)
      {
        gp_Pnt aP1, aP2, aP3;
        Handle(AIS_Point) aPoint = Handle(AIS_Point)::DownCast (aShapes.First());
        if (aPoint.IsNull())
          return 1;
        aP1 = aPoint->Component()->Pnt();
        aShapes.RemoveFirst();
        aPoint = Handle(AIS_Point)::DownCast (aShapes.First());
        if (aPoint.IsNull())
          return 1;
        aP2 = aPoint->Component()->Pnt();
        aShapes.RemoveFirst();
        aPoint = Handle(AIS_Point)::DownCast (aShapes.First());
        if (aPoint.IsNull())
          return 1;
        aP3 = aPoint->Component()->Pnt();
        aDim = new AIS_AngleDimension (aP1, aP2, aP3);
      }
      else
      {
        std::cerr << theArgs[0] << ": wrong number of shapes to build dimension.\n";
        return 1;
      }

      break;
    }

    case AIS_KOD_RADIUS: // radius of the circle
    {
      if (aShapes.Extent() == 1)
      {
        Handle(AIS_Circle) aShape = Handle(AIS_Circle)::DownCast (aShapes.First());
        gp_Circ aCircle = aShape->Circle()->Circ();
        aDim = new AIS_RadiusDimension (aCircle);
      }
      else
      {
        std::cerr << theArgs[0] << ": wrong number of shapes to build dimension.\n";
        return 1;
      }

      break;
    }

    case AIS_KOD_DIAMETER:
    {
      if (aShapes.Extent() == 1)
      {
        Handle(AIS_Circle) aShape = Handle(AIS_Circle)::DownCast (aShapes.First());
        gp_Circ aCircle = aShape->Circle()->Circ();
        aDim = new AIS_DiameterDimension (aCircle);
      }
      else
      {
        std::cerr << theArgs[0] << ": wrong number of shapes to build dimension.\n";
        return 1;
      }

      break;
    }

    default:
    {
      std::cerr << theArgs[0] << ": wrong type of dimension. Type help for more information.\n";
      return 1;
    }
  }

  aDim->SetDimensionAspect (anAspect);

  if (isCustomFlyout)
  {
    aDim->SetFlyout (aCustomFlyout);
  }

  if (GetMapOfAIS().IsBound2(aName))
  {
    theDi << theArgs[0] << ": shape with name " << aName.ToCString ()<< " already exists. It will be replaced\n";
    Handle(AIS_InteractiveObject) anObj = 
        Handle(AIS_InteractiveObject)::DownCast(GetMapOfAIS().Find2(aName));
    TheAISContext()->Remove(anObj, Standard_False);
    GetMapOfAIS().UnBind2(aName);
  }

  GetMapOfAIS().Bind (aDim,aName);

  return 0;
}

//=======================================================================
//function : VAngleDimBuilder
//purpose  : 
//=======================================================================

static int VAngleDimBuilder(Draw_Interpretor& di, Standard_Integer argc, const char** argv) 
{
  Standard_Integer aCurrentIndex;
  if (argc!=2)
  {
    di << argv[0] << " error : wrong number of parameters.\n";
    return 1;
  }

  TheAISContext()->CloseAllContexts();
  aCurrentIndex =  TheAISContext()->OpenLocalContext();
  // Set selection mode for edges.
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(2));
  di << "Select two edges coplanar or not.\n";

  Standard_Integer anArgsNum = 5;
  const char *aBuffer[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
  const char **anArgsVec = (const char **) aBuffer;
  while (ViewerMainLoop (anArgsNum, anArgsVec)) { }

  TopoDS_Shape aFirstShape;
  for (TheAISContext()->InitSelected(); TheAISContext()->MoreSelected(); TheAISContext()->NextSelected())
  {
    aFirstShape = TheAISContext()->SelectedShape();
  }

  if (aFirstShape.IsNull())
  {
    di << argv[0] << " error : no picked shape.\n";
    return 1;
  }

  if (aFirstShape.ShapeType()== TopAbs_EDGE)
  {
    while (ViewerMainLoop (anArgsNum, anArgsVec)) { }

    TopoDS_Shape aSecondShape;
    for (TheAISContext()->InitSelected(); TheAISContext()->MoreSelected(); TheAISContext()->NextSelected())
    {
      aSecondShape = TheAISContext()->SelectedShape();
    }

    if (aSecondShape.IsNull())
    {
      di << argv[0] << " error : no picked shape.\n";
      return 1;
    }

    if (aSecondShape.ShapeType() != TopAbs_EDGE)
    {
      di << argv[0] <<" error: you should have selected an edge.\n"; return 1;
    }

    // Close local context to draw dimension in the neutral point.
    TheAISContext()->CloseLocalContext (aCurrentIndex);

    // Construct the dimension.
    Handle (AIS_AngleDimension) aDim= new AIS_AngleDimension (TopoDS::Edge(aFirstShape) ,TopoDS::Edge(aSecondShape));
    VDisplayAISObject (argv[1], aDim);
  }
  else
  {
    di << argv[0] << " error: you must select 2 edges.\n";
    return 1;
  }

  return 0;
}

//==============================================================================
//function : VDiameterDim
//purpose  : Display the diameter dimension of a face or an edge.
//Draw arg : vdiameterdim Name 
//==============================================================================

static int VDiameterDimBuilder(Draw_Interpretor& di, Standard_Integer argc, const char** argv) 
{
  // Declarations
  Standard_Integer aCurrentIndex;
  Standard_Real aRadius;

  // Verification
  if (argc != 2)
  {
    di<<" vdiameterdim error"<<"\n";
    return 1;
  }

  // Close all local contexts
  TheAISContext()->CloseAllContexts();
  // Open local context and get its index for recovery
  TheAISContext()->OpenLocalContext();
  aCurrentIndex = TheAISContext()->IndexOfCurrentLocal();
  
  // Activate 'edge' selection mode
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(2) );
  di<<" Select an circled edge."<<"\n";
  
  // Loop that will handle the picking.
  Standard_Integer argcc = 5;
  const char *buff[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
  const char **argvv = (const char **) buff;
  while (ViewerMainLoop( argcc, argvv) ) { }
  // end of the loop.
  
  TopoDS_Shape aShape;
  for(TheAISContext()->InitSelected(); TheAISContext()->MoreSelected(); TheAISContext()->NextSelected())
  {
    aShape = TheAISContext()->SelectedShape();
  }

  if (aShape.IsNull())
  {
    di << argv[0] << ": no shape is selected." << "\n";
    return 1;
  }

  if (aShape.ShapeType() != TopAbs_EDGE)
  {
    di << " vdiameterdim error: the selection of a face or an edge was expected." << "\n";
    return 1;
  }

  // Compute the radius
  BRepAdaptor_Curve aCurve (TopoDS::Edge (aShape));

  if (aCurve.GetType() != GeomAbs_Circle)
  {
    di << "vdiameterdim error: the edge is not a circular one." << "\n";
    return 1;
  }

  gp_Circ aCircle = aCurve.Circle();
  aRadius = 2.0 * aCircle.Radius();

  // Construction of the diameter dimension.
  TheAISContext()->CloseLocalContext (aCurrentIndex);
  Handle (AIS_DiameterDimension) aDiamDim= new AIS_DiameterDimension (aShape);
  VDisplayAISObject (argv[1], aDiamDim);

  return 0;
}


//==============================================================================
// Fonction  vconcentric
// -----------------  Uniquement par selection dans le viewer.
//==============================================================================

//==============================================================================
//function : VConcentric
//purpose  : Display the concentric relation between two surfaces.
//Draw arg : vconcentric Name
//==============================================================================
#include <AIS_ConcentricRelation.hxx>
#include <Geom_Plane.hxx>
#include <gp_Pln.hxx>
#include <GC_MakePlane.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <TopExp_Explorer.hxx>


static int VConcentricBuilder(Draw_Interpretor& di, Standard_Integer argc, const char** argv) 
{
  // Declarations
  Standard_Integer myCurrentIndex;
  
  // Verification
  if (argc!=2) {di<<"vconcentric  error."<<"\n";return 1;}
  // Fermeture des contextes locaux
  TheAISContext()->CloseAllContexts();
  // Ouverture d'un contexte local et recuperation de son index.
  TheAISContext()->OpenLocalContext();
  myCurrentIndex=TheAISContext()->IndexOfCurrentLocal();
  
  // On active les modes de selections Edges et Faces.
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(2) );
  di<<" Select a circled edge."<<"\n";
  
  // Boucle d'attente waitpick.
  Standard_Integer argcc = 5;
  const char *buff[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
  const char **argvv = (const char **) buff;
  while (ViewerMainLoop( argcc, argvv) ) { }
  // fin de la boucle
  
  TopoDS_Shape ShapeA;
  for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
    ShapeA = TheAISContext()->SelectedShape();
  }
  // ShapeA est un edge
  // ==================
  if (ShapeA.ShapeType()==TopAbs_EDGE  ) {
    TheAISContext()->DeactivateStandardMode (AIS_Shape::SelectionType(4) );
    di<<" Select an edge."<<"\n";
    
    // Boucle d'attente waitpick.
    Standard_Integer argccc = 5;
    const char *bufff[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
    const char **argvvv = (const char **) bufff;
    while (ViewerMainLoop( argccc, argvvv) ) { }
    // fin de la boucle
    
    TopoDS_Shape ShapeB;
    for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
      ShapeB = TheAISContext()->SelectedShape();
    }
    if (ShapeB.ShapeType()!=TopAbs_EDGE  ) {
      di<<" vconcentric error: select an edge."<<"\n";return 1;
    }
     
    // Construction du plane.
    // On recupere le centre du cercle A.
    BRepAdaptor_Curve theCurveA(TopoDS::Edge(ShapeA) );
    gp_Circ theCircleA=theCurveA.Circle();
    gp_Pnt theCenterA=theCircleA.Location();
    // On recupere deux points sur le cercle A
    gp_Pnt B= theCurveA.Value(0.25);
    gp_Pnt C= theCurveA.Value(0.75);
    // Construction du plane.
    GC_MakePlane MkPlane(theCenterA ,B ,C );
    Handle(Geom_Plane) theGeomPlane=MkPlane.Value();
    
    // Fermeture du context local
    TheAISContext()->CloseLocalContext(myCurrentIndex);
    
    Handle (AIS_ConcentricRelation) myConcentry= new AIS_ConcentricRelation(ShapeA, ShapeB, theGeomPlane );
    TheAISContext()->Display(myConcentry );
    GetMapOfAIS().Bind (myConcentry,argv[1]);
  }
  
  
  else {
    di<<" vconcentric  error: the selection of a face or an edge was expected."<<"\n";return 1;
  }
  
  return 0;
  
  
  
  
}

//==============================================================================
//function : VEqualDistRelation
//purpose  : 
//Draw arg : vdiameterdim Name DiameterValue
//==============================================================================
#include <AIS_EqualDistanceRelation.hxx>
#include <BRepExtrema_ExtCC.hxx>
#include <GC_MakePlane.hxx>


static int VEqualDistRelation(Draw_Interpretor& di, Standard_Integer argc, const char** argv) 
{
  // Declarations
  Standard_Integer myCurrentIndex;
  
  // Verification
  if (argc!=2) {di<<" vequaldistrelation error: no arguments allowed."<<"\n";return 1;}
  
  // Fermeture des contextes locaux
  TheAISContext()->CloseAllContexts();
  
  // Ouverture d'un contexte local et recuperation de son index.
  TheAISContext()->OpenLocalContext();
  myCurrentIndex=TheAISContext()->IndexOfCurrentLocal();
  
  // On active les modes de selections Edges et Vertexes.
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(2) );
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(1) );
  di<<" Select an edge or a vertex"<<"\n";
  
  // Boucle d'attente waitpick.
  Standard_Integer argc1 = 5;
  const char *buf1[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
  const char **argv1 = (const char **) buf1;
  while (ViewerMainLoop( argc1, argv1) ) { }
  // fin de la boucle
  
  TopoDS_Shape ShapeA;
  for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
    ShapeA = TheAISContext()->SelectedShape();
  }
  
  di<<" Select an edge or a vertex"<<"\n";
  // Boucle d'attente waitpick.
  Standard_Integer argc2 = 5;
  const char *buf2[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
  const char **argv2 = (const char **) buf2;
  while (ViewerMainLoop( argc2, argv2) ) { }
  // fin de la boucle
  
  TopoDS_Shape ShapeB;
  for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
    ShapeB = TheAISContext()->SelectedShape();
  }
  
  // Verification des resultats.
  if (ShapeA.ShapeType()==TopAbs_EDGE && ShapeB.ShapeType()==TopAbs_EDGE  ) {
    // A et B sont des edges ils doivent etre paralleles
    BRepExtrema_ExtCC myDeltaEdge (TopoDS::Edge(ShapeA) ,TopoDS::Edge(ShapeB)  );
    // on verifie qu'ils sont pas paralleles.
    if (!myDeltaEdge.IsParallel() ) {di<<"vequaldist error: non parallel edges."<<"\n";return 1; }
    
  }
  
  
  di<<" Select an edge or a vertex"<<"\n";
  // Boucle d'attente waitpick.
  Standard_Integer argc3 = 5;
  const char *buf3[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
  const char **argv3 = (const char **) buf3;
  while (ViewerMainLoop( argc3, argv3) ) { }
  // fin de la boucle
  
  TopoDS_Shape ShapeC;
  for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
    ShapeC = TheAISContext()->SelectedShape();
  }
  
  di<<" Select an edge or a vertex"<<"\n";
  // Boucle d'attente waitpick.
  Standard_Integer argc4 = 5;
  const char *buf4[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
  const char **argv4 = (const char **) buf4;
  while (ViewerMainLoop( argc4, argv4) ) { }
  // fin de la boucle
  
  TopoDS_Shape ShapeD;
  for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
    ShapeD = TheAISContext()->SelectedShape();
  }
  // Verification des resultats.
  if (ShapeC.ShapeType()==TopAbs_EDGE && ShapeD.ShapeType()==TopAbs_EDGE  ) {
    // C et D sont des edges ils doivent etre paralleles
    BRepExtrema_ExtCC myDeltaEdge2 (TopoDS::Edge(ShapeC) ,TopoDS::Edge(ShapeD)  );
    // on verifie qu'ils sont pas paralleles.
    if (!myDeltaEdge2.IsParallel() ) {di<<"vequaldist error: non parallel edges."<<"\n";return 1; }
    
  }
  
  // Creation du plan porteur de la contrainte.Methode lourde!
  gp_Pnt A,B,C;
  if (ShapeA.ShapeType()==TopAbs_EDGE) {
    // A est un edge
    TopoDS_Vertex Va,Vb;
    TopExp::Vertices (TopoDS::Edge(ShapeA) ,Va ,Vb );
    A=BRep_Tool::Pnt(Va);
    B=BRep_Tool::Pnt(Vb);
    
    if (ShapeB.ShapeType()==TopAbs_EDGE) {
      // B est un edge aussi
      TopoDS_Vertex Vc,Vd;
      TopExp::Vertices (TopoDS::Edge(ShapeB) ,Vc ,Vd );
      // besoin que de 1 point.
      C=BRep_Tool::Pnt(Vc);
      
    }
    else {
      // B est un vertex
      C=BRep_Tool::Pnt(TopoDS::Vertex(ShapeB) );
    }
  }
  else {
    // A est un vertex
    A=BRep_Tool::Pnt(TopoDS::Vertex(ShapeA) );
    
    if (ShapeB.ShapeType()==TopAbs_EDGE ) {
      // B est un edge 
      TopoDS_Vertex Vb,Vc;
      TopExp::Vertices (TopoDS::Edge(ShapeB) ,Vb ,Vc );
      // besoin que de 2 points.
      B=BRep_Tool::Pnt(Vb);
      C=BRep_Tool::Pnt(Vc);
      
    }
    else {
      // B est un vertex
      B=BRep_Tool::Pnt(TopoDS::Vertex(ShapeB) );
      C.SetX(B.X()+5.);
      C.SetY(B.Y()+5.);
      C.SetZ(B.Z()+5.);
      
    }
  }
  
  // Fermeture du context local.
  TheAISContext()->CloseLocalContext(myCurrentIndex);
  
  // construction du plane 
  GC_MakePlane MkPlane(A ,B ,C );
  Handle(Geom_Plane) theGeomPlane=MkPlane.Value();
  
  // Construction de l'AIS_EqualDistanceRelation
  Handle (AIS_EqualDistanceRelation ) myRelation= new AIS_EqualDistanceRelation (ShapeA, ShapeB, ShapeC ,ShapeD , theGeomPlane );
  TheAISContext()->Display(myRelation );
  GetMapOfAIS().Bind (myRelation,argv[1]);
  

  
  return 0;
  
}

//==============================================================================
//function : VEqualRadiusRelation
//purpose  : 
//Draw arg : vdiameterdim Name DiameterValue
//==============================================================================
#include <AIS_EqualRadiusRelation.hxx>
#include <GC_MakePlane.hxx>
#include <BRepAdaptor_Curve.hxx>


static int VEqualRadiusRelation(Draw_Interpretor& di, Standard_Integer argc, const char** argv) 
{
  // Declarations
  Standard_Integer myCurrentIndex;
  
  // Verification
  if (argc!=2) {di<<" vequalrad error: no arguments allowed."<<"\n";return 1;}
  
  // Fermeture des contextes locaux
  TheAISContext()->CloseAllContexts();
  
  // Ouverture d'un contexte local et recuperation de son index.
  TheAISContext()->OpenLocalContext();
  myCurrentIndex=TheAISContext()->IndexOfCurrentLocal();
  
  // On active les modes de selections Edges.
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(2) );
  di<<" Select an circled edge "<<"\n";
  
  // Boucle d'attente waitpick.
  Standard_Integer argc1 = 5;
  const char *buf1[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
  const char **argv1 = (const char **) buf1;
  while (ViewerMainLoop( argc1, argv1) ) { }
  // fin de la boucle
  
  TopoDS_Shape ShapeA;
  for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
    ShapeA = TheAISContext()->SelectedShape();
  }
  
  di<<" Select the last circled edge."<<"\n";
  // Boucle d'attente waitpick.
  Standard_Integer argc2 = 5;
  const char *buf2[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
  const char **argv2 = (const char **) buf2;
  while (ViewerMainLoop( argc2, argv2) ) { }
  // fin de la boucle
  
  TopoDS_Shape ShapeB;
  for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
    ShapeB = TheAISContext()->SelectedShape();
  }
  // creation du plan qui contient la contrainte.
  TopoDS_Edge EdgeA=TopoDS::Edge(ShapeA);
  TopoDS_Edge EdgeB=TopoDS::Edge(ShapeB);
  BRepAdaptor_Curve theCurveA(EdgeA);
  // On recupere 3 points A,B,C de la curve.
  gp_Pnt A=theCurveA.Value(0.1);
  gp_Pnt B=theCurveA.Value(0.5);
  gp_Pnt C=theCurveA.Value(0.9);
  
  // fermeture du contexte local.
  TheAISContext()->CloseLocalContext(myCurrentIndex);
  
  // Creation du plane.
  GC_MakePlane MkPlane (A ,B ,C );
  Handle(Geom_Plane) theGeomPlane=MkPlane.Value();
  // Construction de l'AIS_EqualRadiusRelation
  Handle (AIS_EqualRadiusRelation ) myRelation= new AIS_EqualRadiusRelation (EdgeA,EdgeB, theGeomPlane );
  TheAISContext()->Display(myRelation );
  GetMapOfAIS().Bind (myRelation,argv[1]);
  
  return 0;
  
}


//==============================================================================
//function : VFixRelation
//purpose  : 
//Draw arg : vdiameterdim Name DiameterValue
//==============================================================================
#include <AIS_FixRelation.hxx>
#include <GC_MakePlane.hxx>
#include <BRepAdaptor_Curve.hxx>

static int VFixRelation(Draw_Interpretor& di, Standard_Integer argc, const char** argv) 
{
  // Declarations
  Standard_Integer myCurrentIndex;
  
  // Verification
  if (argc!=2) {di<<" vfix  error: no arguments allowed."<<"\n";return 1;}
  
  // Fermeture des contextes locaux
  TheAISContext()->CloseAllContexts();
  
  // Ouverture d'un contexte local et recuperation de son index.
  TheAISContext()->OpenLocalContext();
  myCurrentIndex=TheAISContext()->IndexOfCurrentLocal();
  
  // On active les modes de selections edge.
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(2) );
  di<<" Select an edge. "<<"\n";
  
  // Boucle d'attente waitpick.
  Standard_Integer argc1 = 5;
  const char *buf1[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
  const char **argv1 = (const char **) buf1;
  while (ViewerMainLoop( argc1, argv1) ) { }
  // fin de la boucle
  
  TopoDS_Shape ShapeA;
  for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
    ShapeA = TheAISContext()->SelectedShape();
  }
  
  // creation du plan qui contient la contrainte.
  TopoDS_Edge EdgeA=TopoDS::Edge(ShapeA);
  BRepAdaptor_Curve theCurveA(EdgeA);
  // On recupere 3 points A,B,C de la curve.
  gp_Pnt A=theCurveA.Value(0.1);
  gp_Pnt B=theCurveA.Value(0.5);
  gp_Pnt D=theCurveA.Value(0.9);
  gp_Pnt C(B.X()+5,B.Y()+5,B.Z()+5);
  
  // fermeture du contexte local.
  TheAISContext()->CloseLocalContext(myCurrentIndex);
  
  // Creation du plane.
  GC_MakePlane MkPlane (A ,D ,C );
  Handle(Geom_Plane) theGeomPlane=MkPlane.Value();
  // Construction de l'AIS_EqualRadiusRelation
  Handle (AIS_FixRelation) myRelation= new AIS_FixRelation (EdgeA,theGeomPlane );
  TheAISContext()->Display(myRelation );
  GetMapOfAIS().Bind (myRelation,argv[1]);
  
  
  return 0;
  
}

//==============================================================================
//function : VIdenticRelation
//purpose  : 
//Draw arg : vdiameterdim Name DiameterValue
//==============================================================================
#include <AIS_IdenticRelation.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <TopExp_Explorer.hxx>


static int VIdenticRelation(Draw_Interpretor& di, Standard_Integer argc, const char** argv) 
{
  // Declarations
  Standard_Integer myCurrentIndex;
  
  // Verification
  if (argc!=2) {di<<" videntity error: no arguments allowed."<<"\n";return 1;}
  
  // Fermeture des contextes locaux
  TheAISContext()->CloseAllContexts();
  
  // Ouverture d'un contexte local et recuperation de son index.
  TheAISContext()->OpenLocalContext();
  myCurrentIndex=TheAISContext()->IndexOfCurrentLocal();
  
  // On active les modes de selections  vertex et face.
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(2) );
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(1) );
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(4) );
  di<<" Select an edge, a face or a vertex. "<<"\n";
  
  // Boucle d'attente waitpick.
  Standard_Integer argc1 = 5;
  const char *buf1[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
  const char **argv1 = (const char **) buf1;
  while (ViewerMainLoop( argc1, argv1) ) { }
  // fin de la boucle
  
  TopoDS_Shape ShapeA;
  for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
    ShapeA = TheAISContext()->SelectedShape();
  }
  
  di<<" Select an edge, a face or a vertex. "<<"\n";
  // Boucle d'attente waitpick.
  Standard_Integer argc2 = 5;
  const char *buf2[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
  const char **argv2 = (const char **) buf2;
  while (ViewerMainLoop( argc2, argv2) ) { }
  // fin de la boucle
  
  TopoDS_Shape ShapeB;
  for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
    ShapeB = TheAISContext()->SelectedShape();
  }
  
  // Recuperation de points pour construir le Geom_Plnae.
  gp_Pnt A,B,C;
  if (ShapeA.ShapeType()==TopAbs_EDGE) {
    // A est un edge; on peut supposer qu'il sera element d'une face ou possesseur d'un vertex ou identic a un autre edge.
    // on recupere deux points sur l'edge (il peut etre rectiligne)
    TopoDS_Edge EdgeA=TopoDS::Edge(ShapeA);
    BRepAdaptor_Curve theCurveA(EdgeA);
    // Creation des 3 points.
    A=theCurveA.Value(0.1);
    B=theCurveA.Value(0.9);
    C.SetX(B.X()+5.);
    C.SetY(B.Y()+5.);
    C.SetZ(B.Z()+5.);
  }
  else if (ShapeA.ShapeType()==TopAbs_VERTEX ) {
    // SahpeA est un Vertex
    // On va utiliser ShapeB
    if (ShapeB.ShapeType()==TopAbs_EDGE) {
      // B est un edge 
      TopoDS_Edge EdgeB=TopoDS::Edge(ShapeB);
      BRepAdaptor_Curve theCurveB(EdgeB);
      // Creation des 3 points.
      A=theCurveB.Value(0.1);
      B=theCurveB.Value(0.9);
      C.SetX(B.X()+5.);
      C.SetY(B.Y()+5.);
      C.SetZ(B.Z()+5.);
      
    }
    else if (ShapeB.ShapeType()==TopAbs_FACE ) {
      // Shape B est une face
      TopoDS_Face  FaceB=TopoDS::Face(ShapeB);
      // On recupere 1 edge de FaceB(la face n'a pas forcement de vertex) (l'element A est forcement dans B).
      TopExp_Explorer FaceExp(FaceB,TopAbs_EDGE);
      TopoDS_Edge EdgeFromB=TopoDS::Edge(FaceExp.Current() );
      // On recupere les 3 points de l'edge de face B
      BRepAdaptor_Curve theCurveB(EdgeFromB);
      // On recupere 3 points A,B,C de la curve.
      A=theCurveB.Value(0.1);
      B=theCurveB.Value(0.5);
      C=theCurveB.Value(0.9);
      
    }
    else {
      // B ets un vetex aussi
      A=BRep_Tool::Pnt(TopoDS::Vertex(ShapeA) );
      B=BRep_Tool::Pnt(TopoDS::Vertex(ShapeB) );
      C.SetX(B.X()+5.);
      C.SetY(B.Y()+5.);
      C.SetZ(B.Z()+5.);
      
    }
    
  }
  else {
    // A est une face.
    TopoDS_Face  FaceA=TopoDS::Face(ShapeA);
    // On recupere 1 edge de FaceA
    TopExp_Explorer FaceExp(FaceA,TopAbs_EDGE);
    TopoDS_Edge EdgeFromA=TopoDS::Edge(FaceExp.Current() );
    // On recupere les 3 points de l'edge
    BRepAdaptor_Curve theCurveA(EdgeFromA);
    // On recupere 3 points A,B,C de la curve.
    A=theCurveA.Value(0.1);
    B=theCurveA.Value(0.5);
    C=theCurveA.Value(0.9);
    
  }
  
  // Fermeture du context local.
  TheAISContext()->CloseLocalContext(myCurrentIndex);
  // On construit le plane 
  GC_MakePlane MkPlane (A ,B ,C );
  Handle(Geom_Plane) theGeomPlane=MkPlane.Value();
  
  // Construction de l'AIS_IdenticRelation
  Handle ( AIS_IdenticRelation ) myRelation= new AIS_IdenticRelation  (ShapeA ,ShapeB, theGeomPlane );
  TheAISContext()->Display(myRelation );
  GetMapOfAIS().Bind (myRelation,argv[1]);
  

  
  return 0;
  
}
//==============================================================================
//function : VLengthDimension
//purpose  : Display the diameter dimension of a face or an edge.
//Draw arg : vdiameterdim Name DiameterValue
//==============================================================================
#include <AIS_LengthDimension.hxx>
#include <BRepExtrema_ExtCC.hxx>
#include <BRepExtrema_ExtPC.hxx>
#include <BRepExtrema_ExtCF.hxx>
#include <BRepExtrema_ExtPF.hxx>
#include <BRepExtrema_ExtFF.hxx>
#include <TCollection_ExtendedString.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <gce_MakePln.hxx>
#include <TopExp_Explorer.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>

static int VLenghtDimension(Draw_Interpretor& di, Standard_Integer argc, const char** argv) 
{
  // Declarations
  Standard_Integer aCurrentIndex;
  // Verification
  if (argc != 2)
  {
    di << argv[0] << " error: wrong number of arguments.\n";
    return 1;
  }

  // Close all local contexts
  TheAISContext()->CloseAllContexts();

  // Open local context
  aCurrentIndex = TheAISContext()->OpenLocalContext();
  // Activate 'edge', 'face' and 'vertex' selection modes.
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(2));
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(1));
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(4));

  // First shape picking
  di << " Select an edge, a face or a vertex. " << "\n";
  // Loop that will handle the picking.
  Standard_Integer argc1 = 5;
  const char *buf1[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
  const char **argv1 = (const char **) buf1;
  while (ViewerMainLoop( argc1, argv1) ) { }
  // end of the loop.

  TopoDS_Shape aFirstShape;
  for(TheAISContext()->InitSelected(); TheAISContext()->MoreSelected(); TheAISContext()->NextSelected())
  {
    aFirstShape = TheAISContext()->SelectedShape();
  }

  if (aFirstShape.IsNull())
  {
    di << argv[0] << "error: no first picked shape.\n";
    return 1;
  }

  // Second shape picking
  di << " Select an edge, a face or a vertex. " << "\n";
  // Loop that will handle the picking.
  Standard_Integer argc2 = 5;
  const char *buf2[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
  const char **argv2 = (const char **) buf2;
  while (ViewerMainLoop( argc2, argv2) ) { }

  TopoDS_Shape aSecondShape;
  for(TheAISContext()->InitSelected(); TheAISContext()->MoreSelected(); TheAISContext()->NextSelected())
  {
    aSecondShape = TheAISContext()->SelectedShape();
  }

  if (aSecondShape.IsNull())
  {
    di << argv[0] << "error: no second picked shape.\n";
    return 1;
  }

  if (aFirstShape.ShapeType() == TopAbs_EDGE)
  {
    TopoDS_Edge EdgeA = TopoDS::Edge (aFirstShape);

    if (aSecondShape.ShapeType() == TopAbs_EDGE)
    {
      TopoDS_Edge EdgeB = TopoDS::Edge (aSecondShape);
      BRepExtrema_ExtCC myDeltaEdge (EdgeA ,EdgeB);

      if (!myDeltaEdge.IsParallel())
      {
        di << argv[0] << " error: non parallel edges." << "\n";
        return 1;
      }

      // 3 points of edges is recovered to build a plane
      TopoDS_Vertex aVertex1, aVertex2, aVertex3, aVertex4;
      TopExp::Vertices (EdgeA, aVertex1, aVertex2);
      TopExp::Vertices (EdgeB, aVertex3, aVertex4);
      gp_Pnt A = BRep_Tool::Pnt (aVertex1);
      gp_Pnt B = BRep_Tool::Pnt (aVertex2);
      gp_Pnt C = BRep_Tool::Pnt (aVertex3);

      gce_MakePln aMakePlane (A,B,C);
      gp_Pln aPlane= aMakePlane.Value();

      // Close local context
      TheAISContext()->CloseLocalContext (aCurrentIndex);

      // Construct the dimension
      Handle(AIS_LengthDimension ) aLenghtDim = new AIS_LengthDimension (EdgeA, EdgeB, aPlane);
      TheAISContext()->Display (aLenghtDim);
      GetMapOfAIS().Bind (aLenghtDim, argv[1]);
    }

    else if (aSecondShape.ShapeType() == TopAbs_VERTEX)
    {
      TopoDS_Vertex aVertex = TopoDS::Vertex (aSecondShape);
      BRepExtrema_ExtPC myDeltaEdgeVertex  (aVertex ,EdgeA);

      TopoDS_Vertex aVertex1, aVertex2;
      TopExp::Vertices (EdgeA, aVertex1, aVertex2);
      gp_Pnt A=BRep_Tool::Pnt (aVertex1);
      gp_Pnt B=BRep_Tool::Pnt (aVertex2);
      gp_Pnt C=BRep_Tool::Pnt (aVertex);

      gce_MakePln aMakePlane (A,B,C);
      gp_Pln aPlane= aMakePlane.Value();

      TheAISContext()->CloseLocalContext (aCurrentIndex);
      Handle(AIS_LengthDimension) aLenghtDim=new AIS_LengthDimension (EdgeA, aVertex, aPlane);
      TheAISContext()->Display (aLenghtDim);
      GetMapOfAIS().Bind (aLenghtDim, argv[1]);
    }

    // Second shape is a face
    else
    {
      TopoDS_Face FaceB = TopoDS::Face (aSecondShape);
      BRepExtrema_ExtCF aDeltaEdgeFace (EdgeA,FaceB);

      if (!aDeltaEdgeFace.IsParallel())
      {
        di << argv[0] << "error: the edge isn't parallel to the face;can't compute the distance." << "\n";
        return 1;
      }

      Handle(AIS_LengthDimension) aLenghtDim = new AIS_LengthDimension (FaceB, EdgeA);
      TheAISContext()->Display (aLenghtDim);
      GetMapOfAIS().Bind (aLenghtDim, argv[1]);
    }
  }
  else if (aFirstShape.ShapeType() == TopAbs_VERTEX)
  {
    TopoDS_Vertex  VertexA = TopoDS::Vertex (aFirstShape);
    if (aSecondShape.ShapeType() == TopAbs_EDGE )
    {
      TopoDS_Edge  EdgeB=TopoDS::Edge (aSecondShape);
      BRepExtrema_ExtPC aDeltaEdgeVertex (VertexA, EdgeB);

      TopoDS_Vertex aVertex1, aVertex2;
      TopExp::Vertices(EdgeB, aVertex1, aVertex2);
      gp_Pnt A = BRep_Tool::Pnt (aVertex1);
      gp_Pnt B = BRep_Tool::Pnt (aVertex2);
      gp_Pnt C = BRep_Tool::Pnt (VertexA);

      gce_MakePln aMakePlane (A,B,C);
      gp_Pln aPlane = aMakePlane.Value();

      // Close local contex by its index.
      TheAISContext()->CloseLocalContext (aCurrentIndex);

      // Construct the dimension.
      Handle(AIS_LengthDimension) aLenghtDim = new AIS_LengthDimension (EdgeB,VertexA, aPlane);
      TheAISContext()->Display (aLenghtDim);
      GetMapOfAIS().Bind (aLenghtDim, argv[1]);
    }

    else if (aSecondShape.ShapeType() == TopAbs_VERTEX)
    {
      TopoDS_Vertex  VertexB = TopoDS::Vertex (aSecondShape);

      gp_Pnt A = BRep_Tool::Pnt (VertexA);
      gp_Pnt B = BRep_Tool::Pnt (VertexB);
      gp_Pnt C(B.X() + 10.0, B.Y() + 10.0, B.Z() + 10.0);

      gce_MakePln aMakePlane (A,B,C);
      gp_Pln aPlane= aMakePlane.Value();

      TheAISContext()->CloseLocalContext (aCurrentIndex);

      Handle(AIS_LengthDimension ) aLenghtDim = new AIS_LengthDimension (VertexA, VertexB, aPlane);
      TheAISContext()->Display (aLenghtDim);
      GetMapOfAIS().Bind (aLenghtDim, argv[1]);
    }
    // The second shape is face
    else
    {
      TopoDS_Face  FaceB = TopoDS::Face (aSecondShape);

      BRepExtrema_ExtPF aDeltaVertexFace (VertexA, FaceB);

      gp_Pnt A = BRep_Tool::Pnt (VertexA);

      // Recover edge from face.
      TopExp_Explorer aFaceExp (FaceB,TopAbs_EDGE);
      TopoDS_Edge aSecondEdge = TopoDS::Edge (aFaceExp.Current());

      TopoDS_Vertex aVertex1, aVertex2;
      TopExp::Vertices (aSecondEdge, aVertex1, aVertex2);
      gp_Pnt C = BRep_Tool::Pnt (aVertex2);

      gp_Pnt aProjA = aDeltaVertexFace.Point(1);
      BRepBuilderAPI_MakeVertex aVertexMaker (aProjA);
      TopoDS_Vertex aVertexAProj = aVertexMaker.Vertex();

      // Create working plane for the dimension.
      gce_MakePln aMakePlane (A, aProjA, C);
      gp_Pln aPlane = aMakePlane.Value();

      TheAISContext()->CloseLocalContext (aCurrentIndex);

      // Construct the dimension.
      Handle(AIS_LengthDimension ) aLenghtDim = new AIS_LengthDimension (VertexA, aVertexAProj, aPlane);
      TheAISContext()->Display (aLenghtDim);
      GetMapOfAIS().Bind (aLenghtDim, argv[1]);
    }
  }

  // The first shape is a face.
  else
  {
    TopoDS_Face FaceA = TopoDS::Face (aFirstShape);

    if (aSecondShape.ShapeType() == TopAbs_EDGE)
    {
      TopoDS_Edge EdgeB = TopoDS::Edge (aSecondShape);
      BRepExtrema_ExtCF aDeltaEdgeFace (EdgeB,FaceA );

      if (!aDeltaEdgeFace.IsParallel())
      {
        di << argv[0] << " error: the edge isn't parallel to the face;can't compute the distance. " << "\n";
        return 1;
      }

      Handle(AIS_LengthDimension) aLenghtDim = new AIS_LengthDimension (FaceA, EdgeB);
      TheAISContext()->Display (aLenghtDim);
      GetMapOfAIS().Bind (aLenghtDim, argv[1]);
    }

    else if (aSecondShape.ShapeType() == TopAbs_VERTEX)
    {
      TopoDS_Vertex  VertexB = TopoDS::Vertex (aSecondShape);
      BRepExtrema_ExtPF aDeltaVertexFace (VertexB, FaceA);

      gp_Pnt B = BRep_Tool::Pnt (VertexB);

      TopExp_Explorer aFaceExp (FaceA, TopAbs_EDGE);
      TopoDS_Edge anEdgeFromA = TopoDS::Edge (aFaceExp.Current());
      TopoDS_Vertex  aVertex1, aVertex2;
      TopExp::Vertices(anEdgeFromA, aVertex1, aVertex2);
      gp_Pnt A=BRep_Tool::Pnt(aVertex1);

#ifdef DEB
      gp_Pnt C = BRep_Tool::Pnt(aVertex2);
#endif

      gp_Pnt aProjB = aDeltaVertexFace.Point(1);
      BRepBuilderAPI_MakeVertex aVertexMaker (aProjB);
      TopoDS_Vertex aVertexBProj = aVertexMaker.Vertex();
      gce_MakePln aMakePlane (A, B, aProjB);
      gp_Pln aPlane= aMakePlane.Value();

      TheAISContext()->CloseLocalContext(aCurrentIndex);

      Handle(AIS_LengthDimension) aLenghtDim  =new AIS_LengthDimension (VertexB, aVertexBProj, aPlane);
      TheAISContext()->Display (aLenghtDim);
      GetMapOfAIS().Bind (aLenghtDim, argv[1]);
    }
    // the second shape is face.
    else
    {
      TopoDS_Face FaceB = TopoDS::Face (aSecondShape);
      BRepExtrema_ExtFF aDeltaFaceFace (FaceA, FaceB);

      if (!aDeltaFaceFace.IsParallel())
      {
        di << argv[0] << " error: the faces are not parallel. "<<"\n";
        return 1;
      }

      TheAISContext()->CloseLocalContext (aCurrentIndex);

      Handle(AIS_LengthDimension) aLenghtDim = new AIS_LengthDimension (FaceA,FaceB);
      TheAISContext()->Display (aLenghtDim);
      GetMapOfAIS().Bind (aLenghtDim, argv[1]);
    }
  }

  return 0;
}


//==============================================================================
//function : VRadiusDim
//purpose  : Display the radius dimension of a face or an edge.
//Draw arg : vradiusdim Name 
//==============================================================================
#include <AIS_RadiusDimension.hxx>
#include <TCollection_ExtendedString.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <gp_Circ.hxx>


static int VRadiusDimBuilder(Draw_Interpretor& di, Standard_Integer argc, const char** argv) 
{
  // Declarations
  Standard_Integer aCurrentIndex;
  Standard_Real aRadius;
  TopoDS_Edge anEdge;
  // Verification
  if (argc != 2)
  {
    di << argv[0] << " error: wrong number of parameters." << "\n";
    return 1;
  }

  // Close all local contexts
  TheAISContext()->CloseAllContexts();

  // Open local context and get its index for recovery.
  TheAISContext()->OpenLocalContext();
  aCurrentIndex = TheAISContext()->IndexOfCurrentLocal();

  // Current selection modes - faces and edges
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(2));
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(4));
  di << " Select a circled edge or face." << "\n";

  // Loop that will be handle picking.
  Standard_Integer argcc = 5;
  const char *buff[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
  const char **argvv = (const char **) buff;
  while (ViewerMainLoop (argcc, argvv)) { }
  // end of the loop

  TopoDS_Shape aShape;

  for (TheAISContext()->InitSelected(); TheAISContext()->MoreSelected(); TheAISContext()->NextSelected() )
  {
    aShape = TheAISContext()->SelectedShape();
  }

  if (aShape.IsNull())
  {
    di << argv[0] << ": no shape is selected." << "\n";
    return 1;
  }

  if (aShape.ShapeType() != TopAbs_EDGE && aShape.ShapeType() != TopAbs_FACE)
  {
    di << argv[0] << " error: the selection of a face or an edge was expected." << "\n";
    return 1;
  }

  if (aShape.ShapeType() == TopAbs_EDGE)
  {
    anEdge = TopoDS::Edge (aShape);
  }
  else // Face
  {
    // Recover an edge of the face.
    TopoDS_Face aFace = TopoDS::Face (aShape);

    TopExp_Explorer aFaceExp (aFace,TopAbs_EDGE);
    anEdge = TopoDS::Edge (aFaceExp.Current());
  }

  // Compute the radius
  BRepAdaptor_Curve aCurve (anEdge);
  if (aCurve.GetType() != GeomAbs_Circle)
  {
    di << argv[0] << " error: the edge is not a circular one." << "\n";
    return 1;
  }
  else
  {
    gp_Circ aCircle = aCurve.Circle();
    aRadius = aCircle.Radius();
    aRadius = Round (aRadius * 10.0) / 10.0;
  }
  // Close the context
  TheAISContext()->CloseLocalContext (aCurrentIndex);

  // Construct radius dimension
  Handle (AIS_RadiusDimension) aRadDim= new AIS_RadiusDimension (aShape);
  VDisplayAISObject (argv[1], aRadDim);

  return 0;
}



//==============================================================================
//function : VOffsetDim
//purpose  : Display the offset dimension
//Draw arg : voffsetdim Name 
//==============================================================================
#include <AIS_OffsetDimension.hxx>
#include <TCollection_ExtendedString.hxx>
#include <BRepExtrema_ExtFF.hxx>


static int VOffsetDimBuilder(Draw_Interpretor& di, Standard_Integer argc, const char** argv) 
{
  // Declarations
  Standard_Integer myCurrentIndex;
  Standard_Real    theDist;
  
  // Verification
  if (argc!=2) {di<<" voffsetdim error"<<"\n";return 1;}
  
  // Fermeture des contextes locaux
  TheAISContext()->CloseAllContexts();
  
  // Ouverture d'un contexte local et recuperation de son index.
  TheAISContext()->OpenLocalContext();
  myCurrentIndex=TheAISContext()->IndexOfCurrentLocal();
  
  // On active les modes de selections Faces.
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(4) );
  di<<" Select a face."<<"\n";
  
  // Boucle d'attente waitpick.
  Standard_Integer argcc = 5;
  const char *buff[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
  const char **argvv = (const char **) buff;
  while (ViewerMainLoop( argcc, argvv) ) { }
  // fin de la boucle
  
  TopoDS_Shape ShapeA;
  for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
    ShapeA = TheAISContext()->SelectedShape();
  }
  
  di<<" Select a face."<<"\n";
  // Boucle d'attente waitpick.
  Standard_Integer argccc = 5;
  const char *bufff[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
  const char **argvvv = (const char **) bufff;
  while (ViewerMainLoop( argccc, argvvv) ) { }
  // fin de la boucle
  
  TopoDS_Shape ShapeB;
  for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
    ShapeB = TheAISContext()->SelectedShape();
  }
  
  
  // Shape A et B est une face
  if (ShapeA.ShapeType()==TopAbs_FACE && ShapeB.ShapeType()==TopAbs_FACE ) {
    
    TopoDS_Face  FaceA=TopoDS::Face(ShapeA);
    TopoDS_Face  FaceB=TopoDS::Face(ShapeB);
    
    BRepExtrema_ExtFF myDeltaFaceFace  (FaceA ,FaceB );
    // On verifie que les deux faces sont bien parelles.
    if (!myDeltaFaceFace.IsParallel() ) {di<<"vdistdim error: the faces are not parallel. "<<"\n";return 1; }
    
    // On saisit la distance et on l'arrondit!
    theDist=Round (sqrt (myDeltaFaceFace.SquareDistance(1))*10. )/10.;
    // Fermeture du contexte local.
    TheAISContext()->CloseLocalContext(myCurrentIndex);
    // Construction du texte.
    TCollection_ExtendedString TheMessage_Str(TCollection_ExtendedString("offset=")+TCollection_ExtendedString(theDist ) );
    
    // on construit l'AIS_OffsetDimension
    Handle(AIS_OffsetDimension) myOffsetDim=new AIS_OffsetDimension (FaceA,FaceB,theDist,TheMessage_Str );
    TheAISContext()->Display(myOffsetDim );
    GetMapOfAIS().Bind (myOffsetDim ,argv[1]);
    
    
    
  }
  
  else {
    di<<" voffsetdim error: the selection of a face was expected."<<"\n";return 1;
  }
  
  return 0;
  
}




//==============================================================================
//function : VParallel
//purpose  : Display the parallel relation 
//Draw arg : vparallel Name 
//==============================================================================
#include <AIS_ParallelRelation.hxx>
#include <TCollection_ExtendedString.hxx>
#include <BRepExtrema_ExtFF.hxx>
#include <BRepExtrema_ExtCC.hxx>
#include <GC_MakePlane.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <TopExp_Explorer.hxx>


static int VParallelBuilder(Draw_Interpretor& di, Standard_Integer argc, const char** argv) 
{
  // Declarations
  Standard_Integer myCurrentIndex;
 
  // Verification
  if (argc!=2) {di<<" vparallel error"<<"\n";return 1;}
  
  // Fermeture des contextes locaux
  TheAISContext()->CloseAllContexts();
  
  // Ouverture d'un contexte local et recuperation de son index.
  TheAISContext()->OpenLocalContext();
  myCurrentIndex=TheAISContext()->IndexOfCurrentLocal();
  
  // On active les modes de selections Edges.
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(2) );
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(4) );
  di<<" Select an edge or a face "<<"\n";
  
  // Boucle d'attente waitpick.
  Standard_Integer argcc = 5;
  const char *buff[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
  const char **argvv = (const char **) buff;
  while (ViewerMainLoop( argcc, argvv) ) { }
  // fin de la boucle
  
  TopoDS_Shape ShapeA;
  for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
    ShapeA = TheAISContext()->SelectedShape();
  }
  
  // SahpeA est un edge.
  // ===================
  if (ShapeA.ShapeType()==TopAbs_EDGE ) {
    
    // desactivation du mode face
    TheAISContext()->DeactivateStandardMode (AIS_Shape::SelectionType(4) );
    di<<" Select a second edge"<<"\n";
    // Boucle d'attente waitpick.
    Standard_Integer argccc = 5;
    const char *bufff[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
    const char **argvvv = (const char **) bufff;
    while (ViewerMainLoop( argccc, argvvv) ) { }
    // fin de la boucle
    
    TopoDS_Shape ShapeB;
    for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
      ShapeB = TheAISContext()->SelectedShape();
    }
    
    // recuperation des edges.
    TopoDS_Edge  EdgeA=TopoDS::Edge(ShapeA);
    TopoDS_Edge  EdgeB=TopoDS::Edge(ShapeB);
    BRepExtrema_ExtCC myDeltaEdge (EdgeA ,EdgeB );
    // on verifie qu'ils ne sont pas paralleles.
    if (!myDeltaEdge.IsParallel() ) {di<<"vparallel error: non parallel edges."<<"\n";return 1; }
    
    
    // On recupere les  vertexes extremites des edge A et B.
    BRepAdaptor_Curve theCurveA(EdgeA);
    BRepAdaptor_Curve theCurveB(EdgeB);
    // On recupere 3 points A,B,C des  curves.
    gp_Pnt A=theCurveA.Value(0.1);
    gp_Pnt B=theCurveA.Value(0.9);
    gp_Pnt C=theCurveB.Value(0.5);
    
    // Construction du Geom_Plane
    GC_MakePlane MkPlane(A,B,C);
    Handle(Geom_Plane) theGeomPlane=MkPlane.Value();
    // Fermeture du contexte local.
    TheAISContext()->CloseLocalContext(myCurrentIndex);
    // Construction de l'AIS_ParallelRelation
    Handle(AIS_ParallelRelation) myParaRelation= new AIS_ParallelRelation(EdgeA ,EdgeB ,theGeomPlane );
    TheAISContext()->Display(myParaRelation );
    GetMapOfAIS().Bind (myParaRelation ,argv[1]);
    
    
  }
  
  // Shape A est une face
  // ====================
  else {
    
    // desactivation du mode edge
    TheAISContext()->DeactivateStandardMode (AIS_Shape::SelectionType(2) );
    di<<" Select a second edge"<<"\n";
    // Boucle d'attente waitpick.
    Standard_Integer argccc = 5;
    const char *bufff[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
    const char **argvvv = (const char **) bufff;
    while (ViewerMainLoop( argccc, argvvv) ) { }
    // fin de la boucle
    
    TopoDS_Shape ShapeB;
    for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
      ShapeB = TheAISContext()->SelectedShape();
    }
    
    TopoDS_Face  FaceA=TopoDS::Face(ShapeA);
    TopoDS_Face  FaceB=TopoDS::Face(ShapeB);
    
    BRepExtrema_ExtFF myDeltaFaceFace  (FaceA ,FaceB );
    // On verifie que les deux faces sont bien parelles.
    if (!myDeltaFaceFace.IsParallel() ) {di<<"vdistdim error: the faces are not parallel. "<<"\n";return 1; }
    
    // recuperation des edges des faces.
    TopExp_Explorer FaceExpA(FaceA,TopAbs_EDGE);
    TopExp_Explorer FaceExpB(FaceB,TopAbs_EDGE);
    
    TopoDS_Edge EdgeA=TopoDS::Edge(FaceExpA.Current() );
    TopoDS_Edge EdgeB=TopoDS::Edge(FaceExpB.Current() );
    
    // On recupere les  vertexes extremites des edge A et B.
    BRepAdaptor_Curve theCurveA(EdgeA);
    BRepAdaptor_Curve theCurveB(EdgeB);
    // On recupere 3 points A,B,C des  curves.
    gp_Pnt A=theCurveA.Value(0.1);
    gp_Pnt B=theCurveA.Value(0.9);
    gp_Pnt C=theCurveB.Value(0.5);
    
    // Construction du Geom_Plane
    GC_MakePlane MkPlane(A,B,C);
    Handle(Geom_Plane) theGeomPlane=MkPlane.Value();
    // Fermeture du contexte local.
    TheAISContext()->CloseLocalContext(myCurrentIndex);
    // Construction de l'AIS_ParallelRelation
    Handle(AIS_ParallelRelation) myParaRelation= new AIS_ParallelRelation(FaceA ,FaceB ,theGeomPlane );
    TheAISContext()->Display(myParaRelation );
    GetMapOfAIS().Bind (myParaRelation ,argv[1]);
    
    
    
  }
  
  
  return 0;
  
}




//==============================================================================
//function : VPerpendicularRelation
//purpose  : Display the Perpendicular Relation
//Draw arg : vperpendicular Name 
//==============================================================================
#include <AIS_PerpendicularRelation.hxx>
#include <TCollection_ExtendedString.hxx>
#include <GC_MakePlane.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <TopExp_Explorer.hxx>



static int VPerpendicularBuilder(Draw_Interpretor& di, Standard_Integer argc, const char** argv) 
{
  // Declarations
  Standard_Integer myCurrentIndex;
 
  // Verification
  if (argc!=2) {di<<" vortho error"<<"\n";return 1;}
  
  // Fermeture des contextes locaux
  TheAISContext()->CloseAllContexts();
  
  // Ouverture d'un contexte local et recuperation de son index.
  TheAISContext()->OpenLocalContext();
  myCurrentIndex=TheAISContext()->IndexOfCurrentLocal();
  
  // On active les modes de selections Edges.
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(2) );
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(4) );
  di<<" Select an edge or a face "<<"\n";
  
  // Boucle d'attente waitpick.
  Standard_Integer argcc = 5;
  const char *buff[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
  const char **argvv = (const char **) buff;
  while (ViewerMainLoop( argcc, argvv) ) { }
  // fin de la boucle
  
  TopoDS_Shape ShapeA;
  for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
    ShapeA = TheAISContext()->SelectedShape();
  }
  
  // ShapeA est un edge.
  // ===================
  if (ShapeA.ShapeType()==TopAbs_EDGE ) {
    
    // desactivation du mode face
    TheAISContext()->DeactivateStandardMode (AIS_Shape::SelectionType(4) );
    di<<" Select a second edge"<<"\n";
    // Boucle d'attente waitpick.
    Standard_Integer argccc = 5;
    const char *bufff[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
    const char **argvvv = (const char **) bufff;
    while (ViewerMainLoop( argccc, argvvv) ) { }
    // fin de la boucle
    
    TopoDS_Shape ShapeB;
    for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
      ShapeB = TheAISContext()->SelectedShape();
    }
    
    // recuperation des edges.
    TopoDS_Edge  EdgeA=TopoDS::Edge(ShapeA);
    TopoDS_Edge  EdgeB=TopoDS::Edge(ShapeB);
    
    // On recupere les  curves
    BRepAdaptor_Curve theCurveA(EdgeA);
    BRepAdaptor_Curve theCurveB(EdgeB);
    // on verifie si les edges sont orthogonaux.
    //gp_Lin theLineA=theCurveA.Line();
    //gp_Lin theLineB=theCurveB.Line();
    //if (abs(theLineA.Angle(theLineB) ) != M_PI/2 ) {cout<<"vperpendicular error: Edges are not  othogonals."<<endl;return 1;}
    
    // On recupere 3 points A,B,C des  curves.
    gp_Pnt A=theCurveA.Value(0.1);
    gp_Pnt B=theCurveA.Value(0.9);
    gp_Pnt C=theCurveB.Value(0.5);
    // Construction du Geom_Plane
    GC_MakePlane MkPlane(A,B,C);
    Handle(Geom_Plane) theGeomPlane=MkPlane.Value();
    // Fermeture du contexte local.
    TheAISContext()->CloseLocalContext(myCurrentIndex);
    // Construction de l'AIS_ParallelRelation
    Handle(AIS_PerpendicularRelation) myOrthoRelation= new AIS_PerpendicularRelation (EdgeA ,EdgeB ,theGeomPlane );
    TheAISContext()->Display(myOrthoRelation );
    GetMapOfAIS().Bind (myOrthoRelation ,argv[1]);
    
    
  }
  
  // Shape A est une face
  // ====================
  else {
    
    // desactivation du mode edge
    TheAISContext()->DeactivateStandardMode (AIS_Shape::SelectionType(2) );
    di<<" Select a second edge"<<"\n";
    // Boucle d'attente waitpick.
    Standard_Integer argccc = 5;
    const char *bufff[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
    const char **argvvv = (const char **) bufff;
    while (ViewerMainLoop( argccc, argvvv) ) { }
    // fin de la boucle
    
    TopoDS_Shape ShapeB;
    for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
      ShapeB = TheAISContext()->SelectedShape();
    }
    
    // pas de verification d'orthogonalite.
    TopoDS_Face  FaceA=TopoDS::Face(ShapeA);
    TopoDS_Face  FaceB=TopoDS::Face(ShapeB);
    
    // recuperation des edges des faces.
    TopExp_Explorer FaceExpA(FaceA,TopAbs_EDGE);
    TopExp_Explorer FaceExpB(FaceB,TopAbs_EDGE);
    
    TopoDS_Edge EdgeA=TopoDS::Edge(FaceExpA.Current() );
    TopoDS_Edge EdgeB=TopoDS::Edge(FaceExpB.Current() );
    
    // On recupere les  vertexes extremites des edge A et B.
    BRepAdaptor_Curve theCurveA(EdgeA);
    BRepAdaptor_Curve theCurveB(EdgeB);
    // On recupere 3 points A,B,C des  curves.
    gp_Pnt A=theCurveA.Value(0.1);
    gp_Pnt B=theCurveA.Value(0.9);
    gp_Pnt C=theCurveB.Value(0.5);
    // Construction du Geom_Plane
    GC_MakePlane MkPlane(A,B,C);
    Handle(Geom_Plane) theGeomPlane=MkPlane.Value();
    // Fermeture du contexte local.
    TheAISContext()->CloseLocalContext(myCurrentIndex);
    // Construction de l'AIS_PerpendicularRelation
    Handle(AIS_PerpendicularRelation) myOrthoRelation= new AIS_PerpendicularRelation(FaceA ,FaceB );
    TheAISContext()->Display(myOrthoRelation );
    GetMapOfAIS().Bind (myOrthoRelation  ,argv[1]);
    
    
    
  }
  
  
  return 0;
  
}


//==============================================================================
//function : VTangentRelation
//purpose  : Display the tangent Relation
//Draw arg : vtangent Name 
//==============================================================================
#include <AIS_TangentRelation.hxx>


static int VTangentBuilder(Draw_Interpretor& di, Standard_Integer argc, const char** argv) 
{
  // Declarations
  Standard_Integer myCurrentIndex;
 
  // Verification
  if (argc!=2) {di<<" vtangent error"<<"\n";return 1;}
  
  // Fermeture des contextes locaux
  TheAISContext()->CloseAllContexts();
  
  // Ouverture d'un contexte local et recuperation de son index.
  TheAISContext()->OpenLocalContext();
  myCurrentIndex=TheAISContext()->IndexOfCurrentLocal();
  
  // On active les modes de selections Edges.
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(2) );
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(4) );
  di<<" Select two coplanar edges(First the circular edge then the tangent edge) or two faces "<<"\n";
  
  // Boucle d'attente waitpick.
  Standard_Integer argcc = 5;
  const char *buff[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
  const char **argvv = (const char **) buff;
  while (ViewerMainLoop( argcc, argvv) ) { }
  // fin de la boucle
  
  TopoDS_Shape ShapeA;
  for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
    ShapeA = TheAISContext()->SelectedShape();
  }
  
  // ShapeA est un edge.
  // ===================
  if (ShapeA.ShapeType()==TopAbs_EDGE ) {
    
    // desactivation du mode face
    TheAISContext()->DeactivateStandardMode (AIS_Shape::SelectionType(4) );
    di<<" Select a second edge"<<"\n";
    // Boucle d'attente waitpick.
    Standard_Integer argccc = 5;
    const char *bufff[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
    const char **argvvv = (const char **) bufff;
    while (ViewerMainLoop( argccc, argvvv) ) { }
    // fin de la boucle
    
    TopoDS_Shape ShapeB;
    for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
      ShapeB = TheAISContext()->SelectedShape();
    }
    
    // recuperation des edges.
    TopoDS_Edge  EdgeA=TopoDS::Edge(ShapeA);
    TopoDS_Edge  EdgeB=TopoDS::Edge(ShapeB);
    
    // On recupere les  curves
    BRepAdaptor_Curve theCurveA(EdgeA);
    BRepAdaptor_Curve theCurveB(EdgeB);
    
    // On recupere 3 points A,B,C des  curves.
    gp_Pnt A=theCurveA.Value(0.1);
    gp_Pnt B=theCurveA.Value(0.9);
    gp_Pnt C=theCurveB.Value(0.5);

    // Construction du Geom_Plane
    GC_MakePlane MkPlane(A,B,C);
    Handle(Geom_Plane) theGeomPlane=MkPlane.Value();
    // Fermeture du contexte local.
    TheAISContext()->CloseLocalContext(myCurrentIndex);
    // Construction de l'AIS_TangentRelation
    Handle(AIS_TangentRelation) myTanRelation= new AIS_TangentRelation  (EdgeA ,EdgeB ,theGeomPlane );
    TheAISContext()->Display(myTanRelation );
    GetMapOfAIS().Bind (myTanRelation ,argv[1]);
    
    
  }
  
  // Shape A est une face
  // ====================
  else {
    
    // desactivation du mode edge
    TheAISContext()->DeactivateStandardMode (AIS_Shape::SelectionType(2) );
    di<<" Select a second edge"<<"\n";
    // Boucle d'attente waitpick.
    Standard_Integer argccc = 5;
    const char *bufff[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
    const char **argvvv = (const char **) bufff;
    while (ViewerMainLoop( argccc, argvvv) ) { }
    // fin de la boucle
    
    TopoDS_Shape ShapeB;
    for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
      ShapeB = TheAISContext()->SelectedShape();
    }
    
    TopoDS_Face  FaceA=TopoDS::Face(ShapeA);
    TopoDS_Face  FaceB=TopoDS::Face(ShapeB);
    
    // recuperation des edges des faces.
    TopExp_Explorer FaceExpA(FaceA,TopAbs_EDGE);
    TopExp_Explorer FaceExpB(FaceB,TopAbs_EDGE);
    
    TopoDS_Edge EdgeA=TopoDS::Edge(FaceExpA.Current() );
    TopoDS_Edge EdgeB=TopoDS::Edge(FaceExpB.Current() );
    
    // On recupere les  vertexes extremites des edge A et B.
    BRepAdaptor_Curve theCurveA(EdgeA);
    BRepAdaptor_Curve theCurveB(EdgeB);
    // On recupere 3 points A,B,C des  curves.
    gp_Pnt A=theCurveA.Value(0.1);
    gp_Pnt B=theCurveA.Value(0.9);
    gp_Pnt C=theCurveB.Value(0.5);

    // Construction du Geom_Plane
    GC_MakePlane MkPlane(A,B,C);
    Handle(Geom_Plane) theGeomPlane=MkPlane.Value();
    // Fermeture du contexte local.
    TheAISContext()->CloseLocalContext(myCurrentIndex);
    // Construction de l'AIS_PerpendicularRelation
    Handle(AIS_TangentRelation) myTanRelation= new AIS_TangentRelation(FaceA ,FaceB,theGeomPlane );
    TheAISContext()->Display(myTanRelation );
    GetMapOfAIS().Bind (myTanRelation  ,argv[1]);
    
    
    
  }
  
  
  return 0;
  
}

//==============================================================================
//function : VSymetricalRelation
//purpose  : Display the Symetrical Relation
//Draw arg : vsymetric Name 
//==============================================================================
#include <AIS_SymmetricRelation.hxx>


static int VSymmetricBuilder(Draw_Interpretor& di, Standard_Integer argc, const char** argv) 
{
  // Declarations
  Standard_Integer myCurrentIndex;
 
  // Verification
  if (argc!=2) {di<<" vSymmetric error"<<"\n";return 1;}
  
  // Fermeture des contextes locaux
  TheAISContext()->CloseAllContexts();
  
  // Ouverture d'un contexte local et recuperation de son index.
  TheAISContext()->OpenLocalContext();
  myCurrentIndex=TheAISContext()->IndexOfCurrentLocal();
  
  // On active les modes de selections
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(2) );
  di<<" Select an edge:the axis of symetry "<<"\n";
  
  // Boucle d'attente waitpick.
  Standard_Integer argcc = 5;
  const char *buff[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
  const char **argvv = (const char **) buff;
  while (ViewerMainLoop( argcc, argvv) ) { }
  // fin de la boucle
  
  TopoDS_Shape ShapeA;
  for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
    ShapeA = TheAISContext()->SelectedShape();
  }
  // recuperation des edges.
  TopoDS_Edge  EdgeA=TopoDS::Edge(ShapeA);
  
  // On active les modes de selections
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(2) );
  TheAISContext()->ActivateStandardMode (AIS_Shape::SelectionType(1) );
  di<<" Select two edges or two vertices. "<<"\n";
  
  // Boucle d'attente waitpick.
  Standard_Integer argcc2 = 5;
//  const char *buff2[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
  const char **argvv2 = (const char **) buff;
  while (ViewerMainLoop( argcc2, argvv2) ) { }
  // fin de la boucle
  
  TopoDS_Shape ShapeB;
  for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
    ShapeB = TheAISContext()->SelectedShape();
  }
  
  
  
  
  
  // ShapeB est un edge.
  // ===================
  if (ShapeB.ShapeType()==TopAbs_EDGE ) {
    
    // desactivation du mode vertex
    TheAISContext()->DeactivateStandardMode (AIS_Shape::SelectionType(1) );
    di<<" Select a second edge"<<"\n";
    // Boucle d'attente waitpick.
    Standard_Integer argccc = 5;
    const char *bufff[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
    const char **argvvv = (const char **) bufff;
    while (ViewerMainLoop( argccc, argvvv) ) { }
    // fin de la boucle
    
    TopoDS_Shape ShapeC;
    for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
      ShapeC = TheAISContext()->SelectedShape();
    }
    
    // recuperation des edges.
    TopoDS_Edge  EdgeB=TopoDS::Edge(ShapeB);
    TopoDS_Edge  EdgeC=TopoDS::Edge(ShapeC);
    // on verifie que les edges sont paralleles
    BRepExtrema_ExtCC myDeltaEdgeAB (EdgeA ,EdgeB );
    BRepExtrema_ExtCC myDeltaEdgeAC (EdgeA ,EdgeC );
    // on verifie qu'ils  sont paralleles.
    if (!myDeltaEdgeAB.IsParallel() ) {di<<"vsymetric error: non parallel edges."<<"\n";return 1; }
    if (!myDeltaEdgeAC.IsParallel() ) {di<<"vsymetric error: non parallel edges."<<"\n";return 1; }
    // on recupere les vertexs
    TopoDS_Vertex  Va,Vb,Vc,Vd;
    TopExp::Vertices(EdgeB,Va,Vb );
    TopExp::Vertices(EdgeC,Vc,Vd );
    gp_Pnt A=BRep_Tool::Pnt(Va);
    gp_Pnt B=BRep_Tool::Pnt(Vc);
    gp_Pnt C = Get3DPointAtMousePosition();
    
    //    gp_Pnt C=BRep_Tool::Pnt(Vc);
    // Construction du Geom_Plane
    GC_MakePlane MkPlane(A,B,C);
    Handle(Geom_Plane) theGeomPlane=MkPlane.Value();
    // Fermeture du contexte local.
    TheAISContext()->CloseLocalContext(myCurrentIndex);
    // Construction de l'AIS_SymmetricRelation
    Handle(AIS_SymmetricRelation) mySymRelation= new AIS_SymmetricRelation (EdgeA ,EdgeB ,EdgeC, theGeomPlane );
    TheAISContext()->Display(mySymRelation );
    GetMapOfAIS().Bind (mySymRelation ,argv[1]);
    
    
  }
  
  // Shape B est un vertex
  // =====================
  else {
    
    // desactivation du mode edge
    TheAISContext()->DeactivateStandardMode (AIS_Shape::SelectionType(2) );
    di<<" Select a second edge"<<"\n";
    // Boucle d'attente waitpick.
    Standard_Integer argccc = 5;
    const char *bufff[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
    const char **argvvv = (const char **) bufff;
    while (ViewerMainLoop( argccc, argvvv) ) { }
    // fin de la boucle
    
    TopoDS_Shape ShapeC;
    for(TheAISContext()->InitSelected() ;TheAISContext()->MoreSelected() ;TheAISContext()->NextSelected() ) {
      ShapeC = TheAISContext()->SelectedShape();
    }
    
    // recuperation des Vertex
    TopoDS_Vertex  VertexB=TopoDS::Vertex(ShapeB);
    TopoDS_Vertex  VertexC=TopoDS::Vertex(ShapeC);
    // transfo en gp_Pnt
    gp_Pnt B=BRep_Tool::Pnt(VertexB);
    gp_Pnt C=BRep_Tool::Pnt(VertexC);
    
    // on recupere les vertexes de l'axe de sym
    TopoDS_Vertex  Va,Vb;
    TopExp::Vertices(EdgeA,Va,Vb );
    gp_Pnt A=BRep_Tool::Pnt(Va);
    // Construction du Geom_Plane
    GC_MakePlane MkPlane(A,B,C);
    Handle(Geom_Plane) theGeomPlane=MkPlane.Value();
    // Fermeture du contexte local.
    TheAISContext()->CloseLocalContext(myCurrentIndex);
    // Construction de l'AIS_SymmetricRelation
    Handle(AIS_SymmetricRelation) mySymRelation= new AIS_SymmetricRelation (EdgeA ,VertexB ,VertexC, theGeomPlane );
    TheAISContext()->Display(mySymRelation );
    GetMapOfAIS().Bind (mySymRelation ,argv[1]);
    
    
    
  }
  
  
  return 0;
  
}

//=======================================================================
//function : VDimParam
//purpose  : Moves dimension or relation text label to defined or picked
//           position and updates the object.
//=======================================================================
static int VDimParam (Draw_Interpretor& theDi, Standard_Integer theArgNum, const char** theArgVec) 
{
  if (theArgNum < 3)
  {
    theDi << theArgVec[0] << " error: the wrong number of input parameters.\n";
    return 1;
  }

  // Get dimension name
  TCollection_AsciiString aName (theArgVec[1]);
  if (!GetMapOfAIS().IsBound2 (aName))
  {
    theDi << theArgVec[0] << "error: no object with this name.\n";
    return 1;
  }

  Handle(AIS_InteractiveObject) anObject = Handle(AIS_InteractiveObject)::DownCast(GetMapOfAIS().Find2 (aName));
  if (anObject->Type() != AIS_KOI_Dimension)
  {
    theDi << theArgVec[0] << "error: no dimension with this name.\n";
    return 1;
  }
  Handle(AIS_Dimension) aDim = Handle(AIS_Dimension)::DownCast (anObject);
  Handle(Prs3d_DimensionAspect) anAspect = aDim->DimensionAspect();

  // Parse parameters
  gp_Pln aWorkingPlane;
  Standard_Real aCustomFlyout = 0.0;

  for (Standard_Integer anIt = 2; anIt < theArgNum; ++anIt)
  {
    TCollection_AsciiString anArgString = theArgVec[anIt];
    TCollection_AsciiString aParamName;
    TCollection_AsciiString aParamValue;
    if (ViewerTest::SplitParameter (anArgString, aParamName, aParamValue))
    {
      aParamName.LowerCase();
      aParamValue.LowerCase();

      if (aParamName == "text")
      {
        anAspect->MakeText3d (aParamValue == "3d");
      }
      else if (aParamName == "name")
      {
        if (aParamValue.IsEmpty())
        {
          std::cerr << theArgVec[0] << ": no name for dimension.\n";
          return 1;
        }

        aName = aParamValue;
      }
      else if (aParamName == "plane")
      {
        if (aParamValue == "xoy")
        {
          aWorkingPlane = gp_Pln (gp_Ax3 (gp::XOY()));
        }
        else if (aParamValue == "zox")
        {
          aWorkingPlane = gp_Pln (gp_Ax3 (gp::ZOX()));
        }
        else if (aParamValue == "yoz")
        {
          aWorkingPlane = gp_Pln (gp_Ax3 (gp::YOZ()));
        }
        else
        {
          std::cerr << theArgVec[0] << ": wrong plane.\n";
          return 1;
        }
      }
      else if (aParamName == "label")
      {
        NCollection_List<TCollection_AsciiString> aListOfLabelVals;
        while (aParamValue.Length() > 0)
        {
          TCollection_AsciiString aValue = aParamValue;

          Standard_Integer aSeparatorPos = aParamValue.Search (",");
          if (aSeparatorPos >= 0)
          {
            aValue.Trunc (aSeparatorPos - 1);
            aParamValue.Remove (aSeparatorPos, 1);
          }

          aListOfLabelVals.Append (aValue);

          aParamValue.Remove (1, aValue.Length());
        }

        NCollection_List<TCollection_AsciiString>::Iterator aLabelValueIt (aListOfLabelVals);
        for ( ; aLabelValueIt.More(); aLabelValueIt.Next())
        {
          aParamValue = aLabelValueIt.Value();

          if (aParamValue == "left")
          {
            anAspect->SetTextHorizontalPosition (Prs3d_DTHP_Left);
          }
          else if (aParamValue == "right")
          {
            anAspect->SetTextHorizontalPosition (Prs3d_DTHP_Right);
          }
          else if (aParamValue == "hcenter")
          {
            anAspect->SetTextHorizontalPosition (Prs3d_DTHP_Center);
          }
          else if (aParamValue == "hfit")
          {
            anAspect->SetTextHorizontalPosition (Prs3d_DTHP_Fit);
          }
          else if (aParamValue == "above")
          {
            anAspect->SetTextVerticalPosition (Prs3d_DTVP_Above);
          }
          else if (aParamValue == "below")
          {
            anAspect->SetTextVerticalPosition (Prs3d_DTVP_Below);
          }
          else if (aParamValue == "vcenter")
          {
            anAspect->SetTextVerticalPosition (Prs3d_DTVP_Center);
          }
          else
          {
            std::cerr << theArgVec[0] << ": invalid label position: \"" << aParamValue << "\".\n";
            return 1;
          }
        }
      }
      else if (aParamName == "flyout")
      {
        if (!aParamValue.IsRealValue())
        {
          std::cerr << theArgVec[0] << ": numeric value expected for flyout.\n";
          return 1;
        }

        aCustomFlyout = aParamValue.RealValue();
        aDim->SetFlyout (aCustomFlyout);
      }
      else if (aParamName == "arrows")
      {
        if (aParamValue == "external")
        {
          anAspect->SetArrowOrientation (Prs3d_DAO_External);
        }
        else if (aParamValue == "internal")
        {
          anAspect->SetArrowOrientation (Prs3d_DAO_Internal);
        }
        else if (aParamValue == "fit")
        {
          anAspect->SetArrowOrientation (Prs3d_DAO_Fit);
        }
      }
      else
      {
        std::cerr << theArgVec[0] << ": unknow parameter: \"" << aParamName << "\".\n";
        return 1;
      }
    }
  }

  // Redisplay a dimension after parameter changing.
  ViewerTest::GetAISContext()->Redisplay (aDim);
  return 0;
}

//=======================================================================
//function : VMoveDim
//purpose  : Moves dimension or relation text label to defined or picked
//           position and updates the object.
//draw args: vmovedim [name] [x y z]
//=======================================================================
static int VMoveDim (Draw_Interpretor& theDi, Standard_Integer theArgNum, const char** theArgVec) 
{
  if (theArgNum > 5)
  {
    theDi << theArgVec[0] << " error: the wrong number of parameters.\n";
    return 1;
  }

  // Parameters parsing
  Standard_Boolean isNameSet = (theArgNum ==2 || theArgNum == 5);
  Standard_Boolean isPointSet = (theArgNum == 4 || theArgNum == 5);

  Handle(AIS_InteractiveObject) aPickedObj;
  gp_Pnt aPoint (gp::Origin());
  Standard_Integer aCurrentIndex = 0;
  Standard_Integer aMaxPickNum = 5;

  // Find object
  if (isNameSet)
  {
     TCollection_AsciiString aName (theArgVec[1]);
     if (!GetMapOfAIS().IsBound2 (aName))
     {
       theDi << theArgVec[0] << " error: no object with this name.\n";
       return 1;
     }

     aPickedObj = Handle(AIS_InteractiveObject)::DownCast (GetMapOfAIS().Find2 (aName));
     
     if (aPickedObj.IsNull())
     {
       theDi << theArgVec[0] << " error: the object with this name is not valid.\n";
       return 1;
     }

     if (aPickedObj->Type() != AIS_KOI_Dimension && aPickedObj->Type() != AIS_KOI_Relation)
     {
       theDi << theArgVec[0] << " error: no dimension or relation with this name.\n";
       return 1;
     }
  }
  else // Pick dimension or relation
  {
    // Close all local contexts
    TheAISContext()->CloseAllContexts();

    // Open local context and get its index for recovery.
    TheAISContext()->OpenLocalContext();
    aCurrentIndex = TheAISContext()->IndexOfCurrentLocal();

    // Loop that will be handle picking.
    Standard_Integer anArgNum = 5;
    const char *aBuffer[] = { "VPick", "X", "VPickY","VPickZ", "VPickShape" };
    const char **anArgVec = (const char **) aBuffer;

    Standard_Boolean isPicked = Standard_False;
    Standard_Integer aPickNum = 0;
    while (!isPicked && aPickNum < aMaxPickNum)
    {
      while (ViewerMainLoop (anArgNum, anArgVec)) { }

      for (TheAISContext()->InitSelected(); TheAISContext()->MoreSelected(); TheAISContext()->NextSelected())
      {
        aPickedObj = TheAISContext()->SelectedInteractive();
      }

      isPicked = (!aPickedObj.IsNull() && (aPickedObj->Type() == AIS_KOI_Dimension || aPickedObj->Type() == AIS_KOI_Relation));

      if (isPicked)
      {
        break;
      }
      aPickNum++;
    }
    if (!isPicked)
    {
      theDi << theArgVec[0] << ": no dimension or relation is selected." << "\n";
      return 1;
    }
  }

  // Find point
  if (isPointSet)
  {
    aPoint = theArgNum == 4 ? gp_Pnt (atoi (theArgVec[1]), atoi (theArgVec[2]), atoi (theArgVec[3]))
                            : gp_Pnt (atoi (theArgVec[2]), atoi (theArgVec[3]), atoi (theArgVec[4]));
  }
  else // Pick the point
  {
    Standard_Integer aPickArgNum = 5;
    const char *aPickBuff[] = {"VPick", "X", "VPickY", "VPickZ", "VPickShape"};
    const char **aPickArgVec = (const char **) aPickBuff;

    while (ViewerMainLoop (aPickArgNum, aPickArgVec)) { }

    // Set text position, update relation or dimension.
    if (aPickedObj->Type() == AIS_KOI_Relation)
    {
      Handle(AIS_Relation) aRelation = Handle(AIS_Relation)::DownCast (aPickedObj);
      aPoint = Get3DPointAtMousePosition();
      aRelation->SetPosition (aPoint);
      TheAISContext()->Redisplay (aRelation);
    }
    else
    {
      Handle(AIS_Dimension) aDim = Handle(AIS_Dimension)::DownCast (aPickedObj);
      gp_Pnt aFirstPoint, aSecondPoint;
      if (aDim->KindOfDimension() == AIS_KOD_PLANEANGLE)
      {
        Handle(AIS_AngleDimension) anAngleDim = Handle(AIS_AngleDimension)::DownCast (aDim);
        aFirstPoint = anAngleDim->FirstPoint();
        aSecondPoint = anAngleDim->SecondPoint();
      }
      else if (aDim->KindOfDimension() == AIS_KOD_LENGTH)
      {
        Handle(AIS_LengthDimension) aLengthDim = Handle(AIS_LengthDimension)::DownCast (aDim);
        aFirstPoint = aLengthDim->FirstPoint();
        aSecondPoint = aLengthDim->SecondPoint();
      }
      else if (aDim->KindOfDimension() == AIS_KOD_RADIUS)
      {
        Handle(AIS_RadiusDimension) aRadiusDim = Handle(AIS_RadiusDimension)::DownCast (aDim);
        aFirstPoint = aRadiusDim->AnchorPoint();
        aSecondPoint = aRadiusDim->Circle().Location();
      }
      else if (aDim->KindOfDimension() == AIS_KOD_DIAMETER)
      {
        Handle(AIS_DiameterDimension) aDiameterDim = Handle(AIS_DiameterDimension)::DownCast (aDim);
        aFirstPoint = aDiameterDim->AnchorPoint();
        aSecondPoint = aDiameterDim->Circle().Location();
      }

      if (!Get3DPointAtMousePosition (aFirstPoint, aSecondPoint, aPoint))
      {
        return 1;
      }

      aDim->SetTextPosition (aPoint);
      TheAISContext()->Redisplay (aDim);
    }

  }

  // Set text position, update relation or dimension.
  if (aPickedObj->Type() == AIS_KOI_Relation)
  {
    Handle(AIS_Relation) aRelation = Handle(AIS_Relation)::DownCast (aPickedObj);
    aRelation->SetPosition (aPoint);
    TheAISContext()->Redisplay (aRelation);
  }
  else
  {
    Handle(AIS_Dimension) aDim = Handle(AIS_Dimension)::DownCast (aPickedObj);
    aDim->SetTextPosition (aPoint);
    TheAISContext()->Redisplay (aDim);
  }

  return 0;
}

//=======================================================================
//function : RelationsCommands
//purpose  : 
//=======================================================================
      

void ViewerTest::RelationCommands(Draw_Interpretor& theCommands)
{
  const char *group = "AISRelations";

  theCommands.Add("vdimension",
      "vdimension {angle|length|radius|diameter} name={Dim_Name} shape1 [shape2 [shape3]]\n"
      " [text={2d|3d}] [plane={xoy|yoz|zox}]\n"
      " [label={left|right|hcenter|hfit},{above|below|vcenter}]\n"
      " [flyout=value] [arrows={external|internal|fit}]\n"
      " -Builds angle, length, radius and diameter dimensions.\n",
      __FILE__,VDimBuilder,group);

  theCommands.Add("vdimparam",
    "vdimparam Dim_Name"
    " [text={2d|3d}] [plane={xoy|yoz|zox}]\n"
    " [label={left|right|hcenter|hfit},{above|below|vcenter}]\n"
    " [flyout=value] [arrows={external|internal|fit}]\n"
    " -Sets parameters for angle, length, radius and diameter dimensions.\n",
    __FILE__,VDimParam,group);

  theCommands.Add("vangledim",
		  "vangledim Name:Selection in the viewer only ",
		  __FILE__,VAngleDimBuilder,group);
  
  theCommands.Add("vdiameterdim",
		  "vdiameterdim Name : Selection in the viewer only ",
		  __FILE__,VDiameterDimBuilder,group);
  
  theCommands.Add("vconcentric",
		  "vconcentric Name : Selection in the viewer only ",
		  __FILE__,VConcentricBuilder,group);

  theCommands.Add("vequaldist",
		  "vequaldist Name Selection in the viewer only ",
		  __FILE__,VEqualDistRelation ,group);
  
  theCommands.Add("vequalrad",
		  "vequalrad Name Selection in the viewer only ",
		  __FILE__,VEqualRadiusRelation  ,group);
  
  theCommands.Add("vfix",  
		  "vfix Name Selection in the viewer only ",
		  __FILE__,VFixRelation  ,group);
  
  theCommands.Add("videntity",
		  "videntity Name Selection in the viewer only ",
		  __FILE__,VIdenticRelation  ,group);
  
  theCommands.Add("vdistdim",
		  "vdistdim Name Selection in the viewer only ",
		  __FILE__,VLenghtDimension ,group);
  
  theCommands.Add("vradiusdim",
		  "vradiusdim Name Selection in the viewer only ",
		  __FILE__,VRadiusDimBuilder ,group);
  
  theCommands.Add("voffsetdim",
		  "voffsetdim Name Selection in the viewer only ",
		  __FILE__,VOffsetDimBuilder ,group);
  
  theCommands.Add("vparallel",
		  "vparallel Name Selection in the viewer only ",
		  __FILE__,VParallelBuilder ,group);
  
  theCommands.Add("vortho",
		  "vortho Name Selection in the viewer only ",
		  __FILE__,VPerpendicularBuilder ,group);
  
  theCommands.Add("vtangent",  
		  "vtangent Name Selection in the viewer only ",
		  __FILE__,VTangentBuilder ,group);
  
  
  theCommands.Add("vsymetric",
		  "vsymetric Name Selection in the viewer only ",
		  __FILE__,VSymmetricBuilder ,group);

  theCommands.Add("vmovedim",
		  "vmovedim [name] : move Mouse in the viewer; click MB1 to stop motion...",
		  __FILE__,VMoveDim,group);

}
