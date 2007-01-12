#ifndef CULLINGWALKER_H_
#define CULLINGWALKER_H_

template<typename _Walker>
class CullingWalker
{
  const VolumeTest& m_volume;
  const _Walker& m_walker;
public:
  CullingWalker(const VolumeTest& volume, const _Walker& walker)
    : m_volume(volume), m_walker(walker)
  {
  }
  bool pre(const scene::Path& path, scene::Instance& instance, VolumeIntersectionValue parentVisible) const
  {
    VolumeIntersectionValue visible = Cullable_testVisible(instance, m_volume, parentVisible);
    if(visible != c_volumeOutside)
    {
      return m_walker.pre(path, instance);
    }
    return true;
  }
  void post(const scene::Path& path, scene::Instance& instance, VolumeIntersectionValue parentVisible) const
  {
    return m_walker.post(path, instance);
  }
};

#endif /*CULLINGWALKER_H_*/
