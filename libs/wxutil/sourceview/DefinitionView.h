#pragma once

#include <string>
#include <wx/panel.h>
#include "wxutil/dialog/DialogBase.h"

class wxStaticText;

namespace wxutil
{

class SourceViewCtrl;

class DefinitionView :
	public DialogBase
{
private:
	wxStaticText* _declName;
	wxStaticText* _filename;

	wxPanel* _panel;

	// Will be created by subclasses
	SourceViewCtrl* _view;

public:
	DefinitionView(const std::string& title, wxWindow* parent = nullptr);

	int ShowModal() override;

protected:
	void update();

	// Whether there is anything to show. If false, the view will be disabled.
	virtual bool isEmpty() const = 0;

	// Get the name of the declaration
	virtual std::string getDeclName() = 0;

	// Get the file name the declaration is defined in
	virtual std::string getDeclFileName() = 0;

	// Provide the definition without the name and the outermost curly braces
	virtual std::string getDefinition() = 0;

	wxWindow* getMainPanel();
	void setSourceView(SourceViewCtrl* view);
};

} // namespace
