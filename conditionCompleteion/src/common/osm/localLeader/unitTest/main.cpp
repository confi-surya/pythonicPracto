#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <boost/shared_ptr.hpp>
#include "libraryUtils/osd_logger_iniliazer.h"
#include "osm/osmServer/utils.h"

int main(int argc, char **argv)
{

        ::testing::InitGoogleTest(&argc, argv);

	boost::shared_ptr<OSD_Logger::OSDLoggerImpl>logger;
        GLUtils gl_utils_obj;
        logger.reset(new OSD_Logger::OSDLoggerImpl("osm"));
        try
        {
                logger->initialize_logger();
                OSDLOG(INFO, "logger initiallization suceussfull");
        }
        catch(...)
        {
                OSDLOG(INFO, "logger initiallization un-successfull");
	}                                            

::setenv("DEBUG_COV","DEFINED",1);
        return RUN_ALL_TESTS();
}

