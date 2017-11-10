#ifndef __MONITORING_MOCK_H__
#define __MONITORING_MOCK_H__

#include "monitoring.h"

class MonitoringMock : public Monitoring
{
public:
        MOCK_METHOD1(add_node_for_monitoring, void(boost::shared_ptr<ServiceObject> msg));
        MOCK_METHOD0(monitor_for_recovery, void());
        MOCK_METHOD1(update_map, bool(boost::shared_ptr<comm_messages::LocalLeaderStartMonitoring> msg));
        MOCK_METHOD1(add_bucket, void(boost::shared_ptr<Bucket> bucket_ptr));
        MOCK_METHOD1(update_time_in_map, bool(std::string node_id));
        MOCK_METHOD1(add_entry_in_map, bool(boost::shared_ptr<ServiceObject> msg));
        MOCK_METHOD3(add_entries_while_recovery, void(std::string node_id, int msg_type, int status));
        MOCK_METHOD3(get_nodes_status, void(
                std::vector<std::string> &nodes_registered,
                std::vector<std::string> &nodes_failed,
                std::vector<std::string> &nodes_retired
        )); 
        MOCK_METHOD2(change_status_in_monitoring_component,  void(std::string node_id, int status));
        MOCK_METHOD1(initialize_prepare_and_add_entry, void (boost::function<void(std::string, int)> prepare_and_add_entry));

}

#endif

