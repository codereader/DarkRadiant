#pragma once

#include "ibrush.h"
#include "math/Plane3.h"

namespace test
{

namespace algorithm
{

// Creates a cubic brush with dimensions 64x64x64 at the given origin
scene::INodePtr createCubicBrush(const scene::INodePtr& parent, 
    const Vector3& origin = Vector3(0,0,0),
    const std::string& material = "_default")
{
    auto brushNode = GlobalBrushCreator().createBrush();
    parent->addChildNode(brushNode);

    auto& brush = *Node_getIBrush(brushNode);

    auto translation = Matrix4::getTranslation(origin);
    brush.addFace(Plane3(+1, 0, 0, 64).transform(translation));
    brush.addFace(Plane3(-1, 0, 0, 64).transform(translation));
    brush.addFace(Plane3(0, +1, 0, 64).transform(translation));
    brush.addFace(Plane3(0, -1, 0, 64).transform(translation));
    brush.addFace(Plane3(0, 0, +1, 64).transform(translation));
    brush.addFace(Plane3(0, 0, -1, 64).transform(translation));

    brush.setShader(material);

    brush.evaluateBRep();

    return brushNode;
}

}

}
