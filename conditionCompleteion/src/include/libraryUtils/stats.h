#ifndef _OSD_STATS_
#define _OSD_STATS_

#include "statsGather/statisticsMapFormatter.h"
#include "cool/types.h"
#include "object_storage_log_setup.h"
#include "Python.h"

namespace OSDStats {

typedef int32_t ReportSourceId;
class StatsInterface
{
        protected:
               std::string sender;
               bool isActive;
               boost::mutex statMutex;
        public:
               ReportSourceId sourceId;
               void deactivateStat();
               std::string getSender();
               StatsInterface(std::string sender, ReportSourceId sourceId);
	       StatsInterface();
	       StatsInterface(const StatsInterface &obj);
               virtual ~StatsInterface();
               virtual bool addStatsToCounterMap(cool::stats::StatsCountersMap& statsMap)
	       {
			return true;
	       }
};

class TransactionMemoryStats : public StatsInterface, protected cool::stats::StatisticMapFormatter {

	public:
		TransactionMemoryStats(std::string sender, ReportSourceId sourceId);
		~TransactionMemoryStats();
		bool addStatsToCounterMap(cool::stats::StatsCountersMap& transMemStat);

		uint64_t totalNumberOfActiveRecords;
		uint64_t totalNumberOfTriggeredRecoveryExecution;
		uint64_t totalNumberOfTransactionFreedInTriggeredRecovery;
};

class TransactionLibraryStats : public StatsInterface, protected cool::stats::StatisticMapFormatter {
	public:
		TransactionLibraryStats(std::string sender, ReportSourceId sourceId);	
		~TransactionLibraryStats();
		
		bool addStatsToCounterMap(cool::stats::StatsCountersMap& transLibStat);

		uint64_t totalNumberOfBulkDeleteRecords;
		double totalTimeForTransactionRecovery;
		uint64_t totalNumberOfTransactionRecovery;
};


class AccountLibraryStats : public StatsInterface, protected cool::stats::StatisticMapFormatter {
	public:
		AccountLibraryStats(std::string sender, ReportSourceId sourceId);
		~AccountLibraryStats();

		bool addStatsToCounterMap(cool::stats::StatsCountersMap& contLibStats);

		double totalTimeForAccountCreation;
		double totalTimeForContainerUpdate;
};


class ContainerLibraryStats : public StatsInterface, protected cool::stats::StatisticMapFormatter {
	public:
		ContainerLibraryStats(std::string sender, ReportSourceId sourceId);
		~ContainerLibraryStats();
		
		bool addStatsToCounterMap(cool::stats::StatsCountersMap& contLibStats);

		uint64_t totalNumberOfObjectRecords;
		uint64_t totalNumberOfContainersCreated;
		double totalTimeForContainerCreation;
		double totalTimeForContainerRecovery;
		uint64_t totalNumberOfContainerRecovery;
		double totalTimeForFlushContainer;
};


class GlobalLeaderStats : public StatsInterface, protected cool::stats::StatisticMapFormatter {
	
	public:
		GlobalLeaderStats(std::string sender, ReportSourceId sourceId);
		~GlobalLeaderStats();
		void udpateTotalNumberofConnectionCreated();
		void updateTotalNumberofConnectionDestroyed();
		bool addStatsToCounterMap(cool::stats::StatsCountersMap& globalLeaderStats);

		uint32_t totalNumberofConnectionCreated;
		uint32_t totalNumberofConnectionDestroyed;
		double totalTimeForComponentBalancing;
		uint64_t totalNumberOfComponentBalancing;
		uint64_t totalNumberOfVersionUpgrade;
		double totalTimeForWriteInNodeInfoFile;
		double totalTimeForWriteInCompInfoFile;
		double totalTimeForWriteInGLJournalFile;
		double waitTimeForStateChangeInitiatorLock;
                double waitTimeForRecoveryIdentifierLock;
		double maxWaitingTimeForThread;
		double totalTimeForHTTPConnection;

	private:
		boost::mutex connCreateMutex;
		boost::mutex connDestMutex;
};

class GlMonitoringStats : public StatsInterface, protected cool::stats::StatisticMapFormatter {
	
	public:
		GlMonitoringStats(std::string sender, ReportSourceId sourceId);
		~GlMonitoringStats();

		bool addStatsToCounterMap(cool::stats::StatsCountersMap& glMonitoringStats);

		double waitTimeForRecoveryIdentifierLock;
		double totalTimeForWriteInNodeInfoFile;
};

class GlComponentMgrStats : public StatsInterface, protected cool::stats::StatisticMapFormatter {
        
        public:
                GlComponentMgrStats(std::string sender, ReportSourceId sourceId);
                ~GlComponentMgrStats();

                bool addStatsToCounterMap(cool::stats::StatsCountersMap& glCompMgrStats);

		uint64_t totalNumberOfVersionUpgrade;
		double totalTimeForWriteInCompInfoFile;
};

class GLStateChangeThreadMgrStats : public StatsInterface, protected cool::stats::StatisticMapFormatter {

