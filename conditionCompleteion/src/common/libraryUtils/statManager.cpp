#include "libraryUtils/statFactory.h"
#include "libraryUtils/statManager.h"
#include <utility>
#include <map>
#include <exception>

namespace OSDStats {

typedef std::vector< boost::shared_ptr<StatsInterface> > OsdStatsContainer;

ReportSourceIds::ReportSourceIds():lastGeneratedReportSourceId(MIN_USED_REPORT_SOURCE_ID)
{
}
ReportSourceIds::~ReportSourceIds()
{
}

ReportSourceId ReportSourceIds::getUniqueReporterSourceID()
{
        boost::mutex::scoped_lock lock(this->resourceIdLock);
        this->lastGeneratedReportSourceId++;
        OSDLOG(DEBUG,"ReportSourceIds::getUniqueReporterSourceID:savedReportSourceId:"<<this->lastGeneratedReportSourceId);
        for (;; ++this->lastGeneratedReportSourceId)
        {
                if (this->lastGeneratedReportSourceId >= MAX_USED_REPORT_SOURCE_ID)
                {
                        this->lastGeneratedReportSourceId = MIN_USED_REPORT_SOURCE_ID;
                }
                if (!this->containsReportSourceId(this->lastGeneratedReportSourceId))
                {
                        break;
                }
        }
	this->activeSourceIds.insert(this->lastGeneratedReportSourceId);
        return this->lastGeneratedReportSourceId;
}
void ReportSourceIds::removeUsedReportSourceId(ReportSourceId const & reportSourceId)
{
        boost::mutex::scoped_lock lock(this->resourceIdLock);
        if(this->containsReportSourceId(reportSourceId))
        {
                int32 numErased = this->activeSourceIds.erase(reportSourceId);
		cool::unused(numErased);
                OSDLOG(DEBUG,"ReportSourceIds:removeUsedReportSourceId:"<<numErased);
        }
        else
        {
                OSDLOG(DEBUG,"ReportSourceIds::removeUsedReportSourceId:Unable to Remove as not present in map:"
 		        <<reportSourceId);
        }
}

bool const ReportSourceIds::containsReportSourceId(ReportSourceId const & reportSourceId)
{
        return !(this->activeSourceIds.find(reportSourceId) == this->activeSourceIds.end());
}


StatsManager* StatsManager::statsManagerInstance = NULL;

StatsManager* StatsManager::getStatsManager()
{
        if(!statsManagerInstance)
        {
                statsManagerInstance = new StatsManager();
        }
        return statsManagerInstance;
}

StatsManager::StatsManager():reportSourceIds()
{
        OSDLOG(INFO,"StatsManager Created Successfully.");
}

StatsManager::~StatsManager()
{
        statsManagerInstance = NULL;
}

void StatsManager::putStatsIntoRunningStatContainer(OsdStatsContainer& runningContainer)
{
        boost::mutex::scoped_lock lock(this->activeContainerLock);
        OsdStatsContainer::iterator it = this->activeStatsContainer.begin();
        while(it != this->activeStatsContainer.end())
        {
                runningContainer.push_back(*it);
                it = this->activeStatsContainer.erase(it);
        }
        this->activeStatsContainer.clear();
}

bool StatsManager::insertStatsToActiveStatsContainer(boost::shared_ptr<StatsInterface> activeStat)
{
        try
        {
                boost::mutex::scoped_lock lock(this->activeContainerLock);
                this->activeStatsContainer.push_back(activeStat);
        }
        catch(std::exception &e)
        {
                OSDLOG(INFO,"StatsManager:: Unable to insert stat object in activeContainer.");
                return false;
        }
        return true;

}

boost::shared_ptr<StatsInterface> StatsManager::createAndRegisterStat(std::string sender, Module statType)
{
        ReportSourceId sourceId = this->reportSourceIds.getUniqueReporterSourceID();
        boost::shared_ptr<StatsInterface> statInstance(StatsFactory::getStatsInstance(sender, sourceId, statType));
        if(!this->insertStatsToActiveStatsContainer(statInstance))
        {
                OSDLOG(ERROR,"Failed to register stat in active container For Sender:"<<sender);
                this->unregisterStat(sourceId);
                //throw exception 
        }
        return statInstance;
}

void StatsManager::unregisterStat(ReportSourceId id)
{
        this->reportSourceIds.removeUsedReportSourceId(id);
}


}
