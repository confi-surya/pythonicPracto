#ifndef __STATS_MANAGER_
#define __STATS_MANAGER_


#include <boost/shared_ptr.hpp>
#include <boost/python.hpp>
#include "object_storage_log_setup.h"
#include <map>
#include "stats.h"
#include "cool/boostTime.h"
#include "cool/time.h"
#include "libraryUtils/osdStatGather.h"


namespace OSDStats {

class StatUpdateHelperFunctions;
class StatsInterface;

enum
{
        GLOBAL_REPORT_SOURCE_ID = 0,
        MIN_USED_REPORT_SOURCE_ID = 10,
        MAX_USED_REPORT_SOURCE_ID = 100000000, // maximum generated report source id - it will wrap after this value
};

class ReportSourceIds
{
        public:
                ReportSourceIds();
        	~ReportSourceIds();
	        ReportSourceId  getUniqueReporterSourceID();
                void removeUsedReportSourceId(ReportSourceId const & reportSourceId);
        	bool const containsReportSourceId(ReportSourceId const & reportSourceId);
	private:
                std::set<ReportSourceId> activeSourceIds;
	        boost::mutex resourceIdLock;
        	ReportSourceId lastGeneratedReportSourceId;
};

class StatsManager
{
        public:
                ~StatsManager();
                typedef std::vector< boost::shared_ptr<StatsInterface> > OSDStatsContainer;
                boost::shared_ptr<StatsInterface> createAndRegisterStat(std::string sender, Module statType);
                // This method will add active stats into the running Stats Container
                void putStatsIntoRunningStatContainer(OSDStatsContainer& runningContainer);
                // This method will insert the newly arrived active stats to the active stats container
                // at this point these stats are active but are not yet scheduled i.e not registered in
                // running stats container
                bool insertStatsToActiveStatsContainer(boost::shared_ptr<StatsInterface> activeStat);
                void unregisterStat(ReportSourceId id);
                // This container will temporaryly hold active stats
                OSDStatsContainer activeStatsContainer;
                static StatsManager* getStatsManager();

        private:
                StatsManager();
                boost::mutex activeContainerLock;
                StatsManager(const StatsManager&);
                static StatsManager *statsManagerInstance;
                ReportSourceIds reportSourceIds;
};

#define CREATE_AND_REGISTER_STAT(statInstance, sender, statType)\
{\
        try{\
                statInstance = StatsManager::getStatsManager()->createAndRegisterStat(sender, statType);\
                OSDLOG(INFO,"CREATE_AND_REGISTER_STAT: Stats Successfully registerd for sender:"<<sender);\
        }\
        catch(std::exception &e){\
                OSDLOG(DEBUG,"Failed to register Stat for the sender: "<<sender<<" :Exception:"<<e.what());\
        }\
}
#define UNREGISTER_STAT(statInstance)\
{\
        try{\
                if(statInstance){\
                        statInstance->deactivateStat();\
                }\
        }\
        catch(std::exception &e){\
                OSDLOG(DEBUG,"Failed to unregister/deactivate Stat. Exception:"<<e.what());\
        }\
}

}



#endif
