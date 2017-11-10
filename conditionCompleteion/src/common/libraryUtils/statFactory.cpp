#include "libraryUtils/statFactory.h"
#include "libraryUtils/stats.h"
#include "libraryUtils/osdStatGather.h"
#include "cool/types.h"


namespace OSDStats {

boost::shared_ptr<StatsInterface> StatsFactory::getStatsInstance(std::string sender, ReportSourceId sourceId, Module statType)
{
        boost::shared_ptr<StatsInterface> statObj;
        switch(statType)
        {
                case TransactionLib:
				statObj.reset(new TransactionLibraryStats(sender, sourceId));
                                break;
		case TransactionMem:
                                statObj.reset(new TransactionMemoryStats(sender, sourceId));
				break;
                case ContainerService:
                                statObj.reset(new ContainerServiceStats(sender, sourceId));
                                break;
		case ContainerLib:
				statObj.reset(new ContainerLibraryStats(sender, sourceId));
				break;
		case ProxyService:
				statObj.reset(new ProxyServiceStats(sender, sourceId));
				break;
		case LlServiceStart:
				statObj.reset(new LlServiceStartStats(sender, sourceId));
				break;
		case LlElection:
				statObj.reset(new LlElectionStats(sender, sourceId));
				break;
		case LlMonitoringMgr:
				statObj.reset(new LlMonitoringMgrStats(sender, sourceId));
				break;
		case GlMonitoring:
				statObj.reset(new GlMonitoringStats(sender, sourceId));
				break;
		case GlComponentMgr:
				statObj.reset(new GlComponentMgrStats(sender, sourceId));
                                break;
                case GLStateChangeThreadMgr:
				statObj.reset(new GlComponentMgrStats(sender, sourceId));
				break;
		case ObjectService:
				statObj.reset(new ObjectServiceStats(sender, sourceId));
                                break;
		case ObjectServiceGet:
				statObj.reset(new ObjectServiceGetStats(sender, sourceId));
                                break;
		case AccountLib:
				statObj.reset(new AccountLibraryStats(sender, sourceId));
				break;
                default:
 	                        OSDLOG(DEBUG,"Invalid StatType.");
                                //TODO throw exception or return
				break;
        }
        return statObj;
}

}
