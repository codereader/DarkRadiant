#pragma once

#include "imodule.h"
#include "imodel.h"
#include "itextstream.h"
#include "ifilesystem.h"

#include <stdio.h>
#include "picomodel.h"

#include <boost/algorithm/string/case_conv.hpp>
#include "PicoModelLoader.h"

typedef unsigned char byte;

void PicoPrintFunc(int level, const char *str)
{
	if (str == 0)
		return;

	switch (level)
	{
	case PICO_NORMAL:
		rMessage() << str << std::endl;
		break;

	case PICO_VERBOSE:
		//rMessage() << "PICO_VERBOSE: " << str << std::endl;
		break;

	case PICO_WARNING:
		rError() << "PICO_WARNING: " << str << std::endl;
		break;

	case PICO_ERROR:
		rError() << "PICO_ERROR: " << str << std::endl;
		break;

	case PICO_FATAL:
		rError() << "PICO_FATAL: " << str << std::endl;
		break;
	}
}

void PicoLoadFileFunc(char *name, byte **buffer, int *bufSize)
{
	*bufSize = static_cast<int>(GlobalFileSystem().loadFile(name, (void**)buffer));
}

void PicoFreeFileFunc(void* file)
{
	GlobalFileSystem().freeFile(file);
}

namespace model
{

class PicoModelModule :
	public RegisterableModule
{
public:
	// RegisterableModule implementation
	const std::string& getName() const
	{
		static std::string _name("PicoModelModule");
		return _name;
	}

	const StringSet& getDependencies() const
	{
		static StringSet _dependencies;

		if (_dependencies.empty())
		{
			_dependencies.insert(MODULE_MODELFORMATMANAGER);
		}

		return _dependencies;
	}

	void initialiseModule(const ApplicationContext& ctx)
	{
		PicoInit();
		PicoSetMallocFunc(malloc);
		PicoSetFreeFunc(free);
		PicoSetPrintFunc(PicoPrintFunc);
		PicoSetLoadFileFunc(PicoLoadFileFunc);
		PicoSetFreeFileFunc(PicoFreeFileFunc);

		// Register all importers available through picomodel
		const picoModule_t** modules = PicoModuleList(0);

		while (*modules != nullptr) 
		{
			const picoModule_t* module = *modules++;

			if (module->canload && module->load) 
			{
				for (char* const* ext = module->defaultExts; *ext != 0; ++ext) 
				{
					// greebo: File extension is expected to be UPPERCASE
					std::string extension(*ext);
					boost::algorithm::to_upper(extension);

					GlobalModelFormatManager().registerImporter(
						std::make_shared<PicoModelLoader>(module, extension)
					);
				}
			}
		}
	}
};

}
