#ifndef RENDERHIGHLIGHTED_H_
#define RENDERHIGHLIGHTED_H_

class RenderHighlighted
{
  Renderer& m_renderer;
  const VolumeTest& m_volume;
public:
  RenderHighlighted(Renderer& renderer, const VolumeTest& volume)
    : m_renderer(renderer), m_volume(volume)
  {
  }
  void render(const Renderable& renderable) const
  {
    switch(m_renderer.getStyle())
    {
    case Renderer::eFullMaterials:
      renderable.renderSolid(m_renderer, m_volume);
      break;
    case Renderer::eWireframeOnly:
      renderable.renderWireframe(m_renderer, m_volume);
      break;
    }      
  }
  typedef ConstMemberCaller1<RenderHighlighted, const Renderable&, &RenderHighlighted::render> RenderCaller;

  bool pre(const scene::Path& path, scene::Instance& instance, VolumeIntersectionValue parentVisible) const
  {
    m_renderer.PushState();

    if (Cullable_testVisible(instance, m_volume, parentVisible) != c_volumeOutside)
    {
      Renderable* renderable = Instance_getRenderable(instance);
      if(renderable)
      {
        renderable->viewChanged();
      }

      Selectable* selectable = Instance_getSelectable(instance);
      if(selectable != 0 && selectable->isSelected())
      {
        if(GlobalSelectionSystem().Mode() != SelectionSystem::eComponent)
        {
          m_renderer.Highlight(Renderer::eFace);
        }
        else if(renderable)
        {
          renderable->renderComponents(m_renderer, m_volume);
        }
        m_renderer.Highlight(Renderer::ePrimitive);
      }
        
      if(renderable)
      {
        render(*renderable);    
      }
    }

    return true;
  }
  void post(const scene::Path& path, scene::Instance& instance, VolumeIntersectionValue parentVisible) const
  {
    m_renderer.PopState();
  }
};

#endif /*RENDERHIGHLIGHTED_H_*/
