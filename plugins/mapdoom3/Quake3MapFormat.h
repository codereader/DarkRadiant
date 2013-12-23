#pragma once

#include "imapformat.h"
#include <boost/enable_shared_from_this.hpp>

namespace map
{

class Quake3MapFormat :
	public MapFormat,
	public boost::enable_shared_from_this<Quake3MapFormat>
{
public:
	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
	virtual void shutdownModule();

	virtual const std::string& getMapFormatName() const;
	virtual const std::string& getGameType() const;
	virtual IMapReaderPtr getMapReader(IMapImportFilter& filter) const;
	virtual IMapWriterPtr getMapWriter() const;

	virtual bool allowInfoFileCreation() const;

	virtual bool canLoad(std::istream& stream) const;
};
typedef boost::shared_ptr<Quake3MapFormat> Quake3MapFormatPtr;

} // namespace
