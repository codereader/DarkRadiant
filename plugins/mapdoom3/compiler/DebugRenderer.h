#pragma once

#include "irender.h"
#include "ProcFile.h"
#include "entitylib.h"
#include "render.h"

namespace map
{

class RenderableBspNode :
	public OpenGLRenderable
{
private:
	BspTreeNodePtr _node;

	std::size_t _level;
	
	typedef boost::shared_ptr<RenderablePointVector> RenderablePointVectorPtr;
	typedef std::vector<RenderablePointVectorPtr> Portals;
	Portals _portals;

public:
	RenderableBspNode(const BspTreeNodePtr& node, std::size_t level) :
		_node(node),
		_level(level)
	{
		std::size_t side;

		// clip the portal by all the other portals in the node
		for (ProcPortal* p = node->portals.get(); p != NULL; p = p->next[side].get())
		{
			side = (p->nodes[0] == node) ? 0 : 1;

			RenderablePointVectorPtr renderPortal(new RenderablePointVector(GL_POLYGON, p->winding.size()));

			_portals.push_back(renderPortal);

			for (std::size_t i = 0; i < renderPortal->size(); ++i)
			{
				(*renderPortal)[i].vertex = p->winding[i].vertex;
				(*renderPortal)[i].colour = Colour4b(1,0,0,1);
			}
		}
	}

	std::size_t getLevel() const
	{
		return _level;
	}

	const BspTreeNodePtr& getNode() const
	{
		return _node;
	}

	void render(const RenderInfo& info) const
	{
		if (!_node) return;

		if (info.checkFlag(RENDER_VERTEX_COLOUR) || info.checkFlag(RENDER_POINT_COLOUR))
        {
            glEnableClientState(GL_COLOR_ARRAY);
        }

		RenderableSolidAABB aabb(_node->bounds);
		//aabb.render(info);

		for (std::size_t i = 0; i < _portals.size(); ++i)
		{
			_portals[i]->render(info);
		}
	}
};

class DebugRenderer :
	public Renderable,
	public OpenGLRenderable
{
private:
	ProcFilePtr _procFile;

	ShaderPtr _highlightShader;
	ShaderPtr _normalShader;
	ShaderPtr _redShader;

	typedef boost::shared_ptr<RenderableBspNode> RenderableBspNodePtr;
	typedef std::vector<RenderableBspNodePtr> RenderableNodes;
	RenderableNodes _nodes;

	std::size_t _activeNode;

public:
	DebugRenderer() :
		_activeNode(0)
	{}

	// Renderable implementation
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
	{
		if (!_procFile) return;

		for (RenderableNodes::const_iterator i = _nodes.begin(); i != _nodes.end(); ++i)
		{
			if ((*i)->getNode()->nodeId == _activeNode)
			{
				collector.SetState(_highlightShader, RenderableCollector::eFullMaterials);
				collector.addRenderable(**i, Matrix4::getIdentity());
			}
			else
			{
				collector.SetState(_normalShader, RenderableCollector::eFullMaterials);
				collector.addRenderable(**i, Matrix4::getIdentity());
			}
		}

		collector.SetState(_redShader, RenderableCollector::eFullMaterials);
		collector.addRenderable(*this, Matrix4::getIdentity());
	}

	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
	{
		renderSolid(collector, volume);
	}

	void setRenderSystem(const RenderSystemPtr& renderSystem)
	{}

	bool isHighlighted() const
	{
		return false; // never highlighted
	}

	void setActiveNode(int nodeId)
	{
		_activeNode = nodeId;
	}

	void render(const RenderInfo& info) const
	{}

	void setProcFile(const ProcFilePtr& file)
	{
		_procFile = file;

		prepare();
	}

	void prepare()
	{
		if (!_procFile) return;

		std::string wireCol = (boost::format("$WIRE_OVERLAY")).str();
		_highlightShader = GlobalRenderSystem().capture(wireCol);

		wireCol = (boost::format("<0.5 0.5 0.5>")).str();
		_normalShader = GlobalRenderSystem().capture(wireCol);

		wireCol = (boost::format("$POINTFILE")).str();
		_redShader = GlobalRenderSystem().capture(wireCol);

		_nodes.clear();
		constructRenderableNodes(_procFile->entities[0]->tree.head, 0);
	}

private:
	void constructRenderableNodes(const BspTreeNodePtr& node, std::size_t level)
	{
		RenderableBspNodePtr renderNode(new RenderableBspNode(node, level));
		_nodes.push_back(renderNode);

		if (node->children[0])
		{
			constructRenderableNodes(node->children[0], level+1);
		}

		if (node->children[1])
		{
			constructRenderableNodes(node->children[1], level+1);
		}
	}
};
typedef boost::shared_ptr<DebugRenderer> DebugRendererPtr;

} // namespace
