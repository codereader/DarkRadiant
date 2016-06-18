#pragma once

#include <memory>
#include <sigc++/signal.h>
#include "registry/buffer.h"

class wxWindow;

namespace ui
{

class PreferenceItemBase
{
protected:
	std::string _registryKey;

	std::string _label;

public:
	PreferenceItemBase(const std::string& label) :
		_label(label)
	{}

	virtual ~PreferenceItemBase() {}

	virtual const std::string& getName() const
	{
		return _label;
	}

	virtual const std::string& getRegistryKey() const
	{
		return _registryKey;
	}

	virtual void setRegistryKey(const std::string& key)
	{
		_registryKey = key;
	}

	// Create the widget for packing into a parent container
	virtual wxWindow* createWidget(wxWindow* parent) = 0;

	// Connect the widget to the registry buffer for reading/writing/resetting the values
	virtual void connectWidgetToKey(registry::Buffer& buffer, sigc::signal<void>& resetSignal) 
	{}

	virtual bool useFullWidth()
	{
		return true;
	}
};
typedef std::shared_ptr<PreferenceItemBase> PreferenceItemBasePtr;

}
