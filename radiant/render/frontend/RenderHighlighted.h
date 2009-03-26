#ifndef RENDERHIGHLIGHTED_H_
#define RENDERHIGHLIGHTED_H_

class RenderHighlighted
{
  RenderableCollector& m_renderer;
  const VolumeTest& m_volume;
public:
  RenderHighlighted(RenderableCollector& collector, const VolumeTest& volume)
    : m_renderer(collector), m_volume(volume)
  {
  }
  
	// Render function, instructs the Renderable object to submit its geometry
	// to the contained RenderableCollector.
	void render(const Renderable& renderable) const {
	    switch(m_renderer.getStyle())
	    {
	    case RenderableCollector::eFullMaterials:
	      renderable.renderSolid(m_renderer, m_volume);
	      break;
	    case RenderableCollector::eWireframeOnly:
	      renderable.renderWireframe(m_renderer, m_volume);
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
		m_renderer.PushState();

	    if (Cullable_testVisible(node, m_volume, parentVisible) != c_volumeOutside)
	    {
	      RenderablePtr renderable = Node_getRenderable(node);
	      if(renderable)
	      {
	        renderable->viewChanged();
	      }
	
	      if (Node_isSelected(node))
	      {
	        if(GlobalSelectionSystem().Mode() != SelectionSystem::eComponent)
	        {
	          m_renderer.Highlight(RenderableCollector::eFace);
	        }
	        else if(renderable)
	        {
	          renderable->renderComponents(m_renderer, m_volume);
	        }
	        m_renderer.Highlight(RenderableCollector::ePrimitive);
	      }
	        
	      if(renderable)
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
    	m_renderer.PopState();
  	}
};

#endif /*RENDERHIGHLIGHTED_H_*/
