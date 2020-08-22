#pragma once

#include "inamespace.h"

class NamespaceFactory :
	public INamespaceFactory
{
public:
	/**
	 * Creates and returns a new Namespace.
	 */
	virtual INamespacePtr createNamespace() override;

	// RegisterableModule implementation
	virtual const std::string& getName() const override;
	virtual const StringSet& getDependencies() const override;
	virtual void initialiseModule(const IApplicationContext& ctx) override;
};

