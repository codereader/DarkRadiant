#pragma once

#include <sigc++/functors/mem_fun.h>
#include <sigc++/connection.h>
#include "ieclass.h"
#include "icolourscheme.h"
#include "wxutil/event/SingleIdleCallback.h"

namespace ui
{

class EntityClassColourManager :
	public wxutil::SingleIdleCallback
{
private:
	sigc::connection _loadedConn;
	sigc::connection _reloadedConn;

public:
	EntityClassColourManager()
	{
		_loadedConn = GlobalEntityClassManager().defsLoadedSignal().connect(
			sigc::mem_fun(*this, &EntityClassColourManager::onEntityDefsLoaded)
		);
		_reloadedConn = GlobalEntityClassManager().defsReloadedSignal().connect(
			sigc::mem_fun(*this, &EntityClassColourManager::onEntityDefsReloaded)
		);
	}

	~EntityClassColourManager()
	{
		_loadedConn.disconnect();
		_reloadedConn.disconnect();
	}

protected:
	void onIdle() override
	{
		applyColourScheme();
	}

private:
	void applyColourScheme()
	{
		// greebo: Override the eclass colours of two special entityclasses
		Vector3 worlspawnColour = GlobalColourSchemeManager().getColour("default_brush");
		Vector3 lightColour = GlobalColourSchemeManager().getColour("light_volumes");

		auto light = GlobalEntityClassManager().findClass("light");

		if (light)
		{
			light->setColour(lightColour);
		}

		auto worldspawn = GlobalEntityClassManager().findClass("worldspawn");

		if (worldspawn)
		{
			worldspawn->setColour(worlspawnColour);
		}
	}

	void onEntityDefsLoaded()
	{
		requestIdleCallback();
	}

	void onEntityDefsReloaded()
	{
		requestIdleCallback();
	}
};

}
