#pragma once

#include "i18n.h"
#include "ishaderclipboard.h"
#include "iuimanager.h"
#include <sigc++/connection.h>
#include <sigc++/functors/mem_fun.h>
#include <fmt/format.h>

namespace ui
{

namespace
{
	const char* const STATUS_BAR_ELEMENT = "ShaderClipBoard";
}

class ShaderClipboardStatus
{
private:
	sigc::connection _conn;

public:
	ShaderClipboardStatus()
	{
		GlobalUIManager().getStatusBarManager().addTextElement(
			STATUS_BAR_ELEMENT,
			"icon_texture.png",
			IStatusBarManager::POS_SHADERCLIPBOARD,
			_("The name of the shader in the clipboard")
		);

		_conn = GlobalShaderClipboard().signal_sourceChanged().connect(
			sigc::mem_fun(*this, &ShaderClipboardStatus::onShaderClipboardSourceChanged)
		);
	}

	~ShaderClipboardStatus()
	{
		_conn.disconnect();
	}

private:
	std::string getSourceTypeString()
	{
		switch (GlobalShaderClipboard().getSourceType())
		{
		case selection::IShaderClipboard::SourceType::Face:
			return _("Face");
		case selection::IShaderClipboard::SourceType::Patch:
			return _("Patch");
		case selection::IShaderClipboard::SourceType::Shader:
			return _("Shader");
		default:
			return "";
		};
	}

	void onShaderClipboardSourceChanged()
	{
		auto type = GlobalShaderClipboard().getSourceType();
		std::string statusText;

		if (type != selection::IShaderClipboard::SourceType::Empty)
		{
			statusText = fmt::format(_("ShaderClipboard: {0} ({1})"), 
				GlobalShaderClipboard().getShaderName(), getSourceTypeString());
		}
		else
		{
			statusText = _("ShaderClipboard is empty.");
		}

		GlobalUIManager().getStatusBarManager().setText(STATUS_BAR_ELEMENT, statusText);
	}
};

}
