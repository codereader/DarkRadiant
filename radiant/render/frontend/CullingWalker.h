#ifndef CULLINGWALKER_H_
#define CULLINGWALKER_H_

/**
 * Walker wrapper class which invokes its child walker for instances which pass
 * a Cullable::intersectVolume test.
 */
template<typename _Walker>
class CullingWalker
{
	const VolumeTest& m_volume;
	_Walker& m_walker;

public:
	
	// Constructor
	CullingWalker(const VolumeTest& volume, _Walker& walker)
    : m_volume(volume), m_walker(walker)
	{  }
  
	// Pre-descent function
	bool pre(const scene::INodePtr& node, 
			 VolumeIntersectionValue parentVisible) const 
	{
		VolumeIntersectionValue visible = Cullable_testVisible(node, 
    														   m_volume, 
    														   parentVisible);
		if(visible != c_volumeOutside) {
			return m_walker.pre(node);
		}
		return true;
	}
  
  	// Post-descent function
	void post(const scene::INodePtr& node, 
			  VolumeIntersectionValue parentVisible) const 
	{
    	return m_walker.post(node);
	}
};

#endif /*CULLINGWALKER_H_*/
