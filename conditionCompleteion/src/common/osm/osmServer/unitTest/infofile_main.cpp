#include <iostream>
#include <boost/shared_ptr.hpp>
#include "osm/osm_info_file.h"
#include "libraryUtils/record_structure.h"
#include "osm/global_leader.h"
#include "osm/component_balancing.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

int instantiate_GL(
	boost::shared_ptr<global_leader::GlobalLeader> &gl_obj,
	boost::shared_ptr<gl_parser::GLMsgHandler> &gl_msg_ptr,
	boost::shared_ptr<boost::thread> &gl_thread,
	boost::shared_ptr<common_parser::MessageParser> &parser_obj
)
{
	std::cout << "Yes I am GL\n";
	//boost::shared_ptr<global_leader::GlobalLeader> gl_obj(new global_leader::GlobalLeader);
	gl_obj.reset(new global_leader::GlobalLeader);
	//gl_obj.initialize();
	gl_msg_ptr = gl_obj->get_gl_msg_handler();
	//boost::shared_ptr<gl_parser::GLMsgHandler> gl_msg_ptr(gl_obj->get_gl_msg_handler());
	bool if_cluster = true;
	bool env_check = true;
	if(if_cluster and env_check)
	{
		//boost::thread gl_main_thread(&global_leader::GlobalLeader::start, gl_obj, true);
		gl_thread.reset(new boost::thread(boost::bind(&global_leader::GlobalLeader::start, gl_obj, true)));

	}
	else
	{
		// GL should run in wait mode
		gl_thread.reset(new boost::thread(boost::bind(&global_leader::GlobalLeader::start, gl_obj, false)));
	}
	common_parser::GLMsgProvider gl_msg_provider_obj;
	gl_msg_provider_obj.register_msgs(parser_obj, gl_msg_ptr);
	//parser_obj.register_gl_msg_handler(gl_msg_ptr);
}



