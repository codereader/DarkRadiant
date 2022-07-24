#pragma once

#include "isound.h"
#include "wxutil/DefinitionView.h"

namespace ui
{

class SoundShaderDefinitionView :
    public wxutil::DefinitionView
{
private:
	// The shader which should be previewed
    ISoundShader::Ptr _shader;

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
