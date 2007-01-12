#ifndef FOREACHVISIBLE_H_
#define FOREACHVISIBLE_H_

template<typename Walker_>
class ForEachVisible : public scene::Graph::Walker
{
  const VolumeTest& m_volume;
  const Walker_& m_walker;
  mutable std::vector<VolumeIntersectionValue> m_state;
public:
  ForEachVisible(const VolumeTest& volume, const Walker_& walker)
    : m_volume(volume), m_walker(walker)
  {
    m_state.push_back(c_volumePartial);
  }
  bool pre(const scene::Path& path, scene::Instance& instance) const
  {
    VolumeIntersectionValue visible = (path.top().get().visible()) ? m_state.back() : c_volumeOutside;

	// Examine the entity class for its filter status. If it is filtered, use the c_volumeOutside
	// state to ensure it is not rendered.
	Entity* entity = Node_getEntity(path.top().get());
	if (entity) {
		const IEntityClass& eclass = entity->getEntityClass();
		if (!GlobalFilterSystem().isVisible("entityclass", eclass.getName())) {
			visible = c_volumeOutside;
		}
	}
	
    if(visible == c_volumePartial)
    {
      visible = m_volume.TestAABB(instance.worldAABB());
    }

    m_state.push_back(visible);

    if (visible == c_volumeOutside)
    {
      return false;
    }
    else
    {
      return m_walker.pre(path, instance, m_state.back());
    }
  }
  void post(const scene::Path& path, scene::Instance& instance) const
  {
    if(m_state.back() != c_volumeOutside)
    {
      m_walker.post(path, instance, m_state.back());
    }

    m_state.pop_back();
  }
};

#endif /*FOREACHVISIBLE_H_*/
