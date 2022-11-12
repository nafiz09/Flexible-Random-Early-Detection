/*
 
//  Network Topology
//
//   Wifi 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n5   n6   n7   n0 -------------- n1   n2   n3   n4
//                   point-to-point  |    |    |    |
//                                   *    *    *    *
//                                   AP  wifi 10.1.2.0
 
*/
 
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/tcp-westwood.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
 
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
 
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
 
NS_LOG_COMPONENT_DEFINE ("update");
 
using namespace ns3;
 
// std::ofstream goodput("./lastFiles/goodput.txt");
Ptr<PacketSink> sink;                         /* Pointer to the packet sink application */
uint64_t lastTotalRx = 0;                     /* The value of the last total received bytes */
uint32_t prev = 0;
Time prevTime = Seconds (0);
double avgThroughput = 0;
double end_to_end_delay=0;
int avgthrpt=0;
int sent_packets=0;
int recv_packets=0;
// int delay_sum=0;
int lost_packets=0;
int total_packets=0;
double packet_loss_ratio=0;
double packet_delivery_ratio=0;
// uint32_t total_dropp = 0;
// double end2end_delay=0;
 
 
//std::ofstream thrpt ("thrpt4_clred.txt", std::ios::out | std::ios::app);
//std::ofstream delayt ("delay4_clred.txt", std::ios::out | std::ios::app);
 
// Calculate throughput
static void
TraceThroughput (Ptr<FlowMonitor> monitor)
{
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
    auto itr = stats.begin ();
    Time curTime = Now ();
 
    std::ofstream thr ("wl.dat", std::ios::out | std::ios::app);
    thr<< itr->first << " "<<8 * (itr->second.txBytes - prev) / (1000  * (curTime.GetSeconds () - prevTime.GetSeconds ())) << std::endl;
    // thr <<  curTime << " " << 8 * (itr->second.txBytes - prev) / (1000 * 1000 * (curTime.GetSeconds () - prevTime.GetSeconds ())) << std::endl;
//    std::cout <<  curTime << " " << 8 * (itr->second.txBytes - prev) / (1000 * 1000 * (curTime.GetSeconds () - prevTime.GetSeconds ())) << std::endl;
    prevTime = curTime;
    prev = itr->second.txBytes;
    Simulator::Schedule (Seconds (0.2), &TraceThroughput, monitor);
}
 
 
 
