#pragma once

#include "ipreferencesystem.h"
#include "PreferenceItemBase.h"

class wxStaticText;
class wxTextCtrl;
class wxCheckBox;
class wxChoice;

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
		_flag(flag),
		_checkbox(nullptr)
	{}

	virtual wxWindow* createWidget(wxWindow* parent) override;

	virtual void connectWidgetToKey(registry::Buffer& buffer, sigc::signal<void>& resetSignal) override;
};

class PreferenceCombobox :
	public PreferenceItemBase
{
private:
	ComboBoxValueList _values;
	wxChoice* _choice;
	bool _storeValueNotIndex;

public:
	PreferenceCombobox(const std::string& label, const ComboBoxValueList& values, bool storeValueNotIndex) :
		PreferenceItemBase(label),
		_values(values),
		_storeValueNotIndex(storeValueNotIndex),
		_choice(nullptr)
	{}

	virtual wxWindow* createWidget(wxWindow* parent) override;

	virtual void connectWidgetToKey(registry::Buffer& buffer, sigc::signal<void>& resetSignal) override;
};

}