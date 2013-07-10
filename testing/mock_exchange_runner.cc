/* mock_exchange_runner.cc
   Eric Robert, March 2013
   Copyright (c) 2013 Datacratic.  All rights reserved.

   Mock exchange runner
*/

#include "soa/service/service_utils.h"
#include "mock_exchange.h"

#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/make_shared.hpp>

using namespace std;
using namespace Datacratic;

int main(int argc, char ** argv)
{
    using namespace boost::program_options;

    std::vector<std::string> bidUrls = { "localhost:12339" };
    std::vector<std::string> winUrls = { "localhost:12340" };
    int thread = 1;

    ServiceProxyArguments args;
    options_description options = args.makeProgramOptions();
    options_description more("Mock Exchange");
    more.add_options()
        ("bid-url,b", value(&bidUrls), "outgoing url for bids")
        ("win-url,w", value(&winUrls), "outgoing url for wins")
        ("thread,t", value(&thread), "number of worker thread");

    options.add(more);
    options.add_options() ("help,h", "Print this message");

    variables_map vm;
    store(command_line_parser(argc, argv) .options(options) .run(), vm);
    notify(vm);

    if (vm.count("help")) {
        cerr << options << endl;
        exit(1);
    }

    std::vector<NetworkAddress> bids;
    for(auto item : bidUrls) {
        bids.emplace_back(item);
    }

    std::vector<NetworkAddress> wins;
    for(auto item : winUrls) {
        wins.emplace_back(item);
    }

    RTBKIT::MockExchange exchange(args);
    exchange.start(thread, 0, bids, wins);

    for(;;) {
        this_thread::sleep_for(chrono::seconds(10));
    }

    return 0;
}