int
main (int argc, char *argv[])
{
    uint32_t payloadSize = 1472;                       /* Transport layer payload size in bytes. */
    std::string dataRate = "100Mbps";                  /* Application layer datarate. */
//    std::string tcpVariant = "TcpNewReno";             /* TCP variant type. */
    std::string phyRate = "HtMcs7";                    /* Physical layer bitrate. */
    double simulationTime = 2;                        /* Simulation time in seconds. */
 
    std::string redLinkDataRate = "1.5Mbps";
    std::string redLinkDelay = "20ms";
 
    uint32_t nWifi = 100;                                /* Number of wifi STA devices. */
    int no_of_flow = 50;
    double coverage = 2;
 
//    uint32_t    nLeaf = 10;
    uint32_t    maxPackets = 100;
    bool        modeBytes  = false;
    uint32_t    queueDiscLimitPackets = 1000;
    double      minTh = 10;
    //double      minTh_2 = 20;
    double      maxTh = 30;
    uint32_t    pktSize = 1024;
    uint32_t    number_of_packets = 100; //100, 200, 300, 400, and 500;
    int  drate = number_of_packets*pktSize*8/1000;
    //unint32 to string
    std::stringstream ss;
    ss << drate;
    std::string str = ss.str();
    std::string appDataRate = str+"kbps";
    std::cout<<"appDataRate: "<<appDataRate<<std::endl;
 
    // std::string appDataRate = "10Mbps";
    std::string queueDiscType = "RED";
//    uint16_t port = 5001;fst
    std::string bottleNeckLinkBw = "1Mbps";
    std::string bottleNeckLinkDelay = "50ms";
 
 
    /* Command line argument parser setup. */
    CommandLine cmd (__FILE__);
    cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
    cmd.AddValue ("no_of_flow", "Number of flow", no_of_flow);
 
    cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
    cmd.AddValue ("dataRate", "Application data ate", dataRate);
//    cmd.AddValue ("tcpVariant", "Transport protocol to use: TcpNewReno, "
//                                "TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, "
//                                "TcpBic, TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus, TcpLedbat ", tcpVariant);
    cmd.AddValue ("phyRate", "Physical layer bitrate", phyRate);
    cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
    cmd.Parse (argc, argv);
 
    Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (pktSize));
    Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue (appDataRate));
 
    Config::SetDefault ("ns3::DropTailQueue<Packet>::MaxSize",
                        QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, maxPackets)));
 
 
 
    if (!modeBytes)
    {
        Config::SetDefault ("ns3::RedQueueDisc::MaxSize",
                            QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, queueDiscLimitPackets)));
    }
    else
    {
        Config::SetDefault ("ns3::RedQueueDisc::MaxSize",
                            QueueSizeValue (QueueSize (QueueSizeUnit::BYTES, queueDiscLimitPackets * pktSize)));
        minTh *= pktSize;
        maxTh *= pktSize;
    }
 
    Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (minTh));
    //Config::SetDefault ("ns3::RedQueueDisc::MinTh_2", DoubleValue (minTh_2));
    Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (maxTh));
    Config::SetDefault ("ns3::RedQueueDisc::LinkBandwidth", StringValue (bottleNeckLinkBw));
    Config::SetDefault ("ns3::RedQueueDisc::LinkDelay", StringValue (bottleNeckLinkDelay));
    Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (pktSize));
    Config::SetDefault("ns3::RangePropagationLossModel::MaxRange", DoubleValue(coverage));
    // Config::SetDefault("ns3::RedQueueDisc::CLRED", BooleanValue(true));
 
 
 
 
 
    NodeContainer p2pNodes;
    p2pNodes.Create (2);
 
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("50ms"));
    pointToPoint.SetQueue ("ns3::DropTailQueue");
 
    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install (p2pNodes);
 
    NodeContainer wifiStaNodes_left;
    wifiStaNodes_left.Create (nWifi);
 
    NodeContainer wifiStaNodes_right;
    wifiStaNodes_right.Create (nWifi);
 
    NodeContainer wifiApNode_left = p2pNodes.Get (0);
    NodeContainer wifiApNode_right = p2pNodes.Get (1);
 
    YansWifiChannelHelper channel_left = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy_left;
    phy_left.SetChannel (channel_left.Create ());
    phy_left.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
 
    YansWifiChannelHelper channel_right = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy_right;
    phy_right.SetChannel (channel_right.Create ());
    phy_right.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
 
    WifiHelper wifi_left;
    wifi_left.SetRemoteStationManager ("ns3::AarfWifiManager");
 
    WifiHelper wifi_right;
    wifi_right.SetRemoteStationManager ("ns3::AarfWifiManager");
 
    WifiMacHelper mac_left;
    Ssid ssid_left = Ssid ("ns-3-ssid");
    mac_left.SetType ("ns3::StaWifiMac",
                      "Ssid", SsidValue (ssid_left),
                      "ActiveProbing", BooleanValue (false));
 
    WifiMacHelper mac_right;
    Ssid ssid_right = Ssid ("ns-3-ssid");
    mac_right.SetType ("ns3::StaWifiMac",
                       "Ssid", SsidValue (ssid_right),
                       "ActiveProbing", BooleanValue (false));
 
 
    NetDeviceContainer staDevices_left;
    staDevices_left = wifi_left.Install (phy_left, mac_left, wifiStaNodes_left);
 
    NetDeviceContainer staDevices_right;
    staDevices_right = wifi_right.Install (phy_right, mac_right, wifiStaNodes_right);
 
 
    mac_left.SetType ("ns3::ApWifiMac",
                      "Ssid", SsidValue (ssid_left));
 
    NetDeviceContainer apDevices_left;
    apDevices_left = wifi_left.Install (phy_left, mac_left, wifiApNode_left);
 
    mac_right.SetType ("ns3::ApWifiMac",
                       "Ssid", SsidValue (ssid_right));
 
    NetDeviceContainer apDevices_right;
    apDevices_right = wifi_right.Install (phy_right, mac_right, wifiApNode_right);
 
    MobilityHelper mobility;
 
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue (0.0),
                                   "MinY", DoubleValue (0.0),
                                   "DeltaX", DoubleValue (5.0),
                                   "DeltaY", DoubleValue (10.0),
                                   "GridWidth", UintegerValue (3),
                                   "LayoutType", StringValue ("RowFirst"));
 
    //mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
    //                           "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
    //mobility.Install (wifiStaNodes);
 
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (wifiApNode_left);
    mobility.Install(wifiStaNodes_left);
 
    mobility.Install (wifiApNode_right);
    mobility.Install(wifiStaNodes_right); 
 
 
    //Set Low rate using LRWPAN
    // Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue ("DsssRate1Mbps"));
    // Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
 
 
 
    /* Internet stack */
    InternetStackHelper stack;
    //stack.Install (p2pNodes);
    //stack.Install (csmaNodes);
    stack.Install (wifiApNode_left);
    stack.Install (wifiStaNodes_left);
 
    stack.Install (wifiApNode_right);
    stack.Install (wifiStaNodes_right);
 
    //q disc work using traffic control helper
    TrafficControlHelper tch;
    QueueDiscContainer qdiscs,qdiscs2;
    tch.SetRootQueueDisc("ns3::RedQueueDisc");
    qdiscs2=tch.Install(p2pDevices.Get(0));
    qdiscs = tch.Install(p2pDevices.Get(1));
 
 
    Ipv4AddressHelper address;
 
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces;
    p2pInterfaces = address.Assign (p2pDevices);
 
    address.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer apInterface_left;
    apInterface_left = address.Assign (apDevices_left);
    Ipv4InterfaceContainer staInterface_left;
    staInterface_left = address.Assign (staDevices_left);
 
    address.SetBase ("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer apInterface_right;
    apInterface_right = address.Assign (apDevices_right);
    Ipv4InterfaceContainer staInterface_right;
    staInterface_right = address.Assign (staDevices_right);
 
    /* Populate routing table */
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
 
    /* Install TCP Receiver on the access point */
    Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
    em->SetAttribute ("ErrorRate", DoubleValue (0.00001));
    for(int i = 0; i < no_of_flow; i++){
        //staDevices_right.Get (i)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
 
 
        PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9+i));
        ApplicationContainer sinkApp = sinkHelper.Install (wifiStaNodes_left.Get(i));
        sink = StaticCast<PacketSink> (sinkApp.Get (0));
 
        /* Install TCP/UDP Transmitter on the station */
        OnOffHelper server ("ns3::TcpSocketFactory", (InetSocketAddress (staInterface_left.GetAddress (i), 9+i)));
        server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
        server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        server.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
        //ApplicationContainer serverApp = server.Install (csmaNodes.Get(i+1)); // server node assign
 
        ApplicationContainer serverApp = server.Install (wifiStaNodes_right.Get(i)); // server node assign
 
        /* Start Applications */
        sinkApp.Start (Seconds (0.0));
        serverApp.Start (Seconds (1.0));
    }
 
    //  Flow Monitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll ();
    Simulator::Schedule (Seconds (0 + 0.000001), &TraceThroughput, monitor);
 
    std::cout << "Running the simulation" << std::endl;
 
