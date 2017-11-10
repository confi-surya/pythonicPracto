#ifndef __STATS_FACTORY_
#define __STATS_FACTORY_

#include "cool/types.h"
#include <boost/shared_ptr.hpp>
#include "object_storage_log_setup.h"
#include "libraryUtils/osdStatGather.h"

namespace OSDStats {

typedef int ReportSourceId;
class StatsInterface;
class StatsFactory
{
        public:
                StatsFactory();
                ~StatsFactory();
                static boost::shared_ptr<StatsInterface> getStatsInstance(std::string sender, ReportSourceId sourceId, Module statType);

 };

}
#endif
