/**
 * Walker which traverses selected primitives and parents them to the given
 * entity.
 */
class ParentSelectedPrimitivesToEntityWalker 
: public scene::Graph::Walker
{
  scene::Node& m_parent;
public:
  ParentSelectedPrimitivesToEntityWalker(scene::Node& parent) : m_parent(parent)
  {
  }
  bool pre(const scene::Path& path, scene::Instance& instance) const
  {
    if(path.top().get_pointer() != &m_parent
      && Node_isPrimitive(path.top()))
    {
      Selectable* selectable = Instance_getSelectable(instance);
      if(selectable != 0
        && selectable->isSelected()
        && path.size() > 1)
      {
        return false;
      }
    }
    return true;
  }
  void post(const scene::Path& path, scene::Instance& instance) const
  {
    if(path.top().get_pointer() != &m_parent
      && Node_isPrimitive(path.top()))
    {
      Selectable* selectable = Instance_getSelectable(instance);
      if(selectable != 0
        && selectable->isSelected()
        && path.size() > 1)
      {
        scene::Node& parent = path.parent();
        if(&parent != &m_parent)
        {
          NodeSmartReference node(path.top().get());
          Node_getTraversable(parent)->erase(node);
          Node_getTraversable(m_parent)->insert(node);
        }
      }
    }
  }
};


