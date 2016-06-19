#pragma once

#include <string>
#include <sigc++/signal.h>

#include "ipreferencesystem.h"
#include "registry/buffer.h"

class wxWindow;

namespace ui
{

// Helper class creating and setting up various types of preference UI elements
class PreferenceItem
{
protected:
	wxWindow* _parent;
	std::string _registryKey;
	registry::Buffer& _buffer;
	sigc::signal<void>& _resetSignal;

public:
	PreferenceItem(wxWindow* parent, const std::string& registryKey, 
				   registry::Buffer& buffer, sigc::signal<void>& resetSignal) :
		_parent(parent),
		_registryKey(registryKey),
		_buffer(buffer),
		_resetSignal(resetSignal)
	{}

	wxWindow* createLabel(const std::string& label);

	wxWindow* createEntry();

	wxWindow* createCheckbox(const std::string& label);

	wxWindow* createCombobox(const ComboBoxValueList& values, bool storeValueNotIndex);

	wxWindow* createPathEntry(bool browseDirectories);

	wxWindow* createSpinner(double lower, double upper, int fraction);

	wxWindow* createSlider(double lower, double upper, double stepIncrement, double pageIncrement);
};

} // namespace
