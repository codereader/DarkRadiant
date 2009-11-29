#ifndef RENDERHIGHLIGHTED_H_
#define RENDERHIGHLIGHTED_H_

class RenderHighlighted
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

	// Pre-descent function called by ForEachVisible walker.
	bool pre(const scene::INodePtr& node, 
			 VolumeIntersectionValue parentVisible) const
	{
		_collector.PushState();

		if (Cullable_testVisible(node, _volume, parentVisible) != VOLUME_OUTSIDE)
	    {
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
		}

		return true;
	}
  
  	// Post-descent function, called from ForEachVisible
	void post(const scene::INodePtr& node, 
			  VolumeIntersectionValue parentVisible) const
	{
		_collector.PopState();
  	}
};

#endif /*RENDERHIGHLIGHTED_H_*/
