#pragma once

#include "ishaders.h"
#include "ui/common/DefinitionView.h"

namespace ui
{

class MaterialDefinitionView:
	public DefinitionView
{
private:
	// The material which should be previewed
	MaterialPtr _material;

public:
	MaterialDefinitionView(const std::string& shaderName, wxWindow* parent = nullptr);

	void setShader(const std::string& shader);

protected:
	bool isEmpty() const override;
	std::string getDeclName() override;
	std::string getDeclFileName() override;
	std::string getDefinition() override;
};

}
