

#include "ns3/test.h"
#include "ns3/socket-factory.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/simulator.h"
#include "ns3/simple-channel.h"
#include "ns3/simple-net-device.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/config.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/ipv6-static-routing.h"
#include "ns3/ipv6-list-routing.h"
#include "ns3/node.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"

#include "ns3/ipv4-end-point.h"
#include "ns3/arp-l3-protocol.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/icmpv4-l4-protocol.h"
#include "ns3/icmpv6-l4-protocol.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-l4-protocol.h"

#include "ns3/core-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/mp-tcp-socket-factory-impl.h"

#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/tcp-newreno.h"
#include "ns3/point-to-point-module.h"
#include "ns3/pcap-file.h"
#include "ns3/tcp-option-mptcp.h"
//#include "ns3/point-to-point-channel.h"
#include <string>

NS_LOG_COMPONENT_DEFINE ("MpTcpOptionsTestSuite");

using namespace ns3;


template<class T>
class TcpOptionMpTcpTestCase : public TestCase
{
public:
    TcpOptionMpTcpTestCase(Ptr<T> configuredOption,std::string desc) : TestCase(desc)
    {
        NS_LOG_FUNCTION(this);
        m_option = configuredOption;
    }

    virtual ~TcpOptionMpTcpTestCase()
    {
        NS_LOG_FUNCTION(this);
    }

    virtual void TestSerialize(void)
    {
        NS_LOG_INFO( "option.GetSerializedSize ():" << m_option->GetSerializedSize () );
        m_buffer.AddAtStart ( m_option->GetSerializedSize ());
        m_option->Serialize( m_buffer.Begin() );


    };

    virtual void TestDeserialize(void)
    {
        T option;
        Buffer::Iterator start = m_buffer.Begin ();
        uint8_t kind = start.ReadU8 ();



        NS_TEST_EXPECT_MSG_EQ (kind, TcpOption::MPTCP, "Option number does not match MPTCP sequence number");


        uint32_t read = option.Deserialize( start );

        NS_TEST_EXPECT_MSG_EQ ( read, option.GetSerializedSize(), "PcapDiff(file, file) must always be false");

        bool res= (*m_option == option);
        NS_TEST_EXPECT_MSG_EQ ( res,true, "Option loaded after serializing/deserializing are not equal. you should investigate ");
    };


    virtual void DoRun(void)
    {
        TestSerialize();
        TestDeserialize();
    }

protected:
    Ptr<T> m_option;
    Buffer m_buffer;
};


typedef struct _crypto_material {
uint64_t keySyn;
uint64_t keySynAck;
uint32_t expectedTokenSyn;
uint32_t nonceSyn;
uint32_t nonceSynAck;
uint64_t expectedHmacSynAck;
uint32_t expectedHmacAck;
} crypto_materials_t;


/**
Used to test key/token generation
**/
class MpTcpCryptoTest : public TestCase
{
public:

  MpTcpCryptoTest(crypto_materials_t t) : TestCase("MPTCP crypto test with values ..."),m_c(t)
  {
    //!
    NS_LOG_FUNCTION(this);
  }

  virtual ~MpTcpCryptoTest()
  {
      NS_LOG_FUNCTION(this);
  }

  virtual void DoRun(void)
  {
    uint32_t result = MpTcpSubFlow::GenerateTokenForKey(m_c.keySynAck);
    NS_LOG_INFO( "Generated token "<< result << ". Expected "<< m_c.expectedTokenSyn);
    NS_TEST_EXPECT_MSG_EQ( m_c.expectedTokenSyn, result, "Token generated does not match key");
//      MpTcpSubFlow::GenerateHMac(key,nonce);

    //!
  }

protected:
  crypto_materials_t m_c;
//  uint64_t m_key;
//  uint32_t m_expectedToken;
//  uint32_t m_nonce;

};


