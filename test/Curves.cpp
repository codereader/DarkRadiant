#include "RadiantTest.h"

#include "ientity.h"
#include "iselection.h"
#include "imap.h"
#include "algorithm/Scene.h"
#include "algorithm/Selection.h"

namespace test
{

using CurveTest = RadiantTest;

inline Vector3 getFirstNurbsVertex(const scene::INodePtr& splineEntity)
{
    auto entity = Node_getEntity(splineEntity);
    auto fullValue = entity->getKeyValue("curve_Nurbs");

    Vector3 position;
    int vertexCount;
    std::string parenthesis;

    std::istringstream stream(fullValue);
    stream >> std::skipws;
    stream >> vertexCount;
    stream >> parenthesis;
    stream >> position.x() >> position.y() >> position.z();

    return position;
}

inline void performSelectionTestOnSplineEntity(const std::string& splineEntityName)
{
    auto splineEntity = algorithm::getEntityByName(GlobalMapModule().getRoot(), splineEntityName);
    auto firstNurbsVertex = getFirstNurbsVertex(splineEntity);

    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 0) << "Nothing should be selected at first";

    algorithm::performPointSelectionOnPosition(firstNurbsVertex, selection::SelectionSystem::eToggle);

    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1) << "Curve " << splineEntityName << " should be selected";
    EXPECT_EQ(GlobalSelectionSystem().ultimateSelected(), splineEntity);
}

TEST_F(CurveTest, SplineWithoutChildBrushIsSelectable)
{
    loadMap("splines.map");
    performSelectionTestOnSplineEntity("spline_without_child_brush");
}

TEST_F(CurveTest, SplineWithChildBrushIsSelectable)
{
    loadMap("splines.map");
    performSelectionTestOnSplineEntity("spline_with_child_brush");
}

TEST_F(CurveTest, SplineWithoutModelKeyIsSelectable)
{
    loadMap("splines.map");
    performSelectionTestOnSplineEntity("spline_without_model");
}

TEST_F(CurveTest, CreateCatmullRomCurve)
{
    GlobalCommandSystem().executeCommand("CreateCurveCatmullRom");
    auto entity = Node_getEntity(GlobalSelectionSystem().ultimateSelected());

    EXPECT_EQ(entity->getKeyValue("classname"), "func_splinemover") << "wrong eclass";
    EXPECT_NE(entity->getKeyValue("curve_CatmullRomSpline"), "") << "catmull rom key is missing";
    EXPECT_EQ(entity->getKeyValue("model"), "") << "model spawnarg should be empty";
}

TEST_F(CurveTest, CreateNurbsCurve)
{
    GlobalCommandSystem().executeCommand("CreateCurveNURBS");
    auto entity = Node_getEntity(GlobalSelectionSystem().ultimateSelected());

    EXPECT_EQ(entity->getKeyValue("classname"), "func_splinemover") << "wrong eclass";
    EXPECT_NE(entity->getKeyValue("curve_Nurbs"), "") << "catmull rom key is missing";
    EXPECT_EQ(entity->getKeyValue("model"), "") << "model spawnarg should be empty";
}

}
