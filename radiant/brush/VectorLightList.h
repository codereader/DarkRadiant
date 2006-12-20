#ifndef VECTORLIGHTLIST_H_
#define VECTORLIGHTLIST_H_

#include <vector>
#include "irender.h"

class VectorLightList : public LightList {
	typedef std::vector<const RendererLight*> Lights;
	Lights m_lights;
public:
	void addLight(const RendererLight& light) {
		m_lights.push_back(&light);
	}
	
	void clear() {
		m_lights.clear();
	}
	
	void evaluateLights() const {
	}

	void lightsChanged() const {
	}

	void forEachLight(const RendererLightCallback& callback) const {
		for (Lights::const_iterator i = m_lights.begin(); i != m_lights.end(); ++i) {
			callback(*(*i));
		}
	}
}; // class VectorLightList

#endif /*VECTORLIGHTLIST_H_*/