static class TcpOptionMpTcpTestSuite : public TestSuite
{
public:
 TcpOptionMpTcpTestSuite ()
 : TestSuite ("mptcp-option", UNIT)
 {
//    for (uint8_t i=0; i< 40; i += 10)
//    {ça n

    // Notice the 'U' suffix at the end of the number . By default compiler
    // considers int as signed, thus triggering a warning

    crypto_materials_t c = {
      .keySyn = 17578475652852252522U,
      .keySynAck = 4250710109353306436U,
      /*expectedTokenSyn computed from SynAck key */
      .expectedTokenSyn = 109896498,
//      .expectedTokenSyn = 109896498,
      .nonceSyn = 4179070691,
      .nonceSynAck = 786372555,
      .expectedHmacSynAck = 17675966670101668951U,
      .expectedHmacAck = 0
//    wireshark presents it into 5 bytes, why ?
//      104752622
//1348310848
//1453523503
//3525837167
//3796638235
    };

    AddTestCase( new MpTcpCryptoTest(c), QUICK );


        ////////////////////////////////////////////////
        //// MP CAPABLE
        ////
        Ptr<TcpOptionMpTcpCapable> mpc = CreateObject<TcpOptionMpTcpCapable>(),
                mpc2 = CreateObject<TcpOptionMpTcpCapable>();
        mpc->SetRemoteKey(42);
        mpc->SetSenderKey(232323);
        AddTestCase(
            new TcpOptionMpTcpTestCase<TcpOptionMpTcpCapable> (mpc,"MP_CAPABLE with Sender & Peer keys both set"),
            QUICK
            );

       mpc2->SetSenderKey(3);
        AddTestCase(
            new TcpOptionMpTcpTestCase<TcpOptionMpTcpCapable> (mpc2,"MP_CAPABLE with only sender Key set"),
            QUICK
            );



        ////////////////////////////////////////////////
        //// MP PRIORITY
        ////
        Ptr<TcpOptionMpTcpChangePriority> prio = CreateObject<TcpOptionMpTcpChangePriority>() ,
                prio2 = CreateObject<TcpOptionMpTcpChangePriority>();

        prio->SetAddressId(3);
        AddTestCase(
            new TcpOptionMpTcpTestCase<TcpOptionMpTcpChangePriority> (prio,"Change priority for a different address"),
            QUICK
            );



        prio->SetBackupFlag(true);
        AddTestCase(
            new TcpOptionMpTcpTestCase<TcpOptionMpTcpChangePriority> (prio2,"Change priority for current address with backup flag ons"),
            QUICK
            );

        ////////////////////////////////////////////////
        //// MP REMOVE_ADDRESS
        ////
        // TODO generate random/real  addresses  with IPv4 helper ?
        Ptr<TcpOptionMpTcpRemoveAddress> rem = CreateObject<TcpOptionMpTcpRemoveAddress>();
         for (uint8_t i=0; i<15; ++i)
         {
            Ptr<TcpOptionMpTcpRemoveAddress> rem2 = CreateObject<TcpOptionMpTcpRemoveAddress>();

            rem->AddAddressId(i);
            rem2->AddAddressId(i);

            AddTestCase(
                new TcpOptionMpTcpTestCase<TcpOptionMpTcpRemoveAddress> (rem,"With X addresses"),
                QUICK
                );

            AddTestCase(
                new TcpOptionMpTcpTestCase<TcpOptionMpTcpRemoveAddress> (rem2,"With 1 address"),
                QUICK
                );

     }


        ////////////////////////////////////////////////
        //// MP ADD_ADDRESS
        ////
        Ptr<TcpOptionMpTcpAddAddress> add = CreateObject<TcpOptionMpTcpAddAddress>();
        add->SetAddress( InetSocketAddress( "123.24.23.32"), 8 );


//        add2.SetAddress( InetSocketAddress( "ffe2::1"), 4 );

        AddTestCase(
                new TcpOptionMpTcpTestCase<TcpOptionMpTcpAddAddress> (add,"AddAddress IPv4"),
                QUICK
                );


        ////////////////////////////////////////////////
        //// MP
        ////
        Ptr<TcpOptionMpTcpDSS> dsn = CreateObject<TcpOptionMpTcpDSS>(),
                dsn2 = CreateObject<TcpOptionMpTcpDSS>(),
                dsn3 = CreateObject<TcpOptionMpTcpDSS>();
        MpTcpMapping mapping;
        mapping.Configure( SequenceNumber32(54),32);
        mapping.MapToSSN( SequenceNumber32(40));


        dsn->SetMapping(mapping);


        AddTestCase(
                new TcpOptionMpTcpTestCase<TcpOptionMpTcpDSS> (dsn,"DSN mapping only"),
                QUICK
                );

        dsn2->SetDataAck(3210);
        AddTestCase(
                new TcpOptionMpTcpTestCase<TcpOptionMpTcpDSS> (dsn2,"DataAck only"),
                QUICK
                );

        dsn->SetDataAck(45000);
        AddTestCase(
                new TcpOptionMpTcpTestCase<TcpOptionMpTcpDSS> (dsn,"DataAck + DSN mapping"),
                QUICK
                );


        ////////////////////////////////////////////////
        //// MP_JOIN Initial syn
        ////
        Ptr<TcpOptionMpTcpJoin> syn = CreateObject<TcpOptionMpTcpJoin>(),
                    syn2 = CreateObject<TcpOptionMpTcpJoin>();
//                    TcpOptionMpTcpMain::CreateMpTcpOption(TcpOptionMpTcpMain::MP_JOIN);
//                     CreateObject<TcpOptionMpTcpJoin>();
        syn->SetState(TcpOptionMpTcpJoin::Syn);
        syn->SetAddressId(4);
        syn->SetPeerToken(5323);
        AddTestCase(
                new TcpOptionMpTcpTestCase<TcpOptionMpTcpJoin> ( syn, "MP_JOIN Syn"),
                QUICK
                );


        ////////////////////////////////////////////////
        //// MP_JOIN synRcvd
        ////
        Ptr<TcpOptionMpTcpJoin> jsr = CreateObject<TcpOptionMpTcpJoin>(),
            jsr2 = CreateObject<TcpOptionMpTcpJoin>();
        jsr->SetState(TcpOptionMpTcpJoin::SynAck);
        jsr->SetAddressId(4);
        jsr->SetTruncatedHmac( 522323 );
        AddTestCase(
                new TcpOptionMpTcpTestCase<TcpOptionMpTcpJoin> ( jsr, "MP_JOIN Syn Received"),
                QUICK
                );



        ////////////////////////////////////////////////
        //// MP_JOIN SynAck
        ////
        Ptr<TcpOptionMpTcpJoin> jsar = CreateObject<TcpOptionMpTcpJoin>();
        uint8_t hmac[20] = {3,0};
        jsar->SetState(TcpOptionMpTcpJoin::Ack);
        jsar->SetHmac( hmac  );
        AddTestCase(
                new TcpOptionMpTcpTestCase<TcpOptionMpTcpJoin> ( jsar, "MP_JOIN SynAck Received"),
                QUICK
                );


        ////////////////////////////////////////////////
        //// MP_FASTCLOSE
        ////
        Ptr<TcpOptionMpTcpFastClose> close= CreateObject<TcpOptionMpTcpFastClose>();
        close->SetPeerKey(3232);

        AddTestCase(
                new TcpOptionMpTcpTestCase<TcpOptionMpTcpFastClose> ( close, "MP_Fastclose"),
                QUICK
                );


//#endif
//     Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
//
//     for (uint32_t i=0; i<1000; ++i)
//     {
//         AddTestCase (new TcpOptionTSTestCase ("Testing serialization of random "
//         "values for timestamp",
//         x->GetInteger (),
//         x->GetInteger ()), TestCase::QUICK);
//     }

 }




} g_TcpOptionTestSuite;
