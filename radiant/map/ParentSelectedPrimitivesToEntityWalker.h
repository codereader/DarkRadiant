/**
 * Walker which traverses selected primitives and parents them to the given
 * entity.
 */
class ParentSelectedPrimitivesToEntityWalker 
: public scene::Graph::Walker
{
  scene::INodePtr m_parent;
public:
  ParentSelectedPrimitivesToEntityWalker(scene::INodePtr parent) : m_parent(parent)
  {
  }
  bool pre(const scene::Path& path, scene::Instance& instance) const
  {
    if(path.top() != m_parent && Node_isPrimitive(path.top()))
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
    if(path.top() != m_parent && Node_isPrimitive(path.top()))
    {
      Selectable* selectable = Instance_getSelectable(instance);
      if(selectable != 0
        && selectable->isSelected()
        && path.size() > 1)
      {
        scene::INodePtr parent = path.parent();
        if(parent != m_parent) {
          scene::INodePtr node(path.top());
          Node_getTraversable(parent)->erase(node);
          Node_getTraversable(m_parent)->insert(node);
        }
      }
    }
  }
};


