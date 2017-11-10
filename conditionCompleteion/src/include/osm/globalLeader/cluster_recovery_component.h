#ifndef __CLUSTER_RECOVERY_COMPONENT_99__
#define __CLUSTER_RECOVERY_COMPONENT_99__

#include "osm/globalLeader/state_change_initiator.h"
#include "osm/globalLeader/monitoring.h"


namespace cluster_recovery_component
{

class ClusterRecoveryInitiator
{
	public:
//		ClusterRecoveryInitiator() {}
		ClusterRecoveryInitiator(
			boost::shared_ptr<gl_monitoring::Monitoring> &monitoring,
			boost::shared_ptr<system_state::StateChangeThreadManager> &states_thread_mgr,
			std::string,
			uint32_t
			);
//		ClusterRecoveryInitiator(const ClusterRecoveryInitiator&) {}
		void initiate_cluster_recovery();

		boost::shared_ptr<boost::thread> thr;
	private:
		boost::shared_ptr<gl_monitoring::Monitoring> monitoring;
		boost::shared_ptr<system_state::StateChangeThreadManager> states_thread_mgr;
		std::string node_info_file_path;
		uint32_t node_add_response_timeout;
		std::vector<std::pair<std::string, int> > node_status_list_after_reclaim;
		std::vector<boost::shared_ptr<boost::thread> > reclaim_thread_queue;
		void send_node_reclaim_request(std::string node_id);

};

} // namespace cluster_recovery_component

#endif
