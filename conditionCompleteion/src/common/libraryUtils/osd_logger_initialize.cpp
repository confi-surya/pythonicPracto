#include "libraryUtils/osd_logger_iniliazer.h"

object_storage_log_setup::LogSetupTool *logSetupPtr;
using namespace OSD_Logger;
using namespace object_storage_log_setup;
 
void OSDLoggerImpl::initialize_logger()
{
	if (!logSetupPtr)
	{
	        logSetupPtr =new LogSetupTool(this->log_name);
	        logSetupPtr->initializeLogSystem();
	}
	OSDLOG(INFO, objectStorageLogTag<<"object storagelogger initialized");
}

void OSDLoggerImpl::logger_reset()
{
        OSDLOG(INFO, objectStorageLogTag << "Object storage logger reset");
	if (logSetupPtr != NULL)
	{
		delete logSetupPtr;
		logSetupPtr = NULL;
	}
}

