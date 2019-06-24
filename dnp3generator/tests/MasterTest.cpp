#define BOOST_TEST_MODULE boost_test_cidr
#include <boost/test/included/unit_test.hpp>
#include "../CidrCalculator.h"
//#include "MasterTest.h"
BOOST_AUTO_TEST_CASE( test_cidr )
{
    //CIDR Range should go from 192.12.0.0 to 192.12.255.255
    //Although we only want safe address ranges -- exclude ending in .0 and
    //.255 or having .255 in any octet.
    std::string ipaddress;
    CidrCalculator cidr("192.12.0.0/16");
    for(int BoundCheck = 1; BoundCheck < 65536; BoundCheck++)
    {
        ipaddress = cidr.GetNextIpAddress();
        if(BoundCheck == 1)
        {
            BOOST_CHECK (ipaddress == "192.12.0.1");
        }
        BOOST_CHECK (ipaddress != "192.12.0.0");
        BOOST_CHECK (ipaddress != "192.12.0.255");
        BOOST_CHECK (ipaddress != "192.12.255.0");
        BOOST_CHECK (ipaddress != "192.12.254.0");
        BOOST_CHECK (ipaddress != "192.12.255.255");
        BOOST_CHECK (ipaddress != "192.13.0.0");
    }

}
