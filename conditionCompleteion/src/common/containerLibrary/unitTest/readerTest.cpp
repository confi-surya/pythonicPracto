#include <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>
#include <boost/python/extract.hpp>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

//#include "containerLibrary/file.h"
#include "containerLibrary/reader.h"
#include "containerLibrary/writer.h"

namespace readerTest
{

TEST(readerTest, writeAndReadTest)
{
	std::string testFile("writeAndReadTest.file");
	//std::string testFile("account.container.info");

	/*  std::ofstream fileTmp;
        fileTmp.open(testFile.c_str(),  std::ios::out | std::ios::binary);
	char *buff = new char[10247];
        fileTmp.write(buff, 1);
        fileTmp.close();*/

	boost::shared_ptr<fileHandler::FileHandler> file(new fileHandler::FileHandler(testFile, fileHandler::OPEN_FOR_WRITE));

	boost::shared_ptr<containerInfoFile::DataBlockBuilder> data_block_reader(new containerInfoFile::DataBlockBuilder);
	boost::shared_ptr<containerInfoFile::DataBlockWriter> data_block_writer(new containerInfoFile::DataBlockWriter);

	boost::shared_ptr<containerInfoFile::IndexBlockBuilder> index_block_reader(new containerInfoFile::IndexBlockBuilder);
	boost::shared_ptr<containerInfoFile::IndexBlockWriter> index_block_writer(new containerInfoFile::IndexBlockWriter);

	boost::shared_ptr<containerInfoFile::TrailerAndStatBlockBuilder> 
			trailer_stat_builder(new containerInfoFile::TrailerAndStatBlockBuilder);
	boost::shared_ptr<containerInfoFile::TrailerStatBlockWriter> 
			trailer_stat_writer(new containerInfoFile::TrailerStatBlockWriter);

	typedef std::list<std::pair<recordStructure::JournalOffset, recordStructure::ObjectRecord> > RecordList;
        recordStructure::JournalOffset off(0, 0);
        RecordList data;


	recordStructure::ObjectRecord record(1234, "naam", 1230, 12345, "saman", "anda");
        recordStructure::ObjectRecord record2(1234, "naam-do", 1230, 12345, "saman", "anda");
        recordStructure::ObjectRecord record1(1234, "naam-ek", 1230, 12345, "saman", "anda");

	data.push_back(std::make_pair(off, record));
        data.push_back(std::make_pair(off, record2));
        data.push_back(std::make_pair(off, record1));


	boost::python::dict metadata;
	std::cout<<"aoeuao euaoeu \n";
	metadata[std::string("lang")] = std::string("English");
	std::cout<<"aoeuao euaoeu \n";
	boost::python::extract<std::string> ex(metadata["lang"]);
	if(ex.check())
		std::cout<<"sucells full\n";
	else
		std::cout<<"uncessfull\n";
        boost::shared_ptr<recordStructure::ContainerStat> stat(new recordStructure::ContainerStat( // = trailer_stat_builder->create_stat_from_content(
		"account1", "container1", 1000, 1001, 2000, 1, 1024, "ABC123", "A-B-C-123", "DELETED", 1111, metadata));
	std::cout<<"here too\n";
	
	boost::shared_ptr<containerInfoFile::Trailer> trailer(new containerInfoFile::Trailer(0));
	trailer->unsorted_record = 3;
	ASSERT_EQ(uint64_t(3), trailer->unsorted_record);

	boost::shared_ptr<containerInfoFile::IndexBlock> index(new containerInfoFile::IndexBlock);

	std::string naam("naam");
	std::string naam_do("naam-do");
	std::string naam_ek("naam-ex");
        index->add(naam);
        index->add(naam_do);
        index->add(naam_ek);

	containerInfoFile::Writer writer(file, data_block_writer, 
						index_block_writer, trailer_stat_writer, trailer_stat_builder);
	containerInfoFile::Reader reader(file, index_block_reader, trailer_stat_builder);

	writer.write_file(&data, index, stat, trailer);
	file->close();

	file->open(testFile, fileHandler::OPEN_FOR_READ_WRITE);
	boost::tuple<boost::shared_ptr<recordStructure::ContainerStat>, \
                        boost::shared_ptr<containerInfoFile::Trailer> > blocks_tuple = reader.read_stat_trailer_block();
        boost::shared_ptr<recordStructure::ContainerStat> statSecond = blocks_tuple.get<0>();
        boost::shared_ptr<containerInfoFile::Trailer> trailer_second = blocks_tuple.get<1>();

	std::vector<char> index_buffer(trailer_second->sorted_record + trailer_second->unsorted_record);
	reader.read_index_block(&index_buffer[0], 0, 3);
	boost::shared_ptr<containerInfoFile::IndexBlock> index_second = index_block_reader->create_index_block_from_memory(&index_buffer[0], 3);


	ASSERT_EQ(int64_t(1000), statSecond->created_at);
        ASSERT_STREQ("account1", statSecond->account.c_str());
	std::string str =  boost::python::extract<std::string>(statSecond->metadata["lang"]);
        ASSERT_STREQ("English", str.c_str());


	ASSERT_TRUE(index_second->exists(naam));
        ASSERT_TRUE(index_second->exists(naam_ek));
        ASSERT_TRUE(index_second->exists(naam_do));
	std::string no_exist("no exist");
        ASSERT_FALSE(index_second->exists(no_exist));

	ASSERT_TRUE(boost::filesystem::exists(boost::filesystem::path(testFile)));
	boost::filesystem::remove(boost::filesystem::path(testFile));
}

}
