#pragma once

#include <boost/function/function_fwd.hpp>
#include "map/Map.h"
#include <sigc++/trackable.h>

class DeferredDraw :
    public sigc::trackable
{
public:
	typedef boost::function<void()> DrawCallback;

private:
	DrawCallback _draw;
	bool _defer;
	bool _deferred;
public:
	DeferredDraw(const DrawCallback& draw) :
		_draw(draw),
		_defer(false),
		_deferred(false)
	{}

	void defer() {
		_defer = true;
	}

	void draw()
    {
		if (_defer)
        {
			_deferred = true;
		}
		else
        {
			_draw();
		}
	}

	void flush()
    {
        if (_defer && _deferred && _draw)
        {
			_draw();
		}

		_deferred = false;
		_defer = false;
	}

	// Callback target
	void onMapValidChanged()
	{
		if (GlobalMap().isValid())
		{
			flush();
		}
		else
		{
			defer();
		}
	}
};
