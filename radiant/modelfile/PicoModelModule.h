#pragma once

#include "idatastream.h"
#include "iarchive.h"
#include "imodule.h"
#include "imodel.h"
#include "itextstream.h"
#include "ifilesystem.h"

#include "os/path.h"
#include <stdio.h>
#include "picomodel/picomodel.h"

#include "string/case_conv.h"
#include "PicoModelLoader.h"
#include "AseExporter.h"
#include "Lwo2Exporter.h"
#include "WavefrontExporter.h"

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
					string::to_upper(extension);

					GlobalModelFormatManager().registerImporter(
						std::make_shared<PicoModelLoader>(module, extension)
					);
				}
			}
		}

		GlobalModelFormatManager().registerExporter(std::make_shared<AseExporter>());
		GlobalModelFormatManager().registerExporter(std::make_shared<Lwo2Exporter>());
		GlobalModelFormatManager().registerExporter(std::make_shared<WavefrontExporter>());
	}

private:

	static void PicoPrintFunc(int level, const char *str)
	{
		if (str == nullptr) return;

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

	static void PicoLoadFileFunc(char *name, unsigned char** buffer, int *bufSize)
	{
		std::string fixedFilename(os::standardPathWithSlash(name));

		ArchiveFilePtr file = GlobalFileSystem().openFile(fixedFilename);

		if (!file)
		{
			*buffer = nullptr;
			*bufSize = 0;
			return;
		}

		// Allocate one byte more for the trailing zero
		*buffer = reinterpret_cast<unsigned char*>(malloc(file->size() + 1));

		// we need to end the buffer with a 0
		(*buffer)[file->size()] = '\0';

		*bufSize = static_cast<int>(file->getInputStream().read(
			reinterpret_cast<InputStream::byte_type*>(*buffer),
			file->size()
		));
	}

	static void PicoFreeFileFunc(void* file)
	{
		free(file);
	}
};

}
