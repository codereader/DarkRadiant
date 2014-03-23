#pragma once

#include <string>
#include <wx/panel.h>

namespace wxutil { class SourceViewCtrl; }
class wxStaticText;

namespace ui
{

class ShaderDefinitionView :
	public wxPanel
{
	// The shader which should be previewed
	std::string _shader;

	wxStaticText* _materialName;
	wxStaticText* _filename;

	// The actual code view
	wxutil::SourceViewCtrl* _view;

public:
	ShaderDefinitionView(wxWindow* parent);

	void setShader(const std::string& shader);

	void update();

	// Convenience method to show the view in a single dialog
	static void ShowDialog(const std::string& shaderName);
};

} // namespace ui
