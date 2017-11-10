#include "stats.h"
#include "statManager.h"
#include "osdStatGather.h"


namespace OSDStats {

class PyStatManager {

	public:
		PyStatManager(Module module_name);
		~PyStatManager();
		boost::shared_ptr<StatsInterface> createAndRegisterStat(std::string sender, Module type);


	private:
		boost::shared_ptr<OsdStatGatherFramework> statFramework;
                boost::shared_ptr<StatsManager> statsManager;

		boost::shared_ptr<StatsInterface> serv_stats;
	
};


}
