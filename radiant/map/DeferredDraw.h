#ifndef DEFERREDDRAW_H_
#define DEFERREDDRAW_H_

#include "generic/callbackfwd.h"
#include "map/Map.h"

class DeferredDraw
{
	Callback m_draw;
	bool m_defer;
	bool m_deferred;
public:
	DeferredDraw(const Callback& draw) : 
		m_draw(draw), 
		m_defer(false), 
		m_deferred(false)
	{}
	
	void defer() {
		m_defer = true;
	}
	
	void draw() {
		if (m_defer) {
			m_deferred = true;
		}
		else {
			m_draw();
		}
	}
	
	void flush() {
		if (m_defer && m_deferred) {
			m_draw();
		}
		m_deferred = false;
		m_defer = false;
	}
};

inline void DeferredDraw_onMapValidChanged(DeferredDraw& self) {
	if (GlobalMap().isValid()) {
		self.flush();
	}
	else {
		self.defer();
	}
}
typedef ReferenceCaller<DeferredDraw, DeferredDraw_onMapValidChanged> DeferredDrawOnMapValidChangedCaller;

#endif /*DEFERREDDRAW_H_*/
