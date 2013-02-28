/* monitor_behaviour_test.cc
   Wolfgang Sourdeau, Janyary 2013
   Copyright (c) 2013 Datacratic.  All rights reserved.
   
   Functional tests for the Monitor classes
*/

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <memory>
#include "boost/shared_ptr.hpp"
#include "boost/test/unit_test.hpp"

#include "soa/jsoncpp/value.h"

#include "jml/arch/timers.h"
#include "jml/utils/testing/watchdog.h"
#include "jml/utils/pair_utils.h" // ostream << pair

#include "soa/service/service_base.h"
#include "soa/service/testing/zookeeper_temporary_server.h"

#include "rtbkit/core/monitor/monitor_client.h"
#include "rtbkit/core/monitor/monitor_endpoint.h"
#include "rtbkit/core/monitor/monitor_provider.h"

#include "mock_monitor_provider.h"

using namespace std;
using namespace Datacratic;
using namespace RTBKIT;

#if 0
/* test the MonitorEndpoint/MonitorProviderClient pair
   using 2 mock monitor providers */
BOOST_AUTO_TEST_CASE( test_monitor_endpoint )
{
    ML::Watchdog watchdog(30.0);

    auto proxies = std::make_shared<ServiceProxies>();

    MonitorEndpoint endpoint(proxies);
    endpoint.init({"parentservice1", "parentservice2"});
    endpoint.bindTcp();
    endpoint.start();
    auto waitUpdate = [&] (bool initialStatus) {
        for (auto & it: endpoint.providersStatus_) {
            auto & providerStatus = it.second;
            providerStatus.lastStatus = initialStatus;
            Date initialCheck = providerStatus.lastCheck;
            while (providerStatus.lastCheck == initialCheck) {
                ML::sleep(1);
            }
        }
    };

    MockMonitorProvider provider1;
    provider1.providerName_ = "parentservice1";
    ServiceBase parentService1("parentservice1", proxies);
    MonitorProviderClient providerClient1(proxies->zmqContext, provider1);
    providerClient1.init(proxies->config);
    providerClient1.start();

    MockMonitorProvider provider2;
    provider2.providerName_ = "parentservice2";
    ServiceBase parentService2("parentservice2", proxies);
    MonitorProviderClient providerClient2(proxies->zmqContext, provider2);
    providerClient2.init(proxies->config);
    providerClient2.start();

    /* provider1 status is false and provider2's is false
       => proxy status is false */
    cerr << ("test: "
             "provider1 status is false and provider2's is false\n"
             "=> proxy status is false\n");
    provider1.status_ = false;
    provider2.status_ = false;
    waitUpdate(true);
    BOOST_CHECK_EQUAL(endpoint.getMonitorStatus(), false);

    /* provider1 status is true but provider2's is false
       => proxy status is false */
    cerr << ("test: "
             "provider1 status is true but provider2's is false\n"
             "=> proxy status is false\n");
    provider1.status_ = true;
    waitUpdate(true);
    BOOST_CHECK_EQUAL(endpoint.getMonitorStatus(), false);

    /* provider1 status is true and provider2's is true
       => proxy status is true */
    cerr << ("test: "
             "provider1 status is true and provider2's is true\n"
             "=> proxy status is true\n");
    provider2.status_ = true;
    waitUpdate(false);
    BOOST_CHECK_EQUAL(endpoint.getMonitorStatus(), true);

    /* all providers send updates with a delay of one second
       => proxy status is true */
    cerr << ("test: "
             "all providers answer with a delay of one second\n"
             "=> proxy status is true\n");
    provider1.delay_ = 1;
    provider2.delay_ = 1;
    waitUpdate(false);
    BOOST_CHECK_EQUAL(endpoint.getMonitorStatus(), true);

    /* one providers sends updates with a delay of three seconds
       => proxy status is false */
    cerr << ("test: "
             "one provider answers with a delay of three seconds\n"
             "=> proxy status is false\n");
    provider2.delay_ = 3;
    waitUpdate(true);
    BOOST_CHECK_EQUAL(endpoint.getMonitorStatus(), false);
}
#endif

/* test the ability of a MonitorClient to update itself via http, using a
 * Monitor endpoint and zookeeper */
BOOST_AUTO_TEST_CASE( test_monitor_client )
{
    ML::Watchdog watchdog(30.0);

    ZooKeeper::TemporaryServer zookeeper;
    zookeeper.start();

    auto proxies = std::make_shared<ServiceProxies>();
    proxies->useZookeeper(ML::format("localhost:%d", zookeeper.getPort()));

    MonitorEndpoint endpoint(proxies);
    endpoint.init({"tim"});
    endpoint.bindTcp();
    endpoint.start();
    
    MonitorClient client(proxies->zmqContext);
    client.init(proxies->config);
    client.start();

    cerr << "test: expect computed status to be TRUE"
         << " after quering the Monitor" << endl;
    client.lastStatus = false;
    endpoint.providersStatus_["tim"].lastCheck = Date::now();
    endpoint.providersStatus_["tim"].lastStatus = true;
    Date initialCheck = client.lastCheck;
    while (client.lastCheck == initialCheck) {
        /* make sure the monitor does not return false */
        endpoint.providersStatus_["tim"].lastCheck = Date::now();
        ML::sleep(1);
    }
    BOOST_CHECK_EQUAL(client.getStatus(), true);

    cerr << "test: expect computed status to be FALSE"
         << " after quering the Monitor" << endl;
    client.lastStatus = true;
    endpoint.providersStatus_["tim"].lastStatus = false;
    initialCheck = client.lastCheck;
    while (client.lastCheck == initialCheck) {
        /* make sure the endpoint does not return false */
        endpoint.providersStatus_["tim"].lastCheck = Date::now();
        ML::sleep(1);
    }
    BOOST_CHECK_EQUAL(client.getStatus(), false);
}
