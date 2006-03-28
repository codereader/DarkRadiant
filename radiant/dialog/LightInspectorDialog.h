#ifndef LIGHTINSPECTORDIALOG_H_
#define LIGHTINSPECTORDIALOG_H_

#include "DarkRadiantDialog.h"

namespace darkradiant
{

class LightInspectorDialog:
	public DarkRadiantDialog
{
public:
	LightInspectorDialog();
	virtual ~LightInspectorDialog();

	// Create and show the Light Inspector window.
	virtual void doDisplay();
	virtual void doDestroy();
};

}

#endif /*LIGHTINSPECTORDIALOG_H_*/
