#include <statsGather/statTextFormat.h>
#include <statsGather/collectStatistics.h>
#include <statsGather/statGather.h>


#include "statsGather/statsGatherProperties.h"

#include "libraryUtils/statManager.h"
#include "libraryUtils/osdStatGather.h"
#include "libraryUtils/statConstants.h"
#include "cool/sleep.h"

#include <unistd.h>

namespace OSDStats {

OSDStatsPropertyConfiguration::OSDStatsPropertyConfiguration(cool::SharedPtr<config_file::StatsConfigParser> parser_ptr, Module module_name)
{
        this->productVersionTag = parser_ptr->product_versionValue();
        this->statCollectInterval = parser_ptr->stat_collect_intervalValue();

	this->journalUnsavedDataMemoryLimitB = parser_ptr->journal_memory_limitValue();

	this->journalLimitOfUncompressedDataInFileB  = parser_ptr->journal_compress_limitValue();

	switch(module_name)
	{
		case ContainerLib:
			this->baseStatisticsDirectory = parser_ptr->contLib_base_stats_dirValue();
			break;
		case TransactionLib:
			this->baseStatisticsDirectory = parser_ptr->transLib_base_stats_dirValue();
			break;
		case GlobalLeader:
			this->baseStatisticsDirectory = parser_ptr->gl_base_stats_dirValue();
			break;
		case LocalLeader:
			this->baseStatisticsDirectory = parser_ptr->ll_base_stats_dirValue();
			break;
		case ContainerService:
			this->baseStatisticsDirectory = parser_ptr->cont_serv_stats_dirValue();
			break;
		case AccountService:
			this->baseStatisticsDirectory = parser_ptr->acc_serv_stats_dirValue();
			break;
		case ObjectService:
			this->baseStatisticsDirectory = parser_ptr->obj_serv_stats_dirValue();
			break;
		case ProxyService:
			this->baseStatisticsDirectory = parser_ptr->proxy_serv_stats_dirValue();
			break;
		case AccountLib:
			this->baseStatisticsDirectory = parser_ptr->acc_lib_stats_dirValue();
			break;
		default:
			OSDLOG(INFO, "Invalid Stat Type");
			//TODO Throw exception and exit
			break;
	}
			
}	

OSDStatsPropertyConfiguration::~OSDStatsPropertyConfiguration()
{
}

std::string OSDStatsPropertyConfiguration::getProductVersionTag()
{
        return this->productVersionTag;
}

std::string OSDStatsPropertyConfiguration::getBaseStatisticsDirectory()
{
        return this->baseStatisticsDirectory;
}

uint64_t OSDStatsPropertyConfiguration::getStatCollectInterval()
{
	return this->statCollectInterval;
}

uint64_t OSDStatsPropertyConfiguration::getJournalUnsavedDataMemoryLimitB()
{
	return this->journalUnsavedDataMemoryLimitB;
}

uint64_t OSDStatsPropertyConfiguration::getJournalLimitOfUncompressedDataInFileB()
{
	return this->journalLimitOfUncompressedDataInFileB;
}

OsdStatGatherFramework* OsdStatGatherFramework::statGatherInstance = NULL;

OsdStatGatherFramework* OsdStatGatherFramework::getStatsFrameWorkReference(Module module_name)
{
        if(!statGatherInstance)
        {
                statGatherInstance = new OsdStatGatherFramework(module_name);
        }
        return statGatherInstance;
}

OsdStatGatherFramework::OsdStatGatherFramework(Module module_name):
		 collectStat(false), 
		 statCollectorThread(NULL),
                 statGatherFramework(),
                 statCollectoreRunning(false)
{
	cool::config::ConfigurationBuilder<config_file::StatsConfigParser> config_file_builder;
	char const * const osd_config = ::getenv("OBJECT_STORAGE_CONFIG");
	if (!osd_config)
	{
		config_file_builder.readFromFile("/opt/HYDRAstor/objectStorage/configFiles/product/libConfig/perf_frame.conf");
	}
	else
	{
		std::string conf_path(osd_config);
                conf_path = conf_path + "/perf_frame.conf";
                config_file_builder.readFromFile(conf_path);
	}	
	cool::SharedPtr<config_file::StatsConfigParser> parser_ptr = config_file_builder.buildConfiguration();
	
	this->propertyConfig.reset(new OSDStatsPropertyConfiguration(parser_ptr, module_name));
        cool::stats::StatsGatherProperties properties(cool::stats::StatsGatherProperties(
                                        this->propertyConfig->getProductVersionTag(),
                                        this->propertyConfig->getBaseStatisticsDirectory())
        );
        //properties = properties.journalUnsavedDataMemoryLimitBReplaceValue(
        //                this->propertyConfig->getJournalUnsavedDataMemoryLimitB()).

        this->statGatherFramework.reset(new cool::stats::StatGatherFrameworkImpl(
                                        properties, this->statisticsAdditionalInfo));


}

OsdStatGatherFramework::~OsdStatGatherFramework()
{
	//this->statCollectorSubroutine();

        OsdStatGatherFramework::statGatherInstance = NULL;

        OSDLOG(DEBUG,"OsdStatGatherFramework Object is destroyed.");

}

void OsdStatGatherFramework::initializeStatsFramework()
{
        this->startStatCollection();
	OSDLOG(DEBUG,"Stat Collector thread will not be started");
}

void OsdStatGatherFramework::startStatCollection()
{
        // Launching the stats collector thread
        OSDLOG(INFO, "OsdStatGatherFramework::startStatCollection: Starting the Stat CollectorThread.");
        this->collectStat = true;

        statCollectorThread.reset(new boost::thread(boost::bind(
                                &OsdStatGatherFramework::statCollectorSubroutine, this)));
        this->statCollectoreRunning = true;
}

void OsdStatGatherFramework::statCollectorSubroutine()
{
	OSDLOG(INFO,"Start Collector Subroutine Thread");
   try {	

        while(this->collectStat)
        {
		uint64_t statCollectInterval = this->propertyConfig->getStatCollectInterval();
		//std::cout<<"Fetching data"<<std::endl;
                cool::TimeDuration timeTodelay = cool::TimeDuration::makeMilliseconds(statCollectInterval);
                cool::HighResolutionSleep::sleep(timeTodelay);
                OSDLOG(DEBUG,"OsdStatGatherFramework::Stat Collector Thread Running...");
                StatsManager::getStatsManager()->putStatsIntoRunningStatContainer(this->statObjectContainer);

                std::vector< boost::shared_ptr<StatsInterface> >::iterator it = this->statObjectContainer.begin();

                while(it != this->statObjectContainer.end())
                {
			//std::cout<<"Inserting in while loop"<<std::endl;
                        if(this->collectStat)
                        {
				//std::cout<<"Inserted in WHILE loop"<<std::endl;
                                std::auto_ptr<cool::stats::StatsCountersMap> statsMap(
                                                        new cool::stats::StatsCountersMap);

                                boost::posix_time::ptime baseTime = cool::getTime();
                                if((*it)->addStatsToCounterMap(*statsMap))
                                {
					//std::cout<<"Inserting data into Coolkit framework"<<std::endl;
                                        this->statGatherFramework->addStatsValues(
                                                (*it)->getSender(), baseTime, statsMap);
                                        it++;
                                }
                                else
                                {
                                        std::string sender = (*it)->getSender();
                                        this->statGatherFramework->dropSenderMetadata(sender);
                                        StatsManager::getStatsManager()->unregisterStat((*it)->sourceId);
                                        it = this->statObjectContainer.erase(it);
					it++;
                                }
				//std::cout<<"Data Inserted into Coolkit framework"<<std::endl;
                        }
                        else
                        {
                                OSDLOG(INFO,"OsdStatGatherFramework::Stat Collector Thread Exiting.");
                                break;
                        }
		}
          }
          boost::mutex::scoped_lock lock(this->threadWaiterLock);
          if(this->statCollectoreRunning)
          {
                  OSDLOG(DEBUG,"OsdStatGatherFramework:: Going to notify CORE.");
                  this->statCollectoreRunning = false;
                  this->condition.notify_one();
          }
	}
   catch(...)
	{
		OSDLOG(DEBUG, "Catching Unknown Exception");
	}
}
void OsdStatGatherFramework::cleanup()
{
        this->collectStat = false;
        boost::mutex::scoped_lock lock(this->threadWaiterLock);
        while(this->statCollectoreRunning)
        {
                OSDLOG(DEBUG,"Waiting For Stat Collector Thread to finish its work...");
                this->condition.wait(this->threadWaiterLock);
        }
        OSDLOG(INFO,"OsdStatGatherFramework::cleanup: Successful.");


}

}
