#ifndef _RENDERABLE_SPACE_PARTITION_H_
#define _RENDERABLE_SPACE_PARTITION_H_

#include "ispacepartition.h"
#include "irender.h"
#include "irenderable.h"

#include "math/Matrix4.h"
#include "math/AABB.h"

namespace render
{

/**
 * greebo: This is a renderable helper object which can be used
 * to render any space partition system implementing the ISPacepartitionSystem interface.
 *
 * Instantiate such a class and pass the Shader and the SpacePartition system to the
 * setShader() and setSpacePartition() methods to enable rendering.
 *
 * This object can be directly attached to the GlobalRenderSystem().
 */
class RenderableSpacePartition :
	public Renderable,
	public OpenGLRenderable
{
private:
	// The shader we're using
	ShaderPtr _shader;

	// The space partition to render
	scene::ISpacePartitionSystemPtr _spacePartition;

public:
	void setSpacePartition(const scene::ISpacePartitionSystemPtr& spacePartition)
	{
		_spacePartition = spacePartition;
	}

	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
	{
		if (_shader != NULL)
		{
			collector.SetState(_shader, RenderableCollector::eFullMaterials);
			collector.addRenderable(*this, Matrix4::getIdentity());
		}
	}

	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
	{
		if (_shader != NULL)
		{
			collector.SetState(_shader, RenderableCollector::eWireframeOnly);
			collector.addRenderable(*this, Matrix4::getIdentity());
		}
	}

	void setRenderSystem(const RenderSystemPtr& renderSystem)
	{
		if (renderSystem)
		{
			_shader = renderSystem->capture("[1 0 0]");
		}
		else
		{
			_shader.reset();
		}
	}

	bool isHighlighted() const
	{
		return false; // never highlighted
	}

	void renderNode(const scene::ISPNodePtr& node) const
	{
		const scene::ISPNode::MemberList& members = node->getMembers();

		float numItems = members.size() > 2 ? 1 : (members.size() > 0 ? 0.6f : 0);
		glColor3f(numItems, numItems, numItems);

		AABB rb(node->getBounds());

		// Extend the renderbounds *slightly* so that the lines don't overlap
		rb.extents *= 1.02f;

		// Wireframe cuboid
		glBegin(GL_LINES);
			glVertex3d(rb.origin.x() + rb.extents.x(), rb.origin.y() + rb.extents.y(), rb.origin.z() + rb.extents.z());
			glVertex3d(rb.origin.x() + rb.extents.x(), rb.origin.y() + rb.extents.y(), rb.origin.z() + -rb.extents.z());

			glVertex3d(rb.origin.x() + rb.extents.x(), rb.origin.y() + rb.extents.y(), rb.origin.z() + rb.extents.z());
			glVertex3d(rb.origin.x() + -rb.extents.x(), rb.origin.y() + rb.extents.y(), rb.origin.z() + rb.extents.z());

			glVertex3d(rb.origin.x() + rb.extents.x(), rb.origin.y() + rb.extents.y(), rb.origin.z() + -rb.extents.z());
			glVertex3d(rb.origin.x() + -rb.extents.x(), rb.origin.y() + rb.extents.y(), rb.origin.z() + -rb.extents.z());

			glVertex3d(rb.origin.x() + rb.extents.x(), rb.origin.y() + rb.extents.y(), rb.origin.z() + rb.extents.z());
			glVertex3d(rb.origin.x() + rb.extents.x(), rb.origin.y() + -rb.extents.y(), rb.origin.z() + rb.extents.z());

			glVertex3d(rb.origin.x() + -rb.extents.x(), rb.origin.y() + rb.extents.y(), rb.origin.z() + rb.extents.z());
			glVertex3d(rb.origin.x() + -rb.extents.x(), rb.origin.y() + -rb.extents.y(), rb.origin.z() + rb.extents.z());

			glVertex3d(rb.origin.x() + -rb.extents.x(), rb.origin.y() + rb.extents.y(), rb.origin.z() + -rb.extents.z());
			glVertex3d(rb.origin.x() + -rb.extents.x(), rb.origin.y() + -rb.extents.y(), rb.origin.z() + -rb.extents.z());

			glVertex3d(rb.origin.x() + rb.extents.x(), rb.origin.y() + rb.extents.y(), rb.origin.z() + -rb.extents.z());
			glVertex3d(rb.origin.x() + rb.extents.x(), rb.origin.y() + -rb.extents.y(), rb.origin.z() + -rb.extents.z());

			glVertex3d(rb.origin.x() + rb.extents.x(), rb.origin.y() + -rb.extents.y(), rb.origin.z() + rb.extents.z());
			glVertex3d(rb.origin.x() + -rb.extents.x(), rb.origin.y() + -rb.extents.y(), rb.origin.z() + rb.extents.z());

			glVertex3d(rb.origin.x() + rb.extents.x(), rb.origin.y() + -rb.extents.y(), rb.origin.z() + rb.extents.z());
			glVertex3d(rb.origin.x() + rb.extents.x(), rb.origin.y() + -rb.extents.y(), rb.origin.z() + -rb.extents.z());

			glVertex3d(rb.origin.x() + -rb.extents.x(), rb.origin.y() + rb.extents.y(), rb.origin.z() + rb.extents.z());
			glVertex3d(rb.origin.x() + -rb.extents.x(), rb.origin.y() + rb.extents.y(), rb.origin.z() + -rb.extents.z());

			glVertex3d(rb.origin.x() + -rb.extents.x(), rb.origin.y() + -rb.extents.y(), rb.origin.z() +  rb.extents.z());
			glVertex3d(rb.origin.x() + -rb.extents.x(), rb.origin.y() + -rb.extents.y(), rb.origin.z() + -rb.extents.z());

			glVertex3d(rb.origin.x() + rb.extents.x(), rb.origin.y() + -rb.extents.y(), rb.origin.z() + -rb.extents.z());
			glVertex3d(rb.origin.x() + -rb.extents.x(), rb.origin.y() + -rb.extents.y(), rb.origin.z() + -rb.extents.z());
		glEnd();

		const scene::ISPNode::NodeList& children = node->getChildNodes();

		for (scene::ISPNode::NodeList::const_iterator i = children.begin(); i != children.end(); ++i)
		{
			renderNode(*i);
		}
	}

	void render(const RenderInfo& info) const
	{
		if (_spacePartition != NULL)
		{
			renderNode(_spacePartition->getRoot());
		}
	}
};

} // namespace render

#endif /* _RENDERABLE_SPACE_PARTITION_H_ */
