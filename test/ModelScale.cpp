#include "RadiantTest.h"

#include "algorithm/Scene.h"
#include "iscenegraph.h"
#include "imodel.h"
#include "itransformable.h"
#include "icommandsystem.h"
#include "iselectable.h"
#include "iselection.h"
#include "math/Vector3.h"
#include "registry/registry.h"

namespace test
{

// #5263: "Model Scaler" doesn't handle model duplication correctly
TEST_F(RadiantTest, DuplicateScaledModel)
{
    loadMap("duplicate_scaled_model.map");

    const std::string funcStaticName("moss01");
    const Vector3 scale(3, 4, 2);
    auto func_static = algorithm::getEntityByName(GlobalSceneGraph().root(), funcStaticName);

    // Apply the scale to the model beneath the entity
    func_static->foreachNode([&](const scene::INodePtr& node)
    {
        ITransformablePtr transformable = scene::node_cast<ITransformable>(node);

        if (transformable)
        {
            transformable->setType(TRANSFORM_PRIMITIVE);
            transformable->setScale(scale);
            transformable->freezeTransform();
        }

        return true;
    });

    auto model = algorithm::findChildModel(func_static);

    ASSERT_TRUE(model->hasModifiedScale());
    ASSERT_TRUE(model->getModelScale() == scale);

    // Select the func_static and duplicate it
    GlobalSelectionSystem().setSelectedAll(false);
    Node_setSelected(func_static, true);

    registry::setValue("user/ui/offsetClonedObjects", 0);
    GlobalCommandSystem().executeCommand("CloneSelection");

    auto duplicate = GlobalSelectionSystem().ultimateSelected();

    // This must be the duplicate
    ASSERT_TRUE(Node_getEntity(duplicate)->getKeyValue("name") != funcStaticName);

    // The new model must have a modified scale too
    auto duplicatedModel = algorithm::findChildModel(duplicate);
    ASSERT_TRUE(duplicatedModel->hasModifiedScale());
    ASSERT_TRUE(duplicatedModel->getModelScale() == scale);
}

}
