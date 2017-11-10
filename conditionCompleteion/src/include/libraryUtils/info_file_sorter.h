#ifndef __INFO_FILE_SORTER_H__
#define __INFO_FILE_SORTER_H__

#include <string>
#include <vector>

namespace infoFileSorter
{

class InfoFileRecord
{
public:
	virtual ~InfoFileRecord() {}
	virtual bool is_deleted() const = 0;
	virtual const char * get_name() const = 0;
};

class InfoFileIterator
{
public:
	virtual ~InfoFileIterator() {}
	virtual InfoFileRecord const * next() = 0;
	virtual InfoFileRecord const * back() = 0;
};

class InfoFileSorter
{
public:
	InfoFileSorter(std::vector<InfoFileIterator *> iterators,
			std::string marker = std::string(),
			std::string end_marker = std::string(),
			bool prefix = false);
	InfoFileRecord const * next();
private:
	std::vector<InfoFileIterator *> iterators;
	std::vector<InfoFileRecord const *> records;
	std::string end_marker;
	char target_name[256];	// TODO remove magic number
};

}

#endif
