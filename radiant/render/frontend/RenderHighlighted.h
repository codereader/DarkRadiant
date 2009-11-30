#ifndef RENDERHIGHLIGHTED_H_
#define RENDERHIGHLIGHTED_H_

#include "iscenegraph.h"

class RenderHighlighted :
	public scene::Graph::Walker
{
private:
	// The collector which is sorting our renderables
	RenderableCollector& _collector;

	// The view we're using for culling
	const VolumeTest& _volume;

public:
	RenderHighlighted(RenderableCollector& collector, const VolumeTest& volume) : 
		_collector(collector), 
		_volume(volume)
	{}
  
	// Render function, instructs the Renderable object to submit its geometry
	// to the contained RenderableCollector.
	void render(const Renderable& renderable) const
	{
	    switch(_collector.getStyle())
	    {
	    case RenderableCollector::eFullMaterials:
			renderable.renderSolid(_collector, _volume);
			break;
	    case RenderableCollector::eWireframeOnly:
			renderable.renderWireframe(_collector, _volume);
			break;
	    }      
	}
  
	// Callback to allow the render() function to be called by the 
	// ShaderCache::forEachRenderable() enumeration method.
	typedef ConstMemberCaller1<RenderHighlighted, 
							   const Renderable&, 
							   &RenderHighlighted::render> RenderCaller;

	// scene::Graph::Walker implementation, tells each node to submit its OpenGLRenderables
	bool visit(const scene::INodePtr& node)
	{
		_collector.PushState();

		// greebo: Fix for primitive nodes: as we don't traverse the scenegraph nodes
		// top-down anymore, we need to set the shader state of our parent entity ourselves.
		// Otherwise we're in for NULL-states when rendering worldspawn brushes.
		Entity* entity = Node_getEntity(node->getParent());

		if (entity != NULL)
		{
			_collector.SetState(entity->getEntityClass()->getWireShader(), RenderableCollector::eWireframeOnly);
		}

		RenderablePtr renderable = Node_getRenderable(node);

		if (renderable != NULL)
		{
			renderable->viewChanged();
		}

		if (Node_isSelected(node))
		{
			if (GlobalSelectionSystem().Mode() != SelectionSystem::eComponent)
			{
				_collector.Highlight(RenderableCollector::eFace);
			}
			else if (renderable != NULL)
			{
				renderable->renderComponents(_collector, _volume);
			}

			_collector.Highlight(RenderableCollector::ePrimitive);
		}

		if (renderable != NULL)
		{
			render(*renderable);    
		}

		_collector.PopState();

		return true;
	}

};

#endif /*RENDERHIGHLIGHTED_H_*/
