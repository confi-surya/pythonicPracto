#ifndef __STAT_GATHER_
#define __STAT_GATHER_

#include "statsGather/statTextFormat.h"
#include "statsGather/collectStatistics.h"
#include "statsGather/statGather.h"
#include "object_storage_log_setup.h"
#include "statsGather/statGatherImpl.h"
#include "stats.h"

namespace OSDStats {


enum Module
{
   	ContainerLib,
	TransactionLib,
	TransactionMem,
	GlobalLeader,
	GlMonitoring,
	GlComponentMgr,
	GLStateChangeThreadMgr,
	LocalLeader,
	LlServiceStart,
	LlElection,
	LlMonitoringMgr,
	AccountLib,
	ProxyService,
	ContainerService,
	ObjectServiceGet,
	ObjectService,
	AccountService
};

class OSDStatsPropertyConfiguration
{
	public:
                OSDStatsPropertyConfiguration(cool::SharedPtr<config_file::StatsConfigParser> parser_ptr, Module module_name);
		~OSDStatsPropertyConfiguration();
                std::string getProductVersionTag();
                std::string getBaseStatisticsDirectory();
                //uint64_t getWriteMergeMaxPacks();
                uint64_t getJournalUnsavedDataMemoryLimitB();
		uint64_t getJournalLimitOfUncompressedDataInFileB();
                uint64_t getStatCollectInterval();
        private:
                std::string productVersionTag;
                std::string baseStatisticsDirectory;
                uint64_t statCollectInterval;
                //uint64_t writeMergeMaxPacks;
                uint64_t journalUnsavedDataMemoryLimitB;
		uint64_t journalLimitOfUncompressedDataInFileB;
};

class OsdStatGatherFramework {

	public:
		static OsdStatGatherFramework* getStatsFrameWorkReference(Module module_name); // Create the instance of framework
		void initializeStatsFramework();
                ~OsdStatGatherFramework();
                bool collectStat;
                void cleanup();
	private:
		OsdStatGatherFramework(Module module_name);
                OsdStatGatherFramework(const OsdStatGatherFramework&);
                OsdStatGatherFramework& operator=(const OsdStatGatherFramework&);
                static OsdStatGatherFramework* statGatherInstance;
                boost::scoped_ptr<OSDStatsPropertyConfiguration> propertyConfig;
                cool::stats::StatisticsAdditionalInfo statisticsAdditionalInfo;

                // Thread that will be launched to collect stat and start the coolkit thread
                boost::scoped_ptr<boost::thread> statCollectorThread;
                boost::scoped_ptr<cool::stats::StatGatherFrameworkImpl> statGatherFramework;

                // the below vector container keep the object of each instance
                std::vector< boost::shared_ptr<StatsInterface> > statObjectContainer;
                // Method that will be run as a thread to collect stat from various registered stat object/senders
                // and submit stat to the StatGatherFrameworkImpl
                void statCollectorSubroutine();
                void startStatCollection();
                boost::mutex threadWaiterLock;
                boost::condition condition;
                bool statCollectoreRunning;
};

}

#endif