//    Simulator::Run ();
    Time stopTime = Seconds (3.0);
 
    Simulator::Stop (stopTime + TimeStep (1));
 
    //gen pcap file at p2p node
    // wifi_left.EnablePcap("ppp",staDevices_left.Get(1),true);
 
    phy_left.EnablePcap("pppl", staDevices_left.Get(1), true);
    phy_right.EnablePcap("pppr", staDevices_right.Get(1), true);
 
    //gen ascii trace file
    // pointToPoint.EnableAscii("proj",p2pDevices.Get(1));
 
 
    Simulator::Run ();
    flowmon.SerializeToXmlFile("test.xml", true, true);
 
    //Flow stats
    monitor->CheckForLostPackets ();
    // int total_Drop=0;
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
    int flowc=0;
    // int tdp=0;
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
 
        std::cout << "Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
        std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
        std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
        std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
        std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
 
//        std::cout << "  Delay Sum:   " << i->second.delaySum.GetSeconds() << " s\n";
 
        std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / stopTime.GetSeconds () / 1000 / 1000  << " Mbps\n";
        //thrpt<< i->first<<" "<<i->second.rxBytes * 8.0 / stopTime.GetSeconds ()  / 1000<<"\n";
 
        avgThroughput+=(i->second.rxBytes * 8.0/(i->second.timeLastRxPacket.GetSeconds()-i->second.timeFirstTxPacket.GetSeconds()));
        avgthrpt+=i->second.rxBytes * 8.0 / stopTime.GetSeconds () / 1000 / 1000;
        end_to_end_delay+=i->second.delaySum.GetSeconds();
        sent_packets += i->second.txPackets;
        recv_packets += i->second.rxPackets;
        //delayt<< i->first<<" "<<i->second.delaySum<<"\n";
 
        // total_dropp += i->second.packetsDropped;
 
        // lost_packets += (i->second.txPackets - i->second.rxPackets);
        lost_packets += i->second.lostPackets;
 
        flowc++;
    }
 
    avgThroughput/=flowc;
    avgthrpt/=flowc;
 
 
    /* Print per flow statistics */
    QueueDisc::Stats st = qdiscs.Get (0)->GetStats ();
    if (st.GetNDroppedPackets (RedQueueDisc::UNFORCED_DROP) == 0)
    {
        std::cout << "There should be some unforced drops" << std::endl;
//        exit (1);
    }
 
    if (st.GetNDroppedPackets (QueueDisc::INTERNAL_QUEUE_DROP) != 0)
    {
        std::cout << "There should be zero drops due to queue full" << std::endl;
//        exit (1);
    }
    std::cout << "*** Stats from the bottleneck queue disc ***" << std::endl;
    std::cout << st << std::endl;
 
    std::cout << "Destroying the simulation" << std::endl;
    uint32_t total_Drops=st.nTotalDroppedPackets;
 
