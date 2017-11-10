#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp> 
#include <boost/python/extract.hpp>
 
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "containerLibrary/containerInfoFile_manager.h"

namespace containerInfoManagerTest
{

TEST(ContainerInfoManagerTest, contructorTest)
{
	containerInfoFile::ContainerInfoFileManager manager;
	std::list<recordStructure::ObjectRecord> objects;
	objects.push_back(recordStructure::ObjectRecord(1234, "kuchbih", 1230, 12345, "tinda", "danda"));

	std::string account_container_name("louo/name");
	boost::python::dict metadata;
        metadata["lang"] = "English";
	recordStructure::ContainerStat stat("account1",
                "container1", 1000, 1001, 2000, 1, 1024, "ABC123", "A-B-C-123", "DELETED", 1111, metadata);
	manager.add_container(account_container_name, stat);

	typedef std::list<std::pair<recordStructure::JournalOffset, recordStructure::ObjectRecord> > RecordList;
        recordStructure::JournalOffset off(0, 0);
        RecordList data;

        recordStructure::ObjectRecord record(1234, "naam-1", 1230, 12345, "saman", "anda");
        recordStructure::ObjectRecord record2(1234, "naam-do", 1230, 12345, "saman", "anda");
        recordStructure::ObjectRecord record1(1234, "naam-ek", 1230, 12345, "saman", "anda");
        recordStructure::ObjectRecord record7(1234, "naam-ek-1", 1230, 12345, "saman", "anda");

        data.push_back(std::make_pair(off, record));
        data.push_back(std::make_pair(off, record2));
        data.push_back(std::make_pair(off, record1));
        data.push_back(std::make_pair(off, record7));

	manager.flush_container(account_container_name, &data);
	boost::this_thread::sleep(boost::posix_time::milliseconds(1000)); // give thread time to execute
	boost::filesystem::remove_all(boost::filesystem::path("louo"));
}

}
