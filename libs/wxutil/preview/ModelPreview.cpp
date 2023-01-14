#include "ModelPreview.h"
#include "../GLWidget.h"

#include "ifilter.h"
#include "imodelcache.h"
#include "i18n.h"
#include "ieclass.h"
#include "math/AABB.h"
#include "modelskin.h"
#include "entitylib.h"
#include "scenelib.h"
#include "scene/Node.h"
#include "scene/BasicRootNode.h"
#include "wxutil/dialog/MessageBox.h"
#include "string/convert.h"
#include "fmt/format.h"

namespace wxutil
{

namespace
{
	constexpr const char* const FUNC_STATIC_CLASS = "func_static";
}

ModelPreview::ModelPreview(wxWindow* parent) :
    EntityPreview(parent)
{}

ModelPreview::~ModelPreview()
{
    _skinDeclChangedConn.disconnect();
}

const std::string& ModelPreview::getModel() const
{
    return _model;
}

const std::string& ModelPreview::getSkin() const
{
    return _skin;
}

void ModelPreview::setModel(const std::string& model)
{
    // Remember the name and mark the scene as "not ready"
    _model = model;
    queueSceneUpdate();

    if (!_model.empty())
    {
        // Reset time if the model has changed
        if (_model != _lastModel)
        {
            // Reset preview time
            stopPlayback();
        }

        // Redraw
        queueDraw();
    }
    else
    {
        stopPlayback();
    }
}

void ModelPreview::setSkin(const std::string& skin) {

    _skin = skin;
    _skinDeclChangedConn.disconnect();
    queueSceneUpdate();

    // Redraw
    queueDraw();
}

void ModelPreview::setupSceneGraph()
{
    EntityPreview::setupSceneGraph();

    try
    {
        // Add a hidden func_static as preview entity
        auto entity = GlobalEntityModule().createEntity(
            GlobalEntityClassManager().findClass(FUNC_STATIC_CLASS));

        setEntity(entity);

        entity->enable(scene::Node::eHidden);
        entity->getEntity().setKeyValue("model", "-");
    }
    catch (std::runtime_error&)
    {
        Messagebox::ShowError(fmt::format(
            _("Unable to setup the preview,\ncould not find the entity class '{0}'"),
            FUNC_STATIC_CLASS));
    }
}

void ModelPreview::prepareScene()
{
    EntityPreview::prepareScene();

    // If the model name is empty, release the model
    if (_model.empty())
    {
        if (_modelNode)
        {
            getEntity()->removeChildNode(_modelNode);
        }

        _modelNode.reset();

        // Emit the signal carrying an empty pointer
        _modelLoadedSignal.emit(model::ModelNodePtr());
        return;
    }

    if (_modelNode)
    {
        getEntity()->removeChildNode(_modelNode);
    }

    // Check if the model key is pointing to a def
    auto modelDef = GlobalEntityClassManager().findModel(_model);

    _modelNode = GlobalModelCache().getModelNode(modelDef ? modelDef->getMesh() : _model);

    if (_modelNode)
    {
        getEntity()->addChildNode(_modelNode);

        // Apply the skin
        applySkin();

        // Apply the idle pose if possible
        if (modelDef)
        {
            scene::applyIdlePose(_modelNode, modelDef);
        }

        setupInitialViewPosition();

        _lastModel = _model;

        // Done loading, emit the signal
        _modelLoadedSignal.emit(Node_getModel(_modelNode));
    }
}

void ModelPreview::setupInitialViewPosition()
{
    if (_lastModel != _model)
    {
        // Reset the model rotation
        resetModelRotation();

        // Reset the default view, facing down to the model from diagonally above the bounding box
        double distance = getSceneBounds().getRadius() * _defaultCamDistanceFactor;

        setViewOrigin(getSceneBounds().getOrigin() + Vector3(1, 1, 1) * distance);
        setViewAngles(Vector3(34, 135, 0));
    }
}

AABB ModelPreview::getSceneBounds()
{
    if (!_modelNode)
    {
        return EntityPreview::getSceneBounds();
    }

    return _modelNode->localAABB();
}

sigc::signal<void, const model::ModelNodePtr&>& ModelPreview::signal_ModelLoaded()
{
    return _modelLoadedSignal;
}

void ModelPreview::applySkin()
{
    if (auto model = Node_getModel(_modelNode); model)
    {
        auto skin = GlobalModelSkinCache().findSkin(_skin);

        if (skin)
        {
            _skinDeclChangedConn.disconnect();
            _skinDeclChangedConn = skin->signal_DeclarationChanged().connect(
                sigc::mem_fun(*this, &ModelPreview::onSkinDeclarationChanged));
        }

        model->getIModel().applySkin(skin);
    }
}

void ModelPreview::onSkinDeclarationChanged()
{
    applySkin();
    queueDraw();
}

} // namespace ui
