#pragma once

#include "ianimationchooser.h"
#include "MD5AnimationViewer.h"

namespace ui
{

class MD5AnimationChooser :
	public MD5AnimationViewer,
	public IAnimationChooser
{
public:
	MD5AnimationChooser(wxWindow* parent = nullptr);

	Result runDialog(const std::string& preselectModel = std::string(),
		const std::string& preselectAnim = std::string()) override;

	void destroyDialog() override;
};

}
