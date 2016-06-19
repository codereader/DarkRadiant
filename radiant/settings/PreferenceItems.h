#pragma once

#include "ipreferencesystem.h"
#include "PreferenceItemBase.h"

/**
 * greebo: This file contains a couple of classes describing
 * the Preference Items visible in the Preference Dialog. 
 *
 * Client code registers their configurable options with the
 * GlobalPreferenceSystem during module initialisation, these
 * classes here store this information for later retrieval
 * (when the Preference Dialog is constructed, more specifically).
 * 
 * All these are derived from a single base, as most of them
 * want to save their stuff in the Registry.
 */
namespace settings
{

class PreferenceLabel :
	public PreferenceItemBase
{
public:
	PreferenceLabel(const std::string& label) :
		PreferenceItemBase(label)
	{}
};

class PreferenceEntry :
	public PreferenceItemBase
{
public:
	PreferenceEntry(const std::string& label, const std::string& registryKey) :
		PreferenceItemBase(label, registryKey)
	{}
};

class PreferenceCheckbox :
	public PreferenceItemBase
{
public:
	PreferenceCheckbox(const std::string& label, const std::string& registryKey) :
		PreferenceItemBase(label, registryKey)
	{}
};

class PreferenceCombobox :
	public PreferenceItemBase
{
private:
	ComboBoxValueList _values;
	bool _storeValueNotIndex;

public:
	PreferenceCombobox(const std::string& label, const std::string& registryKey, 
					   const ComboBoxValueList& values, bool storeValueNotIndex) :
		PreferenceItemBase(label, registryKey),
		_values(values),
		_storeValueNotIndex(storeValueNotIndex)
	{}

	const ComboBoxValueList& getValues() const
	{
		return _values;
	}

	bool storeValueNotIndex() const
	{
		return _storeValueNotIndex;
	}
};

class PreferencePathEntry :
	public PreferenceItemBase
{
private:
	bool _browseDirectories;

public:
	PreferencePathEntry(const std::string& label, const std::string& registryKey, bool browseDirectories) :
		PreferenceItemBase(label, registryKey),
		_browseDirectories(browseDirectories)
	{}

	bool browseDirectories() const
	{
		return _browseDirectories;
	}
};

class PreferenceSpinner :
	public PreferenceItemBase
{
private:
	double _lower;
	double _upper;
	int _fraction;

public:
	PreferenceSpinner(const std::string& label, const std::string& registryKey, double lower, double upper, int fraction) :
		PreferenceItemBase(label, registryKey),
		_lower(lower),
		_upper(upper),
		_fraction(fraction)
	{}

	double getLower()
	{
		return _lower;
	}

	double getUpper()
	{
		return _upper;
	}

	int getFraction()
	{
		return _fraction;
	}
};

class PreferenceSlider :
	public PreferenceItemBase
{
private:
	double _lower;
	double _upper;
	double _stepIncrement;
	double _pageIncrement;
	int _factor;

public:
	PreferenceSlider(const std::string& label, const std::string& registryKey, double lower, double upper, double stepIncrement, double pageIncrement) :
		PreferenceItemBase(label, registryKey),
		_lower(lower),
		_upper(upper),
		_stepIncrement(stepIncrement),
		_pageIncrement(pageIncrement),
		_factor(1)
	{}

	double getLower()
	{
		return _lower;
	}

	double getUpper()
	{
		return _upper;
	}

	double getStepIncrement()
	{
		return _stepIncrement;
	}

	double getPageIncrement()
	{
		return _pageIncrement;
	}

	int getFactor()
	{
		return _factor;
	}
};

} // namespace
