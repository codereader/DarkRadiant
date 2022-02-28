#include "AnimationPreview.h"

#include "i18n.h"
#include "imodel.h"
#include "ientity.h"
#include "ieclass.h"
#include "imd5anim.h"
#include "itextstream.h"
#include "math/AABB.h"
#include "scene/BasicRootNode.h"
#include <fmt/format.h>

#include "wxutil/GLWidget.h"

namespace ui
{

namespace
{
	const char* const FUNC_STATIC_CLASS = "func_static";
}

AnimationPreview::AnimationPreview(wxWindow* parent) :
	wxutil::RenderPreview(parent, true)
{}

void AnimationPreview::clearModel()
{
	if (_model)
	{
		if (_entity)
		{
			_entity->removeChildNode(_model);
		}

		_model.reset();
	}
}

// Set the model, this also resets the camera
void AnimationPreview::setModelNode(const scene::INodePtr& node)
{
	// Remove the old model from the scene, if any
	clearModel();

	// Ensure that this is an MD5 model node
	model::ModelNodePtr model = Node_getModel(node);
	
	if (!model)
	{
		rError() << "AnimationPreview::setModelNode: node is not a model." << std::endl;
		stopPlayback();
		return;
	}

	// greebo: Call getScene() to trigger a scene setup if necessary
	getScene();

	try
	{
		dynamic_cast<const md5::IMD5Model&>(model->getIModel()).getAnim();
	}
	catch (std::bad_cast&)
	{
		rError() << "AnimationPreview::setModelNode: modelnode doesn't contain an MD5 model." << std::endl;
		stopPlayback();
		return;
	}

	_model = node;

	// Set the animation to play
	dynamic_cast<md5::IMD5Model&>(model->getIModel()).setAnim(_anim);

	// AddChildNode also tells the model which renderentity it is attached to
	_entity->addChildNode(_model);

	// Reset preview time
	stopPlayback();

	if (_model != nullptr)
	{
        // Reset the model rotation
        resetModelRotation();

		// Use AABB to adjust camera distance
		const AABB& bounds = _model->localAABB();

		if (bounds.isValid())
		{
            // Reset the default view, facing down to the model from diagonally above the bounding box
            double distance = bounds.getRadius() * 3.0f;
            setViewOrigin(Vector3(1, 1, 1) * distance);
		}
		else
		{
			// Bounds not valid, fall back to default
            setViewOrigin(Vector3(1, 1, 1) * 40.0f);
		}

        setViewAngles(Vector3(23, 135, 0));

		// Start playback when switching particles
		startPlayback();
	}

	// Redraw
	queueDraw();
}

AABB AnimationPreview::getSceneBounds()
{
	if (!_model) return RenderPreview::getSceneBounds();

	return _model->localAABB();
}

bool AnimationPreview::onPreRender()
{
	if (!_model) return false;

	// Set the animation to play
	model::ModelNodePtr model = Node_getModel(_model);
	dynamic_cast<md5::IMD5Model&>(model->getIModel()).updateAnim(_renderSystem->getTime());

	return true;
}

std::string AnimationPreview::getInfoText()
{
    auto text = RenderPreview::getInfoText();

    if (_model)
    {
        // Set the animation to play
        auto model = Node_getModel(_model);
        auto anim = dynamic_cast<md5::IMD5Model&>(model->getIModel()).getAnim();

        if (anim)
        {
            auto numFrames = anim->getNumFrames();
            auto currentFrame = (_renderSystem->getTime() / MSEC_PER_FRAME) % numFrames;
            return fmt::format(_("{0} | Frame {1} of {2}."), text, currentFrame, numFrames);
        }
    }

    return text;
}

RenderStateFlags AnimationPreview::getRenderFlagsFill()
{
	return RenderPreview::getRenderFlagsFill() | RENDER_DEPTHWRITE | RENDER_DEPTHTEST;
}

void AnimationPreview::setAnim(const md5::IMD5AnimPtr& anim)
{
	_anim = anim;

	if (!_model)
	{
		return;
	}

	// Set the animation to play
	model::ModelNodePtr model = Node_getModel(_model);
	dynamic_cast<md5::IMD5Model&>(model->getIModel()).setAnim(_anim);

	queueDraw();
}

void AnimationPreview::setupSceneGraph()
{
	RenderPreview::setupSceneGraph();

    _root = std::make_shared<scene::BasicRootNode>();

	_entity = GlobalEntityModule().createEntity(
		GlobalEntityClassManager().findClass(FUNC_STATIC_CLASS)
    );

    _root->addChildNode(_entity);

	// This entity is acting as our root node in the scene
	getScene()->setRoot(_root);
}

void AnimationPreview::onModelRotationChanged()
{
    if (_entity)
    {
        // Update the model rotation on the entity
        std::ostringstream value;
        value << _modelRotation.xx() << ' '
            << _modelRotation.xy() << ' '
            << _modelRotation.xz() << ' '
            << _modelRotation.yx() << ' '
            << _modelRotation.yy() << ' '
            << _modelRotation.yz() << ' '
            << _modelRotation.zx() << ' '
            << _modelRotation.zy() << ' '
            << _modelRotation.zz();

        Node_getEntity(_entity)->setKeyValue("rotation", value.str());
    }
}

} // namespace
