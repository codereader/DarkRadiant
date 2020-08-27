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
	public PreferenceItemBase,
	public virtual IPreferenceLabel
{
public:
	PreferenceLabel(const std::string& label) :
		PreferenceItemBase(label)
	{}
};

class PreferenceEntry :
	public PreferenceItemBase,
	public virtual IPreferenceEntry
{
public:
	PreferenceEntry(const std::string& label, const std::string& registryKey) :
		PreferenceItemBase(label, registryKey)
	{}
};

class PreferenceCheckbox :
	public PreferenceItemBase,
	public virtual IPreferenceCheckbox
{
public:
	PreferenceCheckbox(const std::string& label, const std::string& registryKey) :
		PreferenceItemBase(label, registryKey)
	{}
};

class PreferenceCombobox :
	public PreferenceItemBase,
	public virtual IPreferenceCombobox
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

	const ComboBoxValueList& getValues() const override
	{
		return _values;
	}

	bool storeValueNotIndex() const override
	{
		return _storeValueNotIndex;
	}
};

class PreferencePathEntry :
	public PreferenceItemBase,
	public virtual IPreferencePathEntry
{
private:
	bool _browseDirectories;

public:
	PreferencePathEntry(const std::string& label, const std::string& registryKey, bool browseDirectories) :
		PreferenceItemBase(label, registryKey),
		_browseDirectories(browseDirectories)
	{}

	bool browseDirectories() const override
	{
		return _browseDirectories;
	}
};

class PreferenceSpinner :
	public PreferenceItemBase,
	public virtual IPreferenceSpinner
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

	double getLower() override
	{
		return _lower;
	}

	double getUpper() override
	{
		return _upper;
	}

	int getFraction() override
	{
		return _fraction;
	}
};

class PreferenceSlider :
	public PreferenceItemBase,
	public virtual IPreferenceSlider
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

	double getLower() override
	{
		return _lower;
	}

	double getUpper() override
	{
		return _upper;
	}

	double getStepIncrement() override
	{
		return _stepIncrement;
	}

	double getPageIncrement() override
	{
		return _pageIncrement;
	}

	int getFactor() override
	{
		return _factor;
	}
};

} // namespace