//int main()
int main(int argc, char **argv)
{

        ::testing::InitGoogleTest(&argc, argv);
//--------------- Node Info File Record-----------------------
	
	//boost::shared_ptr<system_state::StateChangeThreadManager> state_map_mgr(new system_state::StateChangeThreadManager());
	//boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> nif_write_handler(new osm_info_file_manager::NodeInfoFileWriteHandler("/export/home/smalhotra/");
	//boost::shared_ptr<gl_monitoring::Monitoring> monitoring(new gl_monitoring::Monitoring(state_map_mgr, nif_write_handler));
	boost::shared_ptr<gl_monitoring::Monitoring> monitoring;	
        boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord1(new recordStructure::NodeInfoFileRecord("HN0101_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED)) ;
        boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord2(new recordStructure::NodeInfoFileRecord("HN0102_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED)) ;
        boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord3(new recordStructure::NodeInfoFileRecord("HN0103_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED)) ;
        boost::shared_ptr<recordStructure::NodeInfoFileRecord> nifrecord4(new recordStructure::NodeInfoFileRecord("HN0104_60123", "192.168.131.123", 60123, 61006, 61007, 61008, 61009, 61005, gl_monitoring::REGISTERED)) ;
/*      std::string node_id = "HN0104_60123";
        nifrecord1->set_node_id(node_id);
        std::string node_ip = "192.168.131.123";
        nifrecord1->set_node_ip(node_ip);
        int32_t node_port = 60123;
        nifrecord1->set_node_port(node_port);
        int status = gl_monitoring::REGISTERED;
        nifrecord1->set_status(status);

        std::cout<<"Structure Populated \n";
        std::cout<<nifrecord1->get_node_id()<<std::endl;
        std::cout<<nifrecord1->get_node_ip()<<std::endl;
        std::cout<<nifrecord1->get_node_port()<<std::endl;
        std::cout<<nifrecord1->get_status()<<std::endl;
*/
        std::string path = "/export/home/smalhotra/";
        //osm_info_file_manager::NodeInfoFileWriteHandler whandler(path);
        boost::shared_ptr<osm_info_file_manager::NodeInfoFileWriteHandler> whandler( new osm_info_file_manager::NodeInfoFileWriteHandler(path));
        boost::shared_ptr<osm_info_file_manager::NodeInfoFileReadHandler> rhandler( new osm_info_file_manager::NodeInfoFileReadHandler(path));

        //std::cout<<"NodeInfoFileWriteHandler object created"<<whandler<<std::endl;
        //std::cout<<"NodeInfoFileWriteHandler object created"<<std::endl;
	std::list<boost::shared_ptr<recordStructure::Record> > record_list;
        record_list.push_back(nifrecord1);
        record_list.push_back(nifrecord2);
        record_list.push_back(nifrecord3);
        record_list.push_back(nifrecord4);

       	whandler->add_record(record_list);
        //rhandler->recover_record();
        //whandler->update_record(nifrecord1);
        //rhandler->recover_record();
        //whandler->delete_record(osm_info_file_manager::NODE_INFO_FILE, nifrecord1);
	//osm_info_file_manager::InfoFileReadHandler *rhandler1 = new osm_info_file_manager::NodeInfoFileReadHandler(path, monitoring);
        //rhandler1->recover_record(osm_info_file_manager::NODE_INFO_FILE);
        //whandler->update_record(nifrecord1);
        //rhandler->recover_record();

	/*osm_info_file_manager::InfoFileReadHandler *rhandler2 = new osm_info_file_manager::NodeInfoFileReadHandler(path);
	std::list<std::string> l1 = rhandler2->list_node_ids();
	for (std::list<std::string>::iterator it = l1.begin(); it != l1.end(); it++)
		std::cout << *it << ' ';

		std::cout << '\n';
	*/
        //recordStructure::NodeInfoFileRecord *nifrecord2 = new recordStructure::NodeInfoFileRecord;
        //osm_info_file_manager::NodeInfoFileReadHandler rhandler(path);

        //nifrecord2 = rhandler.read_record();


//-----------------------------------------------------------------------------------------------------------
	
//------------------Target Recovery Node Finder----------------------

/*
	component_balancer::TargetRecoveryNode target;
	target.target_node();
*/

//---------------------------------------------------------------------


//--------------- Component Info File Record ------------------------------------------

/*	boost::shared_ptr<recordStructure::ComponentInfoRecord> cifrecord1(new recordStructure::ComponentInfoRecord());// = new recordStructure::ComponentInfoRecord();
	boost::shared_ptr<recordStructure::ComponentInfoRecord> cifrecord2(new recordStructure::ComponentInfoRecord());// = new recordStructure::ComponentInfoRecord();
	boost::shared_ptr<recordStructure::ComponentInfoRecord> cifrecord3(new recordStructure::ComponentInfoRecord());// = new recordStructure::ComponentInfoRecord();
	boost::shared_ptr<recordStructure::ComponentInfoRecord> cifrecord4(new recordStructure::ComponentInfoRecord());// = new recordStructure::ComponentInfoRecord();

	std::string service_id = "HN0101_60123_account-server";
        cifrecord1->set_service_id(service_id);
        std::string service_ip = "192.168.131.2";
        cifrecord1->set_service_ip(service_ip);
        int32_t service_port = 60123;
        cifrecord1->set_service_port(service_port);
	
	std::string service_id2 = "HN0102_60123_container-server";
        cifrecord2->set_service_id(service_id2);
        std::string service_ip2 = "192.168.131.12";
        cifrecord2->set_service_ip(service_ip2);
        int32_t service_port2 = 60123;
        cifrecord2->set_service_port(service_port2);

	std::string service_id3 = "HN0103_60123_accountupdater-server";
        cifrecord3->set_service_id(service_id3);
        std::string service_ip3 = "192.168.131.32";
        cifrecord3->set_service_ip(service_ip3);
        int32_t service_port3 = 60123;
        cifrecord3->set_service_port(service_port3);

	std::string service_id4 = "HN0104_60123_object-server";
        cifrecord4->set_service_id(service_id4);
        std::string service_ip4 = "192.168.131.32";
        cifrecord4->set_service_ip(service_ip4);
        int32_t service_port4 = 60123;
        cifrecord4->set_service_port(service_port4);

	std::vector<recordStructure::ComponentInfoRecord> cifr_list1;
	cifr_list1.clear();
	cifr_list1.push_back(*cifrecord1);
	std::vector<recordStructure::ComponentInfoRecord> cifr_list2;
	cifr_list2.clear();
	cifr_list2.push_back(*cifrecord2);
	std::vector<recordStructure::ComponentInfoRecord> cifr_list3;
	cifr_list3.clear();
	cifr_list3.push_back(*cifrecord3);

	boost::shared_ptr<recordStructure::CompleteComponentRecord> complete_record1( new recordStructure::CompleteComponentRecord(cifr_list1, 0.11, 11));
	boost::shared_ptr<recordStructure::CompleteComponentRecord> complete_record2(new recordStructure::CompleteComponentRecord(cifr_list2, 0.11, 11));
	boost::shared_ptr<recordStructure::CompleteComponentRecord> complete_record3(new recordStructure::CompleteComponentRecord(cifr_list3, 0.11, 11));



	//std::cout<<"Structure Populated \n";
        //std::cout<<cifrecord1->get_service_id()<<std::endl;
        //std::cout<<cifrecord1->get_service_ip()<<std::endl;
        //std::cout<<cifrecord1->get_service_port()<<std::endl;

	//std::string path = "/export/home/smalhotra/";

	boost::shared_ptr<osm_info_file_manager::ComponentInfoFileWriteHandler> whandler1( new osm_info_file_manager::ComponentInfoFileWriteHandler(path));
	whandler1->add_record(osm_info_file_manager::CONTAINER_COMPONENT_INFO_FILE, complete_record2);
	boost::shared_ptr<osm_info_file_manager::ComponentInfoFileWriteHandler> whandler2(new osm_info_file_manager::ComponentInfoFileWriteHandler(path));
	whandler2->add_record(osm_info_file_manager::ACCOUNT_COMPONENT_INFO_FILE, complete_record1);
	boost::shared_ptr<osm_info_file_manager::ComponentInfoFileWriteHandler> whandler3(new osm_info_file_manager::ComponentInfoFileWriteHandler(path));
	whandler3->add_record(osm_info_file_manager::ACCOUNTUPDATER_COMPONENT_INFO_FILE, complete_record3);
	boost::shared_ptr<osm_info_file_manager::ComponentInfoFileWriteHandler> whandler4(new osm_info_file_manager::ComponentInfoFileWriteHandler(path));
	whandler4->add_record(osm_info_file_manager::OBJECT_COMPONENT_INFO_FILE, cifrecord4);
	//whandler->add_record(osm_info_file_manager::CONTAINER_COMPONENT_INFO_FILE, cifrecord2);
	//std::cout<<"Record added"<<std::endl;
	//whandler->update_record(osm_info_file_manager::CONTAINER_COMPONENT_INFO_FILE, cifrecord3, 1);

	std::cout<<"-------------------"<<std::endl;

	boost::shared_ptr<osm_info_file_manager::ComponentInfoFileReadHandler> rhandler1( new osm_info_file_manager::ComponentInfoFileReadHandler(path));
	rhandler1->recover_record(osm_info_file_manager::ACCOUNT_COMPONENT_INFO_FILE);
	boost::shared_ptr<osm_info_file_manager::ComponentInfoFileReadHandler> rhandler2(new osm_info_file_manager::ComponentInfoFileReadHandler(path));
	rhandler2->recover_record(osm_info_file_manager::CONTAINER_COMPONENT_INFO_FILE);
	boost::shared_ptr<osm_info_file_manager::ComponentInfoFileReadHandler> rhandler3(new osm_info_file_manager::ComponentInfoFileReadHandler(path));
	rhandler3->recover_record(osm_info_file_manager::ACCOUNTUPDATER_COMPONENT_INFO_FILE);
	boost::shared_ptr<osm_info_file_manager::ComponentInfoFileReadHandler> rhandler4(new osm_info_file_manager::ComponentInfoFileReadHandler(path));
	rhandler4->recover_record(osm_info_file_manager::OBJECT_COMPONENT_INFO_FILE);
	//std::cout<<"Record Recovered."<<std::endl;

*/	
//-----------------------------------------------------------------------------------------------------------
	

//--------------- Ring File Record ----------------------------------------------------------------------------------

	//std::string path = "/export/home/smalhotra/";
/*	boost::shared_ptr<osm_info_file_manager::RingFileWriteHandler> whandler11 (new osm_info_file_manager::RingFileWriteHandler(path));
	boost::shared_ptr<osm_info_file_manager::RingFileReadHandler> rhandler11 (new osm_info_file_manager::RingFileReadHandler(path));
	
	whandler11->remakering();
	rhandler11->read_ring_file();
*/

//--------------------------------------------------------------------------------------------------------------------

//------------- Map, Tuple, Pair, list Examples ------------------------------------------------------------------------

//	std::map<std::string, tuple<std::list<recordStructure::ComponentInfoRecord>, float> > basic_map;
//	basic_map.clear();
//	float version = 1.0;
	std::string service_name = "container-server";

/*	std::list<recordStructure::ComponentInfoRecord> l1;
	l1.push_back(*cifrecord1);
	l1.push_back(*cifrecord2);
	l1.push_back(*cifrecord3);
	l1.push_back(*cifrecord1);
	l1.push_back(*cifrecord2);
	l1.push_back(*cifrecord3);
	l1.push_back(*cifrecord1);
	l1.push_back(*cifrecord2);
	l1.push_back(*cifrecord3);
	l1.push_back(*cifrecord1);
	l1.push_back(*cifrecord2);
	l1.push_back(*cifrecord3);
	l1.push_back(*cifrecord1);
	l1.push_back(*cifrecord2);
	l1.push_back(*cifrecord3);
	l1.push_back(*cifrecord1);
*/	
//	tuple<std::list<recordStructure::ComponentInfoRecord>, float> t1;
//	get<0>(t1) = l1;
//	get<1>(t1) = version;
//	basic_map.insert(std::pair<std::string, tuple<std::list<recordStructure::ComponentInfoRecord>, float> >(service_name, t1));
	
	//transient-map
//	std::map<std::string, tuple<std::list<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> > transient_map;
//	transient_map.clear();
//	std::list<std::pair<int32_t, recordStructure::ComponentInfoRecord> > l2;

/*	std::pair<int32_t, recordStructure::ComponentInfoRecord> p11;
	p11.first = 1;
	p11.second = *cifrecord1;
	l2.push_back(p11);

	std::pair<int32_t, recordStructure::ComponentInfoRecord> p12;
	p12.first = 7;
	p12.second = *cifrecord3;
	l2.push_back(p12);

	std::pair<int32_t, recordStructure::ComponentInfoRecord> p13;
	p13.first = 4;
	p13.second = *cifrecord1;
	l2.push_back(p13);

	tuple<std::list<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> t2;
	get<0>(t2) = l2;
	get<1>(t2) = version;
	transient_map.insert(std::pair<std::string, tuple<std::list<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> >(service_name, t2));
*/
	//planned-map
//	std::map<std::string, std::list<std::pair<int32_t, recordStructure::ComponentInfoRecord> > > planned_map;
//	planned_map.clear();
//	std::list<std::pair<int32_t, recordStructure::ComponentInfoRecord> > l3;
	
	/*std::pair<int32_t, recordStructure::ComponentInfoRecord> p31;
	p31.first = 0;
	p31.second = *cifrecord4;
	l3.push_back(p31);*/
/*
	std::pair<int32_t, recordStructure::ComponentInfoRecord> p32;
	p32.first = 10;
	p32.second = *cifrecord3;
	l3.push_back(p32);

	std::pair<int32_t, recordStructure::ComponentInfoRecord> p33;
	p33.first = 13;
	p33.second = *cifrecord3;
	l3.push_back(p33);
*/	
	/*std::pair<int32_t, recordStructure::ComponentInfoRecord> p35;
	p35.first = 3;
	p35.second = *cifrecord4;
	l3.push_back(p35);

	std::pair<int32_t, recordStructure::ComponentInfoRecord> p36;
	p36.first = 2;
	p36.second = *cifrecord4;
	l3.push_back(p36);

	std::pair<int32_t, recordStructure::ComponentInfoRecord> p37;
	p37.first = 5;
	p37.second = *cifrecord4;
	l3.push_back(p37);

	std::pair<int32_t, recordStructure::ComponentInfoRecord> p38;
	p38.first = 8;
	p38.second = *cifrecord4;
	l3.push_back(p38);*/

//	planned_map.insert(std::pair<std::string, std::list<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >(service_name, l3));

	std::cout<<"Reached here"<<std::endl;
	//component_balancer::ComponentBalancer cmp_bal(basic_map, transient_map, planned_map);
	boost::shared_ptr<component_manager::ComponentMapManager> map_manager(new component_manager::ComponentMapManager());
	component_balancer::ComponentBalancer *cmp_bal = new component_balancer::ComponentBalancer(map_manager);
	std::vector<std::string> node_add;
	std::vector<std::string> node_del;
	std::vector<std::string> node_rec;
	
	std::string nid = "HN0102_60123";
	std::list<std::string> list1;
	list1.push_back(nid);
	//cmp_bal.set_unhealthy_node_list(list1);
	
	node_add.clear();
	node_add.push_back("HN0101_60123_container-server");
	cmp_bal->get_new_planned_map(service_name, node_add, node_del, node_rec); 
	//node_add.push_back("HN0102_60123_container-server");

	//node_rec.push_back("HN0103_60123_container-server");
	//node_del.push_back("HN0102_60123_container-server");
	
	//cmp_bal->get_new_planned_map(service_name, node_add, node_del, node_rec); 
	
	std::cout<<"------------"<<std::endl;
	node_add.clear();
	node_add.push_back("HN0102_60123_container-server");
	//node_add.push_back("HN0103_60123_container-server");
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > > new_planned_map;
	new_planned_map = cmp_bal->get_new_planned_map(service_name, node_add, node_del, node_rec); 

	std::cout<<"Second Balancing"<<new_planned_map.size()<<std::endl;
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >::iterator it_internal_planned_map = new_planned_map.find("HN0101_60123_container-server");
	std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > arg2;
	arg2.clear();
	arg2 = it_internal_planned_map->second;
	std::cout<<"2353456346------------------ Size as in planned map: "<<arg2.size()<<std::endl;

	map_manager->update_transient_map(service_name, arg2);
	
	
	//node_add.clear();
	//node_del.clear();
	//node_del.push_back("HN0101_60123_container-server");
	//cmp_bal->get_new_planned_map(service_name, node_add, node_del, node_rec); 
	
	//node_add.clear();
	//node_add.push_back("HN0102_60123_container-server");
	
	//cmp_bal->get_new_planned_map(service_name, node_add, node_del, node_rec); 

	/*std::map<std::string, tuple<std::list<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> > map1;
	map1.clear();
	std::cout<<"Map Size before insert: "<<map1.size()<<std::endl;
	float version = 1.4;
	int32_t comp_no = 1;
	std::string service_name = "Account Service";
	std::list <std::pair<int32_t, recordStructure::ComponentInfoRecord> > l1;
	std::pair <int32_t, recordStructure::ComponentInfoRecord> p1;
	p1 = std::make_pair(comp_no, *cifrecord1);
	l1.push_back(p1);
	tuple <std::list<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> t1;
	get<0>(t1) = l1;
	std::list <std::pair<int32_t, recordStructure::ComponentInfoRecord> > l1_old = get<0>(t1);
	std::cout<<"list Size before push_back"<<l1_old.size()<<std::endl;
	get<0>(t1).push_back(p1);
	std::list <std::pair<int32_t, recordStructure::ComponentInfoRecord> > l1_new = get<0>(t1);
	std::cout<<"list Size after push_back"<<l1_new.size()<<std::endl;
	get<1>(t1) = version;
	map1.insert(std::pair<std::string, tuple<std::list<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> >(service_name, t1));

	std::cout<<"Map Size after insert: "<<map1.size()<<std::endl;
	*/

//------------------------------------------------------------------------------------------------------------
        //return 0;
	return RUN_ALL_TESTS();
}