	public:
		GLStateChangeThreadMgrStats(std::string sender, ReportSourceId sourceId);
		~GLStateChangeThreadMgrStats();

		bool addStatsToCounterMap(cool::stats::StatsCountersMap& glStateChangeMgrStats);

		uint32_t totalNumberofConnectionCreated;
		uint32_t totalNumberofConnectionDestroyed;
		double totalTimeForComponentBalancing;
		uint64_t totalNumberOfComponentBalancing;
		double waitTimeForStateChangeInitiatorLock;

};

class LlServiceStartStats : public StatsInterface, protected cool::stats::StatisticMapFormatter {

	public:
		LlServiceStartStats(std::string sender, ReportSourceId sourceId);
		~LlServiceStartStats();

		bool addStatsToCounterMap(cool::stats::StatsCountersMap& localLeaderStats);

		double totalTimeForAccountUpdaterStart;
                double totalTimeForContainerStart;
                double totalTimeForObjectStart;
                double totalTimeForAccountStart;
                double totalTimeForProxyStart;
};

class LlElectionStats : public StatsInterface, protected cool::stats::StatisticMapFormatter {
	
	public:
		LlElectionStats(std::string sender, ReportSourceId sourceId);
		~LlElectionStats();

		bool addStatsToCounterMap(cool::stats::StatsCountersMap& localLeaderStats);

		uint64_t totalNumberOfElections;
                double totalTimeForOneElection;
                uint64_t totalNumberOfSuccessfulElection;
};

class LlMonitoringMgrStats : public StatsInterface, protected cool::stats::StatisticMapFormatter {

	public:
		LlMonitoringMgrStats(std::string sender, ReportSourceId sourceId);
		~LlMonitoringMgrStats();
		
		bool addStatsToCounterMap(cool::stats::StatsCountersMap& localLeaderStats);

		uint64_t totalNumberOfHeartBeatMissed;
                uint64_t totalNumberOfServiceRestart;

};

class LocalLeaderStats : public StatsInterface, protected cool::stats::StatisticMapFormatter {

	public:
		LocalLeaderStats(std::string sender, ReportSourceId sourceId);
		~LocalLeaderStats();

		bool addStatsToCounterMap(cool::stats::StatsCountersMap& localLeaderStats);

		double totalTimeForMountingAllFs;
		uint64_t totalNumberOfHeartBeatMissed;
		uint64_t totalNumberOfServiceRestart;
		uint64_t totalNumberOfElections;
		double totalTimeForOneElection;
		uint64_t totalNumberOfSuccessfulElection;
		double totalTimeForAccountUpdaterStart;
		double totalTimeForContainerStart;
		double totalTimeForObjectStart;
		double totalTimeForAccountStart;
		double totalTimeForProxyStart;
};


class ContainerServiceStats : public StatsInterface, protected cool::stats::StatisticMapFormatter {

	public:
		ContainerServiceStats(std::string sender, ReportSourceId sourceId);
		~ContainerServiceStats();

		bool addStatsToCounterMap(cool::stats::StatsCountersMap& contServiceStat);

		void print_records();
                uint64_t totalNumberOfRecords;		
};


class ProxyServiceStats : public StatsInterface, protected cool::stats::StatisticMapFormatter {

	public:
		ProxyServiceStats(std::string sender, ReportSourceId sourceId);
		~ProxyServiceStats();

		bool addStatsToCounterMap(cool::stats::StatsCountersMap& contServiceStat);
	
		uint64_t totalNumberOfRequestsReceived;
		uint64_t totalNumberOfSuccessResponses;
		uint64_t totalNumberOfFailureResponses;
		uint64_t totalNumberOfTimeouts;
		double maxWaitingTimeForThread;
		uint64_t totalNumberOfGlobalMapUpgrade;
};


class ObjectServiceStats : public StatsInterface, protected cool::stats::StatisticMapFormatter {

        public:
		ObjectServiceStats(std::string sender, ReportSourceId sourceId);
		~ObjectServiceStats();

		bool addStatsToCounterMap(cool::stats::StatsCountersMap& objServiceStat);

		double minBandForUploadObject;
		double avgBandForUploadObject;
		double maxBandForUploadObject;
		double minBandForDownloadObject;
		double avgBandForDownloadObject;
		double maxBandForDownloadObject;
};

class ObjectServiceGetStats : public StatsInterface, protected cool::stats::StatisticMapFormatter {

	public:
		ObjectServiceGetStats(std::string sender, ReportSourceId sourceId);
		~ObjectServiceGetStats();

		bool addStatsToCounterMap(cool::stats::StatsCountersMap& objServiceStat);

		double minBandForDownloadObject;
                double avgBandForDownloadObject;
};


class AccountServiceStats : public StatsInterface, protected cool::stats::StatisticMapFormatter {
	
	public:
		AccountServiceStats(std::string sender, ReportSourceId sourceId);
		~AccountServiceStats();

		bool addStatsToCounterMap(cool::stats::StatsCountersMap& accServiceStat);

		double totalTimeForCreatingAccount;
		double totalTimeForUpdatingOneContainerInfo;

};

}

#endif
