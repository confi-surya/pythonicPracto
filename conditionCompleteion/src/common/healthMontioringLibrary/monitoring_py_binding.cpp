#include <boost/python.hpp>
#include "healthMontioringLibrary/monitoring_library.h"

using namespace boost::python;

BOOST_PYTHON_MODULE(libHealthMonitoringLib)
{
	class_<healthMonitoring::healthMonitoringController, boost::noncopyable>("healthMonitoring", init<std::string, uint64_t, uint64_t, std::string>())
		.def(init<std::string, uint64_t, uint64_t, std::string, bool>())
		.def("start_monitoring", &healthMonitoring::healthMonitoringController::start_monitoring)
	;
}