//    std::cout<<qdiscs2.Get(0)->GetStats()<<std::endl;
 
    /* Flow Monitor File  */
//    flowMonitor->SerializeToXmlFile("test.xml",false,false);
 
 
    double averageGoodput = ((sink->GetTotalRx () * 8) / (1e6 * simulationTime));
 
    std::cout << "Flows: "<<no_of_flow<<std::endl;
    std::cout << "Average Goodput: "<<averageGoodput<<"Mbit/s" <<std::endl;
 
    // std::cout << "Average ThroughPut: "<<avgThroughput<<"Kbit/s" <<std::endl;
    std::cout << "Average ThroughPut: "<<avgThroughput<<"bit/s" <<std::endl;
    std::cout << "End to End delay: "<<end_to_end_delay<<"s" <<std::endl; //should be end to end delay/num_of_packets
    std::cout << "Average delay: "<<end_to_end_delay/flowc<<"s" <<std::endl; 
    std::cout << "Packet_loss_Ratio: "<<lost_packets*100/sent_packets<<"%" <<std::endl;
    std::cout << "Packet_Delivery_Ratio: "<<recv_packets*100/sent_packets<<"%" <<std::endl;
    std::cout << "Drop Ratio: "<<total_Drops*100/sent_packets<<"%" <<std::endl;
    std::cout << "Total_Drops: "<<total_Drops<<"" <<std::endl;
 
    std::cout << "loss_pkts: "<<lost_packets<<"" <<std::endl;
    std::cout << "Packet_sent: "<<sent_packets<<"" <<std::endl;
    std::cout << "Packet_rcvd: "<<recv_packets<<"" <<std::endl;
 
    Simulator::Destroy ();
 
    return 0;
}