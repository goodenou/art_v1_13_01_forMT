// ======================================================================
//
// ServiceRegistry
//
// ======================================================================

#include "art/Framework/Services/Registry/ActivityRegistry.h"
#include "art/Framework/Services/Registry/ServiceRegistry.h"

//#include "boost/thread/tss.hpp"

using fhicl::ParameterSet;
using art::ServiceRegistry;
using art::ServiceToken;

ServiceRegistry::ServiceRegistry()
: lm_( "service" )
{ }

ServiceRegistry::~ServiceRegistry()
{ }

ServiceToken
  ServiceRegistry::setContext( ServiceToken const & iNewToken )
{
  ServiceToken result(manager_);
  manager_ = iNewToken.manager_;
  return result;
}

void
  ServiceRegistry::unsetContext( ServiceToken const & iOldToken )
{
  manager_ = iOldToken.manager_;
}

ServiceToken
  ServiceRegistry::presentToken() const
{
  return manager_;
}

ServiceToken
ServiceRegistry::createSet(ParameterSets const & iPS, ActivityRegistry & reg)
{
  std::shared_ptr<ServicesManager>
    result(new ServicesManager(iPS,
                               ServiceRegistry::instance().lm_,
                               reg)
          );
  return ServiceToken(result);
}



ServiceRegistry* ServiceRegistry::s_registry = 0;
std::mutex ServiceRegistry::service_reg_mutex;

ServiceRegistry &
ServiceRegistry::instance()
{
    // First check
    if (s_registry == 0)
    {
        std::lock_guard<std::mutex> lock(service_reg_mutex);
        //std::lock_guard<std::mutex> guard(service_reg_mutex);
        if (s_registry == 0)
            s_registry = new ServiceRegistry;
    }
    return *s_registry;
}



// ======================================================================
