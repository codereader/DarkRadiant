#ifndef _WAVEFRONT_MODULE_H_
#define _WAVEFRONT_MODULE_H_

#include "imodule.h"
#include "icommandsystem.h"

namespace exporter
{

class WaveFrontModule :
	public RegisterableModule
{
public:
	void exportSelectionAsOBJ(const cmd::ArgumentList& args);

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
};
typedef boost::shared_ptr<WaveFrontModule> WaveFrontModulePtr;

}

#endif /* _WAVEFRONT_MODULE_H_ */
