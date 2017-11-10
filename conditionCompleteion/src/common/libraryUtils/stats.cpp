#include "libraryUtils/statConstants.h"
#include "cool/compile.h"
#include "libraryUtils/stats.h"

namespace OSDStats {

char const * DELIMITER = "::";
StatsInterface::StatsInterface(std::string sender, ReportSourceId sourceId)
                                :sender(sender), isActive(true), sourceId(sourceId)
{
}

StatsInterface::StatsInterface()
{
}

StatsInterface::~StatsInterface()
{
}

void StatsInterface::deactivateStat()
{
        boost::mutex::scoped_lock lock(statMutex);
        this->isActive = false;
        OSDLOG(DEBUG,"StatsInterface::deactivateStat: Stats Deactivated for Sender:"<<sender);
}

std::string StatsInterface::getSender()
{
        return this->sender;
}

StatsInterface::StatsInterface(const StatsInterface &obj)
{
         sender = obj.sender;
}

/*void StatsInterface::updateCumulativeBytesOperated(uint64_t totalBytes)
{
        //OSDLOG(DEBUG,"totalBytes: "<<totalBytes);
}

void StatsInterface::updateAverageOperationTime(double totalTime)
{
        //OSDLOG(DEBUG,"totalTime: "<<totalTime);
}

void StatsInterface::updateAverageBufferWaitTime(double averageWaitTime)
{
        //OSDLOG(DEBUG,"averageWaitTime: "<<averageWaitTime);
}*/

TransactionMemoryStats::TransactionMemoryStats(std::string sender, ReportSourceId sourceId):
			StatsInterface(sender, sourceId),
			cool::stats::StatisticMapFormatter(sourceId),
			totalNumberOfActiveRecords(0),
			totalNumberOfTriggeredRecoveryExecution(0),
			totalNumberOfTransactionFreedInTriggeredRecovery(0)
{
}

TransactionMemoryStats::~TransactionMemoryStats()
{
}

bool TransactionMemoryStats::addStatsToCounterMap(cool::stats::StatsCountersMap& transMemStat) 
{
	boost::mutex::scoped_lock lock(statMutex);
        const std::string prefix = this->sender;
        if(this->isActive)
	{
		OSDLOG(DEBUG,"Inserting Trans Lib Stats in Stat Map");
		this->insertKeyValue(transMemStat, prefix + DELIMITER + "TotalNumberOfActiveRecords", (cool::uint64)this->totalNumberOfActiveRecords);
		this->insertKeyValue(transMemStat, prefix + DELIMITER + "TotalNumberOfTriggeredRecoveryExecution", (cool::uint64)this->totalNumberOfTriggeredRecoveryExecution);
		this->insertKeyValue(transMemStat, prefix + DELIMITER + "TotalNumberOfTransactionFreedInTriggeredRecovery", (cool::uint64)this->totalNumberOfTransactionFreedInTriggeredRecovery);
		return true;
		
	}
        else
        {
                OSDLOG(DEBUG,"Inactive Transaction Library Stat.Sender:"<<prefix);
                return false;
        }
}

TransactionLibraryStats::TransactionLibraryStats(std::string sender, ReportSourceId sourceId):
			StatsInterface(sender, sourceId),
                        cool::stats::StatisticMapFormatter(sourceId),
			totalNumberOfBulkDeleteRecords(0),
			totalTimeForTransactionRecovery(0),
			totalNumberOfTransactionRecovery(0)
{
}

TransactionLibraryStats::~TransactionLibraryStats()
{
}

bool TransactionLibraryStats::addStatsToCounterMap(cool::stats::StatsCountersMap& transLibStat)
{                      
        boost::mutex::scoped_lock lock(statMutex);
        const std::string prefix = this->sender;
        if(this->isActive)
        {              
		OSDLOG(DEBUG,"Inserting Trans Lib Stats in Stat Map");
                //this->insertKeyValue(transLibStat, prefix + DELIMITER + "TotalNumberOfBulkDeleteRecords", (cool::uint64)this->totalNumberOfBulkDeleteRecords);
                this->insertKeyValue(transLibStat, prefix + DELIMITER + "TotalTimeForTransactionRecovery", this->totalTimeForTransactionRecovery);
                this->insertKeyValue(transLibStat, prefix + DELIMITER + "TotalNumberOfTransactionRecovery", (cool::uint64)this->totalNumberOfTransactionRecovery);
                return true;
                       
        }              
        else           
        {              
                OSDLOG(DEBUG,"Inactive Transaction Library Stat.Sender:"<<prefix);
                return false;
        }              
}

AccountLibraryStats::AccountLibraryStats(std::string sender, ReportSourceId sourceId):
			StatsInterface(sender, sourceId),
                        cool::stats::StatisticMapFormatter(sourceId),
			totalTimeForAccountCreation(0),
			totalTimeForContainerUpdate(0)
{
}

AccountLibraryStats::~AccountLibraryStats()
{
}

bool AccountLibraryStats::addStatsToCounterMap(cool::stats::StatsCountersMap& accLibStat)
{
        boost::mutex::scoped_lock lock(statMutex);
        const std::string prefix = this->sender;
        if(this->isActive)
        {
                OSDLOG(DEBUG,"Inserting Trans Lib Stats in Stat Map");
                this->insertKeyValue(accLibStat, prefix + DELIMITER + "TotalTimeForContainerUpdate", this->totalTimeForAccountCreation);
                this->insertKeyValue(accLibStat, prefix + DELIMITER + "TotalTimeForContainerUpdate", this->totalTimeForContainerUpdate);
		return true;

        }
        else
        {
                OSDLOG(DEBUG,"Inactive Account Library Stat.Sender:"<<prefix);
                return false;
        }
}

ContainerLibraryStats::ContainerLibraryStats(std::string sender, ReportSourceId sourceId):
			StatsInterface(sender, sourceId),
                        cool::stats::StatisticMapFormatter(sourceId),
			totalNumberOfObjectRecords(0),
                	totalNumberOfContainersCreated(0),
                	totalTimeForContainerCreation(0),
                	totalTimeForContainerRecovery(0),
			totalNumberOfContainerRecovery(0),
			totalTimeForFlushContainer(0)
{
}

ContainerLibraryStats::~ContainerLibraryStats()
{
}

bool ContainerLibraryStats::addStatsToCounterMap(cool::stats::StatsCountersMap& contLibStat)
{
	boost::mutex::scoped_lock lock(statMutex);
        const std::string prefix = this->sender;
        if(this->isActive)
        {
                OSDLOG(DEBUG,"Inserting Trans Lib Stats in Stat Map");
                this->insertKeyValue(contLibStat, prefix + DELIMITER + "TotalNumberOfObjectRecords", (cool::uint64)this->totalNumberOfObjectRecords);
                this->insertKeyValue(contLibStat, prefix + DELIMITER + "TotalNumberOfContainersCreated", (cool::uint64)this->totalNumberOfContainersCreated);
                this->insertKeyValue(contLibStat, prefix + DELIMITER + "TotalTimeForContainerRecovery", this->totalTimeForContainerRecovery);
                this->insertKeyValue(contLibStat, prefix + DELIMITER + "TotalTimeForContainerCreation", this->totalTimeForContainerCreation);
		this->insertKeyValue(contLibStat, prefix + DELIMITER + "TotalNumberOfContainerRecovery", (cool::uint64)this->totalNumberOfContainerRecovery);
		//this->insertKeyValue(contLibStat, prefix + DELIMITER + "TotalTimeForFlushContainer", this->totalTimeForFlushContainer);
                return true;

        }
        else
        {
                OSDLOG(DEBUG,"Inactive Container Library Stat.Sender:"<<prefix);
                return false;
        }
}

GLStateChangeThreadMgrStats::GLStateChangeThreadMgrStats(std::string sender, ReportSourceId sourceId):
			StatsInterface(sender, sourceId),
                        cool::stats::StatisticMapFormatter(sourceId),
			totalNumberofConnectionCreated(0),
			totalNumberofConnectionDestroyed(0),
			totalTimeForComponentBalancing(0),
			totalNumberOfComponentBalancing(0),
			waitTimeForStateChangeInitiatorLock(0)
{
}

GLStateChangeThreadMgrStats::~GLStateChangeThreadMgrStats()
{
}

bool GLStateChangeThreadMgrStats::addStatsToCounterMap(cool::stats::StatsCountersMap& globalLeaderStat)
{
        boost::mutex::scoped_lock lock(statMutex);
        const std::string prefix = this->sender;
        if(this->isActive)
        {
                this->insertKeyValue(globalLeaderStat, prefix + DELIMITER + "NumberOfconnectionCreated", (cool::uint64)this->totalNumberofConnectionCreated);
                this->insertKeyValue(globalLeaderStat, prefix + DELIMITER + "NumberOfconnectionDestroyed", (cool::uint64)this->totalNumberofConnectionDestroyed);

                //uint32_t totalNumberofActiveConnections = this->totalNumberofConnectionCreated - this->totalNumberofConnectionDestroyed;
                //OSDLOG(DEBUG,"Global Leader Stats : Total Number of Active Connections : "<< totalNumberofActiveConnections);
                //this->insertKeyValue(globalLeaderStat, prefix + DELIMITER + "NumberofActiveConnections", (cool::uint64)totalNumberofActiveConnections);
                this->insertKeyValue(globalLeaderStat, prefix + DELIMITER + "TimeForComponentBalancing", this->totalTimeForComponentBalancing);
                this->insertKeyValue(globalLeaderStat, prefix + DELIMITER + "NumberOfComponentBalancing", (cool::uint64)this->totalNumberOfComponentBalancing);
                this->insertKeyValue(globalLeaderStat, prefix + DELIMITER + "waitTimeForStateChangeInitiatorLock", this->waitTimeForStateChangeInitiatorLock);
                return true;
        }
        else
        {
                OSDLOG(DEBUG,"Inactive GL Stat Change Manager Stat.Sender:"<<prefix);
                return false;
        }

}

GlMonitoringStats::GlMonitoringStats(std::string sender, ReportSourceId sourceId):
			StatsInterface(sender, sourceId),
                        cool::stats::StatisticMapFormatter(sourceId),
			waitTimeForRecoveryIdentifierLock(0),
			totalTimeForWriteInNodeInfoFile(0)
{
}

GlMonitoringStats::~GlMonitoringStats()
{
}

bool GlMonitoringStats::addStatsToCounterMap(cool::stats::StatsCountersMap& glMonitoringStats) {

        boost::mutex::scoped_lock lock(statMutex);
        const std::string prefix = this->sender;
        if(this->isActive)
        {
                this->insertKeyValue(glMonitoringStats, prefix + DELIMITER + "waitTime For RecoveryIdentifierLock", this->waitTimeForRecoveryIdentifierLock);			
                this->insertKeyValue(glMonitoringStats, prefix + DELIMITER + "Time for write in Node Info File", this->totalTimeForWriteInNodeInfoFile);			
	return true;
        }
        else
        {
                OSDLOG(DEBUG,"Inactive GlMonitoringStats Stat.Sender:"<<prefix);
                return false;
        }

}

GlComponentMgrStats::GlComponentMgrStats(std::string sender, ReportSourceId sourceId):
                        StatsInterface(sender, sourceId),
                        cool::stats::StatisticMapFormatter(sourceId),
                        totalNumberOfVersionUpgrade(0),
                        totalTimeForWriteInCompInfoFile(0)
{
}

GlComponentMgrStats::~GlComponentMgrStats()
{
}

bool GlComponentMgrStats::addStatsToCounterMap(cool::stats::StatsCountersMap& glCompMgrStats) {

        boost::mutex::scoped_lock lock(statMutex);
        const std::string prefix = this->sender;
        if(this->isActive)
        {
                this->insertKeyValue(glCompMgrStats, prefix + DELIMITER + "Total Number of Version Upgrade", (cool::uint64)this->totalNumberOfVersionUpgrade);
                this->insertKeyValue(glCompMgrStats, prefix + DELIMITER + "Time For write in Component Info File", this->totalTimeForWriteInCompInfoFile);
        return true;
        }
        else
        {
                OSDLOG(DEBUG,"Inactive GlComponentMgrStats Stat.Sender:"<<prefix);
                return false;
        }

}



GlobalLeaderStats::GlobalLeaderStats(std::string sender, ReportSourceId sourceId):
			StatsInterface(sender, sourceId),
                        cool::stats::StatisticMapFormatter(sourceId),
			totalNumberofConnectionCreated(0),
			totalNumberofConnectionDestroyed(0),
			totalTimeForComponentBalancing(0),
                        totalNumberOfComponentBalancing(0),
                        totalNumberOfVersionUpgrade(0),
                        totalTimeForWriteInNodeInfoFile(0),
                        totalTimeForWriteInCompInfoFile(0),
                        totalTimeForWriteInGLJournalFile(0),
			waitTimeForStateChangeInitiatorLock(0),
			waitTimeForRecoveryIdentifierLock(0),
                        maxWaitingTimeForThread(0),
                        totalTimeForHTTPConnection(0)
{
}

GlobalLeaderStats::~GlobalLeaderStats()
{
}


void GlobalLeaderStats::udpateTotalNumberofConnectionCreated()
{
	boost::mutex::scoped_lock lock(connCreateMutex);
	this->totalNumberofConnectionCreated++;
}

void GlobalLeaderStats::updateTotalNumberofConnectionDestroyed()
{
	boost::mutex::scoped_lock lock(connDestMutex);
	this->totalNumberofConnectionDestroyed++;
}

bool GlobalLeaderStats::addStatsToCounterMap(cool::stats::StatsCountersMap& globalLeaderStat)
{
	boost::mutex::scoped_lock lock(statMutex);
        const std::string prefix = this->sender;
        if(this->isActive)
        {
		this->insertKeyValue(globalLeaderStat, prefix + DELIMITER + "NumberOfconnectionCreated", (cool::uint64)this->totalNumberofConnectionCreated);
		this->insertKeyValue(globalLeaderStat, prefix + DELIMITER + "NumberOfconnectionDestroyed", (cool::uint64)this->totalNumberofConnectionDestroyed);

		uint32_t totalNumberofActiveConnections = this->totalNumberofConnectionCreated - this->totalNumberofConnectionDestroyed;
		OSDLOG(DEBUG,"Global Leader Stats : Total Number of Active Connections : "<< totalNumberofActiveConnections);
		this->insertKeyValue(globalLeaderStat, prefix + DELIMITER + "NumberofActiveConnections", (cool::uint64)totalNumberofActiveConnections);
		this->insertKeyValue(globalLeaderStat, prefix + DELIMITER + "TimeForComponentBalancing", this->totalTimeForComponentBalancing);
		this->insertKeyValue(globalLeaderStat, prefix + DELIMITER + "NumberOfComponentBalancing", (cool::uint64)this->totalNumberOfComponentBalancing);
		this->insertKeyValue(globalLeaderStat, prefix + DELIMITER + "NumberOfVersionUpgrade", (cool::uint64)this->totalNumberOfVersionUpgrade);
		this->insertKeyValue(globalLeaderStat, prefix + DELIMITER + "TimeForWriteInNodeInfoFile", this->totalTimeForWriteInNodeInfoFile);
		this->insertKeyValue(globalLeaderStat, prefix + DELIMITER + "TimeForWriteInComponentInfoFile", this->totalTimeForWriteInCompInfoFile);
		this->insertKeyValue(globalLeaderStat, prefix + DELIMITER + "TimeForWriteInGLJournalFile", this->totalTimeForWriteInGLJournalFile);
		this->insertKeyValue(globalLeaderStat, prefix + DELIMITER + "waitTimeForStateChangeInitiatorLock", this->waitTimeForStateChangeInitiatorLock);
		this->insertKeyValue(globalLeaderStat, prefix + DELIMITER + "waitTimeForRecoveryIdentifierLock", this->waitTimeForRecoveryIdentifierLock);
		this->insertKeyValue(globalLeaderStat, prefix + DELIMITER + "MaximumWaitingTimeForThread", this->maxWaitingTimeForThread);
		return true;
	}
	else
        {
                OSDLOG(DEBUG,"Inactive Transaction Library Stat.Sender:"<<prefix);
                return false;
        }
	
}


LlServiceStartStats::LlServiceStartStats(std::string sender, ReportSourceId sourceId):
                        StatsInterface(sender, sourceId),
                        cool::stats::StatisticMapFormatter(sourceId),
			totalTimeForAccountUpdaterStart(0),
                	totalTimeForContainerStart(0),
                	totalTimeForObjectStart(0),
                	totalTimeForAccountStart(0),
                	totalTimeForProxyStart(0)
{
}

LlServiceStartStats::~LlServiceStartStats()
{
}

bool LlServiceStartStats::addStatsToCounterMap(cool::stats::StatsCountersMap& localLeaderStats)
{
        boost::mutex::scoped_lock lock(statMutex);
        const std::string prefix = this->sender;
        if(this->isActive)
        {
		this->insertKeyValue(localLeaderStats, prefix + DELIMITER + "Time for starting Account Updater Service", this->totalTimeForAccountUpdaterStart);
                this->insertKeyValue(localLeaderStats, prefix + DELIMITER + "Time for starting Account Service", this->totalTimeForAccountStart);
                this->insertKeyValue(localLeaderStats, prefix + DELIMITER + "Time for starting Container Service", this->totalTimeForContainerStart);
                this->insertKeyValue(localLeaderStats, prefix + DELIMITER + "Time for starting Object Service", this->totalTimeForObjectStart);
                this->insertKeyValue(localLeaderStats, prefix + DELIMITER + "Time for starting Proxy Service", this->totalTimeForProxyStart);
                return true;

        }
        else
        {
                OSDLOG(DEBUG,"Inactive Local Leader Service Start Stat.Sender:"<<prefix);
                return false;
        }
}
	
LlElectionStats::LlElectionStats(std::string sender, ReportSourceId sourceId):
			StatsInterface(sender, sourceId),
                        cool::stats::StatisticMapFormatter(sourceId),
			totalNumberOfElections(0),
                	totalTimeForOneElection(0),
                	totalNumberOfSuccessfulElection(0)
{
}

LlElectionStats::~LlElectionStats()
{
}

bool LlElectionStats::addStatsToCounterMap(cool::stats::StatsCountersMap& localLeaderStats)
{
        boost::mutex::scoped_lock lock(statMutex);
        const std::string prefix = this->sender;
        if(this->isActive)
        {
		this->insertKeyValue(localLeaderStats, prefix + DELIMITER + "Total Number of Elections ", (cool::uint64)this->totalNumberOfElections);
                this->insertKeyValue(localLeaderStats, prefix + DELIMITER + "Time for one Election ", this->totalTimeForOneElection);
                this->insertKeyValue(localLeaderStats, prefix + DELIMITER + "Total Number of successfull elections ", (cool::uint64)this->totalNumberOfSuccessfulElection);
                return true;

        }
        else
        {
                OSDLOG(DEBUG,"Inactive Local Leader Election Stat.Sender:"<<prefix);
                return false;
        }
}

LlMonitoringMgrStats::LlMonitoringMgrStats(std::string sender, ReportSourceId sourceId):
                        StatsInterface(sender, sourceId),
                        cool::stats::StatisticMapFormatter(sourceId),
			totalNumberOfHeartBeatMissed(0),
                	totalNumberOfServiceRestart(0)
{
}

LlMonitoringMgrStats::~LlMonitoringMgrStats()
{
}

bool LlMonitoringMgrStats::addStatsToCounterMap(cool::stats::StatsCountersMap& localLeaderStats)
{
        boost::mutex::scoped_lock lock(statMutex);
        const std::string prefix = this->sender;
        if(this->isActive)
        {
		this->insertKeyValue(localLeaderStats, prefix + DELIMITER + "NumberOfHeartBeatMissed", (cool::uint64)this->totalNumberOfHeartBeatMissed);
                this->insertKeyValue(localLeaderStats, prefix + DELIMITER + "NumberOfServiceRestart", (cool::uint64)this->totalNumberOfServiceRestart);
		return true;

        }
        else
        {
                OSDLOG(DEBUG,"Inactive Container Library Stat.Sender:"<<prefix);
                return false;
        }

}


LocalLeaderStats::LocalLeaderStats(std::string sender, ReportSourceId sourceId):
                        StatsInterface(sender, sourceId),
                        cool::stats::StatisticMapFormatter(sourceId),
                        totalTimeForMountingAllFs(0),
                        totalNumberOfHeartBeatMissed(0),
                        totalNumberOfServiceRestart(0),
                        totalNumberOfElections(0),
                        totalTimeForOneElection(0),
                        totalNumberOfSuccessfulElection(0),
			totalTimeForAccountUpdaterStart(0),
                	totalTimeForContainerStart(0),
                	totalTimeForObjectStart(0),
                	totalTimeForAccountStart(0),
                	totalTimeForProxyStart(0)
{
}

LocalLeaderStats::~LocalLeaderStats()
{
}

bool LocalLeaderStats::addStatsToCounterMap(cool::stats::StatsCountersMap& localLeaderStats)
{
	boost::mutex::scoped_lock lock(statMutex);
        const std::string prefix = this->sender;
        if(this->isActive)
        {	
		this->insertKeyValue(localLeaderStats, prefix + DELIMITER + "TimeForMountingAllFs", this->totalTimeForMountingAllFs);	
		this->insertKeyValue(localLeaderStats, prefix + DELIMITER + "NumberOfHeartBeatMissed", (cool::uint64)this->totalNumberOfHeartBeatMissed);	
		this->insertKeyValue(localLeaderStats, prefix + DELIMITER + "NumberOfServiceRestart", (cool::uint64)this->totalNumberOfServiceRestart);	
		this->insertKeyValue(localLeaderStats, prefix + DELIMITER + "NumberOfElections", (cool::uint64)this->totalNumberOfElections);	
		this->insertKeyValue(localLeaderStats, prefix + DELIMITER + "TimeForOneElection", this->totalTimeForOneElection);	
		this->insertKeyValue(localLeaderStats, prefix + DELIMITER + "NumberOfSuccessfulElection", (cool::uint64)this->totalNumberOfSuccessfulElection);	
		this->insertKeyValue(localLeaderStats, prefix + DELIMITER + "Time for starting Account Updater Service", this->totalTimeForAccountUpdaterStart);
		this->insertKeyValue(localLeaderStats, prefix + DELIMITER + "Time for starting Account Service", this->totalTimeForAccountStart);
		this->insertKeyValue(localLeaderStats, prefix + DELIMITER + "Time for starting Container Service", this->totalTimeForContainerStart);
		this->insertKeyValue(localLeaderStats, prefix + DELIMITER + "Time for starting Object Service", this->totalTimeForObjectStart);
		this->insertKeyValue(localLeaderStats, prefix + DELIMITER + "Time for starting Proxy Service", this->totalTimeForProxyStart);
		return true;

        }
        else
        {
                OSDLOG(DEBUG,"Inactive Container Library Stat.Sender:"<<prefix);
                return false;
        }

}

ContainerServiceStats::ContainerServiceStats(std::string sender, ReportSourceId sourceId):
                        StatsInterface(sender, sourceId),
                        cool::stats::StatisticMapFormatter(sourceId),
			totalNumberOfRecords(0)
{
}

ContainerServiceStats::~ContainerServiceStats()
{
}

void ContainerServiceStats::print_records()
{
	std::cout<<"This is set from Python : "<<this->totalNumberOfRecords<<std::endl;
}

bool ContainerServiceStats::addStatsToCounterMap(cool::stats::StatsCountersMap& contServiceStat)
{
        boost::mutex::scoped_lock lock(statMutex);
        const std::string prefix = this->sender;
        if(this->isActive)
        {
                this->insertKeyValue(contServiceStat, prefix + DELIMITER + "TotalNumberOfRecords", (cool::uint64)this->totalNumberOfRecords);
	        return true;
        }
        else
        {
                OSDLOG(DEBUG,"Inactive Container Service Stat.Sender:"<<prefix);
                return false;
        }
}

ProxyServiceStats::ProxyServiceStats(std::string sender, ReportSourceId sourceId):
                        StatsInterface(sender, sourceId),
                        cool::stats::StatisticMapFormatter(sourceId),
		        totalNumberOfRequestsReceived(0),
                        totalNumberOfSuccessResponses(0),
                        totalNumberOfFailureResponses(0),
                        totalNumberOfTimeouts(0),
                        maxWaitingTimeForThread(0),
                        totalNumberOfGlobalMapUpgrade(0)
{
}

ProxyServiceStats::~ProxyServiceStats()
{
}

bool ProxyServiceStats::addStatsToCounterMap(cool::stats::StatsCountersMap& proxyServiceStat)
{
        boost::mutex::scoped_lock lock(statMutex);
        const std::string prefix = this->sender;
        if(this->isActive)
        {
		this->insertKeyValue(proxyServiceStat, prefix + DELIMITER + "NumberOfRequestsReceived", (cool::uint64)this->totalNumberOfRequestsReceived);
		this->insertKeyValue(proxyServiceStat, prefix + DELIMITER + "NumberOfSuccessResponses", (cool::uint64)this->totalNumberOfSuccessResponses);
		this->insertKeyValue(proxyServiceStat, prefix + DELIMITER + "NumberOfFailureResponses", (cool::uint64)this->totalNumberOfFailureResponses);
		this->insertKeyValue(proxyServiceStat, prefix + DELIMITER + "NumberOfTimeouts", (cool::uint64)this->totalNumberOfTimeouts);
		this->insertKeyValue(proxyServiceStat, prefix + DELIMITER + "MaxWaitingTimeForThread", this->maxWaitingTimeForThread);
		this->insertKeyValue(proxyServiceStat, prefix + DELIMITER + "NumberOfGlobalMapUpgrade", (cool::uint64)this->totalNumberOfGlobalMapUpgrade);
		return true;
        }
        else
        {
                OSDLOG(DEBUG,"Inactive Proxy Service Stat.Sender:"<<prefix);
                return false;
        }
}

ObjectServiceGetStats::ObjectServiceGetStats(std::string sender, ReportSourceId sourceId):
			StatsInterface(sender, sourceId),
                        cool::stats::StatisticMapFormatter(sourceId),
			minBandForDownloadObject(0),
                        avgBandForDownloadObject(0)
{
}

ObjectServiceGetStats::~ObjectServiceGetStats()
{
}

bool ObjectServiceGetStats::addStatsToCounterMap(cool::stats::StatsCountersMap& objectServiceStat)
{
        boost::mutex::scoped_lock lock(statMutex);
        const std::string prefix = this->sender;
        if(this->isActive)
        {
                this->insertKeyValue(objectServiceStat, prefix + DELIMITER + "MinimumBandwidthForDownloadingAnObject ", this->minBandForDownloadObject);
                this->insertKeyValue(objectServiceStat, prefix + DELIMITER + "AverageBandwidthForDownloadingAnObject ", this->avgBandForDownloadObject);
                return true;
        }
        else
        {
                return false;
        }

}

ObjectServiceStats::ObjectServiceStats(std::string sender, ReportSourceId sourceId):
                        StatsInterface(sender, sourceId),
                        cool::stats::StatisticMapFormatter(sourceId),
		        minBandForUploadObject(0),
                        avgBandForUploadObject(0),
			maxBandForUploadObject(0),
			minBandForDownloadObject(0),
			avgBandForDownloadObject(0),
			maxBandForDownloadObject(0)
	
{
}

ObjectServiceStats::~ObjectServiceStats()
{
}

bool ObjectServiceStats::addStatsToCounterMap(cool::stats::StatsCountersMap& objectServiceStat)
{
	boost::mutex::scoped_lock lock(statMutex);
        const std::string prefix = this->sender;
        if(this->isActive)
        {
                this->insertKeyValue(objectServiceStat, prefix + DELIMITER + "MiniumBandwidthForUploadingAnObject  ", this->minBandForUploadObject);
                this->insertKeyValue(objectServiceStat, prefix + DELIMITER + "AverageBandwidthForUploadingAnObject ", this->avgBandForUploadObject);
                this->insertKeyValue(objectServiceStat, prefix + DELIMITER + "MaximumBandwidthForUploadingAnObject ", this->maxBandForUploadObject);
                this->insertKeyValue(objectServiceStat, prefix + DELIMITER + "MinimumBandwidthForDownloadingAnObject ", this->minBandForDownloadObject);
                this->insertKeyValue(objectServiceStat, prefix + DELIMITER + "AverageBandwidthForDownloadingAnObject ", this->avgBandForDownloadObject);
                this->insertKeyValue(objectServiceStat, prefix + DELIMITER + "MaximumBandwidthForDownloadingAnObject ", this->maxBandForDownloadObject);
		return true;	
	}
	else	
	{
		return false;	
	}

}


}
