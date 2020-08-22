#pragma once

#include "ipreferencesystem.h"

namespace settings
{

// Base class for all pereference items. All of them can carry 
// a title and a registry key
class PreferenceItemBase :
	public virtual IPreferenceItemBase
{
protected:
	std::string _registryKey;

	std::string _label;

public:
	PreferenceItemBase(const std::string& label) :
		_label(label)
	{}

	PreferenceItemBase(const std::string& label, const std::string& registryKey) :
		_registryKey(registryKey),
		_label(label)
	{}

	virtual ~PreferenceItemBase() {}

	virtual const std::string& getLabel() const override
	{
		return _label;
	}

	virtual const std::string& getRegistryKey() const override
	{
		return _registryKey;
	}

	virtual void setRegistryKey(const std::string& key) override
	{
		_registryKey = key;
	}
};

} // namespace
