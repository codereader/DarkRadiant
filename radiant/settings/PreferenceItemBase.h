#pragma once

namespace settings
{

// Base class for all pereference items. All of them can carry 
// a title and a registry key
class PreferenceItemBase
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

	virtual const std::string& getLabel() const
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
};
typedef std::shared_ptr<PreferenceItemBase> PreferenceItemBasePtr;

} // namespace
