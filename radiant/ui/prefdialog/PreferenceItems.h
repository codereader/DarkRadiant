#pragma once

#include "PreferenceItemBase.h"

class wxStaticText;

namespace ui
{

class PreferenceLabel :
	public PreferenceItemBase
{
private:
	std::string _label;

	wxStaticText* _labelWidget;

public:
	PreferenceLabel(const std::string& label) :
		_label(label),
		_labelWidget(nullptr)
	{}

	virtual const std::string& getName() const override
	{
		return _label;
	}

	virtual wxWindow* createWidget(wxWindow* parent) override;
};

class PreferenceEntry :
	public PreferenceItemBase
{
private:
	std::string _label;

	wxTextCtrl* _entryWidget;

public:
	PreferenceEntry(const std::string& label) :
		_label(label),
		_entryWidget(nullptr)
	{}

	virtual const std::string& getName() const override
	{
		return _label;
	}

	virtual wxWindow* createWidget(wxWindow* parent) override;

	virtual void connectWidgetToKey(registry::Buffer& buffer, sigc::signal<void>& resetSignal) override;
};

}