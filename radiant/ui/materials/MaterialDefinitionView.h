#pragma once

#include "ishaders.h"
#include <sigc++/connection.h>
#include "wxutil/DefinitionView.h"

namespace ui
{

class MaterialDefinitionView:
	public wxutil::DefinitionView
{
private:
	// The material which should be previewed
	MaterialPtr _material;

    sigc::connection _materialChanged;

public:
	MaterialDefinitionView(const std::string& shaderName, wxWindow* parent = nullptr);

    ~MaterialDefinitionView();

	void setShader(const std::string& shader);

protected:
	bool isEmpty() const override;
	std::string getDeclName() override;
	std::string getDeclFileName() override;
	std::string getDefinition() override;
};

}
