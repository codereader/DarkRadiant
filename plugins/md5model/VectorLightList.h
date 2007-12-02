#ifndef VECTORLIGHTLIST_H_
#define VECTORLIGHTLIST_H_

#include "irender.h"
#include <vector>

namespace md5 {

class VectorLightList : 
	public LightList
{
	typedef std::vector<const RendererLight*> Lights;
	Lights _lights;
	
public:
	
	void addLight(const RendererLight& light) {
		_lights.push_back(&light);
	}
	
	void clear() {
		_lights.clear();
	}
	
	void evaluateLights() const
	{}
	
	void lightsChanged() const
	{}
	
	void forEachLight(const RendererLightCallback& callback) const {
		for (Lights::const_iterator i = _lights.begin(); i != _lights.end(); ++i) {
			callback(*(*i));
		}
	}
};

} // namespace md5

#endif /*VECTORLIGHTLIST_H_*/
