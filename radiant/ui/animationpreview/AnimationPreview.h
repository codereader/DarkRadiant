#pragma once

#include "gtkutil/preview/RenderPreview.h"

#include "imd5model.h"
#include "inode.h"
#include "ieclass.h"
#include "ientity.h"

namespace ui
{

class AnimationPreview :
	public gtkutil::RenderPreview
{
private:
	// Current MD5 model node to display
	scene::INodePtr _model;

	// Each model node needs a parent entity to be properly renderable
	IEntityNodePtr _entity;

	// The animation to play on this model
	md5::IMD5AnimPtr _anim;

public:
	/** Construct a AnimationPreview widget.
	 */
	AnimationPreview();

	void setModelNode(const scene::INodePtr& model);
	void setAnim(const md5::IMD5AnimPtr& anim);

	const scene::INodePtr& getModelNode() const
	{
		return _model;
	}

	const md5::IMD5AnimPtr& getAnim() const
	{
		return _anim;
	}

	AABB getSceneBounds();

protected:
	// Creates parent entity etc.
	void setupSceneGraph();

	bool onPreRender();

	void clearModel();

	RenderStateFlags getRenderFlagsFill();
};
typedef boost::shared_ptr<AnimationPreview> AnimationPreviewPtr;

}
