#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>

namespace job
{

class WorkerThread
{
public:
        WorkerThread();
        run();
};

}
