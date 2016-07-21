#pragma once

#include "ModuleManager.h"		

/**
 * The public interface of the GenesisMetaImporter module
 */
class IGenesisMetaImporter : public IModuleInterface
{

public:

	/**
	 * Singleton-like access to IGenesisMetaImporter
	 *
	 * @return Returns GenesisMetaImporter singleton instance, loading the module on demand if needed
	 */
	static inline IGenesisMetaImporter& Get()
	{
		return FModuleManager::LoadModuleChecked<IGenesisMetaImporter>("GenesisMetaImporter");
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("GenesisMetaImporter");
	}
};

