#include "libraryUtils/pyStatManager.h"


namespace OSDStats {

PyStatManager::PyStatManager(Module module_name)
{
	cool::FastTime::startFastTimer();
        this->statsManager.reset(StatsManager::getStatsManager());
        this->statFramework.reset(OsdStatGatherFramework::getStatsFrameWorkReference(module_name));
        this->statFramework->initializeStatsFramework();
}

boost::shared_ptr<StatsInterface> PyStatManager::createAndRegisterStat(std::string sender, Module type)
{
	return this->statsManager->createAndRegisterStat(sender, type);
}

PyStatManager::~PyStatManager()
{

	this->statFramework->cleanup();
	cool::FastTime::finishFastTimer();
}


}
