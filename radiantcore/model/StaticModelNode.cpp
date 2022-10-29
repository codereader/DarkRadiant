#include "StaticModelNode.h"

#include "StaticModelSurface.h"
#include "ishaders.h"
#include "iscenegraph.h"
#include "imap.h"

namespace model
{

StaticModelNode::StaticModelNode(const StaticModelPtr& picoModel) :
    _model(new StaticModel(*picoModel)),
    _name(picoModel->getFilename())
{
    _model->signal_ShadersChanged().connect(sigc::mem_fun(*this, &StaticModelNode::onModelShadersChanged));
    _model->signal_SurfaceScaleApplied().connect(sigc::mem_fun(*this, &StaticModelNode::onModelScaleApplied));

    // Update the skin
    skinChanged("");
}

void StaticModelNode::createRenderableSurfaces()
{
    _model->foreachSurface([&](const StaticModelSurface& surface)
    {
        if (surface.getVertexArray().empty() || surface.getIndexArray().empty())
        {
            return; // don't handle empty surfaces
        }

        emplaceRenderableSurface(std::make_shared<RenderableModelSurface>(surface, _renderEntity, localToWorld()));
    });
}

void StaticModelNode::onInsertIntoScene(scene::IMapRootNode& root)
{
    _model->connectUndoSystem(root.getUndoSystem());

    ModelNodeBase::onInsertIntoScene(root);
}

void StaticModelNode::onRemoveFromScene(scene::IMapRootNode& root)
{
    _model->disconnectUndoSystem(root.getUndoSystem());

    ModelNodeBase::onRemoveFromScene(root);
}

const IModel& StaticModelNode::getIModel() const
{
    return *_model;
}

IModel& StaticModelNode::getIModel()
{
    return *_model;
}

bool StaticModelNode::hasModifiedScale()
{
    return _model->getScale() != Vector3(1, 1, 1);
}

Vector3 StaticModelNode::getModelScale()
{
	return _model->getScale();
}

const AABB& StaticModelNode::localAABB() const {
    return _model->localAABB();
}

// SelectionTestable implementation
void StaticModelNode::testSelect(Selector& selector, SelectionTest& test) {
    _model->testSelect(selector, test, localToWorld());
}

std::string StaticModelNode::name() const {
    return _model->getFilename();
}

const StaticModelPtr& StaticModelNode::getModel() const {
    return _model;
}

void StaticModelNode::setModel(const StaticModelPtr& model) {
    _model = model;
}

void StaticModelNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
    ModelNodeBase::setRenderSystem(renderSystem);

    // This will trigger onModelShadersChanged() to refresh the renderables
    _model->setRenderSystem(renderSystem);
}

void StaticModelNode::onModelShadersChanged()
{
    // Detach renderables on model shader change,
    // they will be refreshed next time things are rendered
    detachFromShaders();
}

// Traceable implementation
bool StaticModelNode::getIntersection(const Ray& ray, Vector3& intersection)
{
    return _model->getIntersection(ray, intersection, localToWorld());
}

// Skin changed notify
void StaticModelNode::skinChanged(const std::string& newSkinName)
{
    // The new skin name is stored locally
    _skin = newSkinName;

    // greebo: Acquire the ModelSkin reference from the SkinCache (might return null)
    // Applying the skin might trigger onModelShadersChanged()
    _model->applySkin(GlobalModelSkinCache().findSkin(_skin));

    // Refresh the scene (TODO: get rid of that)
    GlobalSceneGraph().sceneChanged();
}

// Returns the name of the currently active skin
std::string StaticModelNode::getSkin() const
{
    return _skin;
}

void StaticModelNode::_onTransformationChanged()
{
    // Always revert to our original state before evaluating
    if (getTransformationType() & TransformationType::Scale)
    {
        _model->revertScale();
        _model->evaluateScale(getScale());
    }
    else if (getTransformationType() == TransformationType::NoTransform)
    {
        // Transformation has been changed but no transform mode is set,
        // so the reason we got here is a cancelTransform() call, revert everything
        if (_model->revertScale())
        {
            // revertScale returned true, the scale has actually been modified
            _model->evaluateScale(Vector3(1,1,1));
        }
    }
}

void StaticModelNode::_applyTransformation()
{
    if (getTransformationType() & TransformationType::Scale)
    {
        _model->revertScale();
        _model->evaluateScale(getScale());
        _model->freezeScale();
    }
}

void StaticModelNode::onModelScaleApplied()
{
    queueRenderableUpdate();
}

} // namespace model
