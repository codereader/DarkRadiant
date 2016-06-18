#pragma once

#include "ipreferencesystem.h"
#include "PreferenceItemBase.h"

class wxStaticText;
class wxTextCtrl;
class wxCheckBox;
class wxChoice;
class wxWindow;
namespace wxutil { class PathEntry; }

namespace ui
{

class PreferenceLabel :
	public PreferenceItemBase
{
private:
	wxStaticText* _labelWidget;

public:
	PreferenceLabel(const std::string& label) :
		PreferenceItemBase(label),
		_labelWidget(nullptr)
	{}

	virtual wxWindow* createWidget(wxWindow* parent) override;
};

class PreferenceEntry :
	public PreferenceItemBase
{
private:
	wxTextCtrl* _entryWidget;

public:
	PreferenceEntry(const std::string& label) :
		PreferenceItemBase(label),
		_entryWidget(nullptr)
	{}

	virtual wxWindow* createWidget(wxWindow* parent) override;

	virtual void connectWidgetToKey(registry::Buffer& buffer, sigc::signal<void>& resetSignal) override;
};

class PreferenceCheckbox :
	public PreferenceItemBase
{
private:
	std::string _flag;

	wxCheckBox* _checkbox;

public:
	PreferenceCheckbox(const std::string& label, const std::string& flag) :
		PreferenceItemBase(label),
		_checkbox(nullptr),
		_flag(flag)
	{}

	virtual wxWindow* createWidget(wxWindow* parent) override;

	virtual void connectWidgetToKey(registry::Buffer& buffer, sigc::signal<void>& resetSignal) override;
};

class PreferenceCombobox :
	public PreferenceItemBase
{
private:
	wxChoice* _choice;
	ComboBoxValueList _values;
	bool _storeValueNotIndex;

public:
	PreferenceCombobox(const std::string& label, const ComboBoxValueList& values, bool storeValueNotIndex) :
		PreferenceItemBase(label),
		_choice(nullptr),
		_values(values),
		_storeValueNotIndex(storeValueNotIndex)		
	{}

	virtual wxWindow* createWidget(wxWindow* parent) override;

	virtual void connectWidgetToKey(registry::Buffer& buffer, sigc::signal<void>& resetSignal) override;
};

class PreferencePathEntry :
	public PreferenceItemBase
{
private:
	wxutil::PathEntry* _entry;
	bool _browseDirectories;

public:
	PreferencePathEntry(const std::string& label, bool browseDirectories) :
		PreferenceItemBase(label),
		_entry(nullptr),
		_browseDirectories(browseDirectories)		
	{}

	virtual wxWindow* createWidget(wxWindow* parent) override;

	virtual void connectWidgetToKey(registry::Buffer& buffer, sigc::signal<void>& resetSignal) override;
};

class PreferenceSpinner :
	public PreferenceItemBase
{
private:
	wxWindow* _spinner;
	double _lower;
	double _upper;
	int _fraction;

public:
	PreferenceSpinner(const std::string& label, double lower, double upper, int fraction) :
		PreferenceItemBase(label),
		_spinner(nullptr),
		_lower(lower),
		_upper(upper),
		_fraction(fraction)		
	{}

	virtual wxWindow* createWidget(wxWindow* parent) override;

	virtual void connectWidgetToKey(registry::Buffer& buffer, sigc::signal<void>& resetSignal) override;
};

class PreferenceSlider :
	public PreferenceItemBase
{
private:
	wxSlider* _slider;
	double _value;
	double _lower;
	double _upper;
	double _stepIncrement;
	double _pageIncrement;
	int _factor;

public:
	PreferenceSlider(const std::string& label, double value, double lower, double upper, double stepIncrement, double pageIncrement) :
		PreferenceItemBase(label),
		_slider(nullptr),
		_value(value),
		_lower(lower),
		_upper(upper),
		_stepIncrement(stepIncrement),
		_pageIncrement(pageIncrement),
		_factor(1)
	{}

	virtual wxWindow* createWidget(wxWindow* parent) override;

	virtual void connectWidgetToKey(registry::Buffer& buffer, sigc::signal<void>& resetSignal) override;
};

}