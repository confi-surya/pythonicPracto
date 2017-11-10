#include "libraryUtils/osd_logger_iniliazer.h"
#include <boost/python.hpp>

#include "libraryUtils/stats.h"
#include "libraryUtils/statManager.h"
#include "libraryUtils/osdStatGather.h"
#include "libraryUtils/pyStatManager.h"


using namespace boost::python;

namespace bp = boost::python;



BOOST_PYTHON_MODULE(libraryUtils){

	class_<OSD_Logger::OSDLoggerImpl>("OSDLoggerImpl", init<std::string>())
		.def("logger_reset", &OSD_Logger::OSDLoggerImpl::logger_reset)
                .def("initialize_logger", &OSD_Logger::OSDLoggerImpl::initialize_logger)
	;
	class_<OSDStats::StatsInterface> ("StatsInterface", init<>())
                .def("getSender", &OSDStats::StatsInterface::getSender)
                .def("deactivateStat", &OSDStats::StatsInterface::deactivateStat)
        ;

	enum_<OSDStats::Module>("Module")
                .value("ProxyService", OSDStats::ProxyService)
                .value("ObjectService", OSDStats::ObjectService)
        ;

        class_<OSDStats::PyStatManager>("PyStatManager",init<OSDStats::Module>())
                .def("createAndRegisterStat", &OSDStats::PyStatManager::createAndRegisterStat)
        ;
	
	class_<OSDStats::ObjectServiceStats, bases<OSDStats::StatsInterface> >("ObjectServiceStats", init<std::string, int32_t>())
                .def_readwrite( "maxBandForUploadObject", &OSDStats::ObjectServiceStats::maxBandForUploadObject)
                .def_readwrite( "minBandForUploadObject", &OSDStats::ObjectServiceStats::minBandForUploadObject)
                .def_readwrite( "avgBandForUploadObject", &OSDStats::ObjectServiceStats::avgBandForUploadObject)
                .def_readwrite( "maxBandForDownloadObject", &OSDStats::ObjectServiceStats::maxBandForDownloadObject)
                .def_readwrite( "avgBandForDownloadObject", &OSDStats::ObjectServiceStats::avgBandForDownloadObject)
                .def_readwrite( "minBandForDownloadObject", &OSDStats::ObjectServiceStats::minBandForDownloadObject)
        ;

        register_ptr_to_python< boost::shared_ptr<OSDStats::StatsInterface> >();

}
