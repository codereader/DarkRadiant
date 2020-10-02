#pragma once

#include "isound.h"
#include "DefinitionView.h"

namespace ui
{

class SoundShaderDefinitionView :
	public DefinitionView
{
private:
	// The shader which should be previewed
	ISoundShaderPtr _shader;

public:
	SoundShaderDefinitionView(const std::string& shaderName, wxWindow* parent = nullptr);

	void setShader(const std::string& shader);

protected:
	bool isEmpty() const override;
	std::string getDeclName() override;
	std::string getDeclFileName() override;
	std::string getDefinition() override;
};

}
