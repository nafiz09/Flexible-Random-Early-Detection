/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 NITK Surathkal
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Phani Kiran S V S <phanikiran.harithas@gmail.com>
 *          Nichit Bodhak Goel <nichit93@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 *
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"

#include <iostream>
#include <iomanip>
#include <map>

using namespace ns3;

double avgThroughput = 0;
double end_to_end_delay = 0;
double sent_packets = 0;
double recv_packets = 0;
double lost_packets = 0;
std::ofstream throuput("throughput.txt", std::ios::out | std::ios::app);
int main (int argc, char *argv[])
{
  uint32_t    numberofnodes = 190;
  uint32_t    nLeaf = numberofnodes/2;
  uint32_t    maxPackets = 100;
  bool        modeBytes  = false;
  uint32_t    queueDiscLimitPackets = 100;
  double      minTh = 5;
  double      maxTh = 15;
  double      delta = (minTh+maxTh)/2;
  //double      Datarate1 = 110;
  double      bottleneck = 1;
  //double      rho = Datarate1/bottleneck;
  uint32_t    pktSize = 512;
  std::string appDataRate = "10Mbps";
  std::string Dataratee = "10Mbps";
  std::string queueDiscType = "RED";
  uint16_t port = 5001;
  std::string bottleNeckLinkBw = "10Mbps";
  std::string bottleNeckLinkDelay = "20ms";

  CommandLine cmd (__FILE__);
  cmd.AddValue ("nLeaf",     "Number of left and right side leaf nodes", nLeaf);
  cmd.AddValue ("maxPackets","Max Packets allowed in the device queue", maxPackets);
  cmd.AddValue ("queueDiscLimitPackets","Max Packets allowed in the queue disc", queueDiscLimitPackets);
  cmd.AddValue ("queueDiscType", "Set Queue disc type to RED or FXRED", queueDiscType);
  cmd.AddValue ("appPktSize", "Set OnOff App Packet Size", pktSize);
  cmd.AddValue ("appDataRate", "Set OnOff App DataRate", appDataRate);
  cmd.AddValue ("modeBytes", "Set Queue disc mode to Packets (false) or bytes (true)", modeBytes);

  cmd.AddValue ("redMinTh", "RED queue minimum threshold", minTh);
  cmd.AddValue ("redMaxTh", "RED queue maximum threshold", maxTh);
  cmd.Parse (argc,argv);

  if ((queueDiscType != "RED") && (queueDiscType != "FXRED"))
    {
      std::cout << "Invalid queue disc type: Use --queueDiscType=RED or --queueDiscType=FXRED" << std::endl;
      exit (1);
    }

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
  
  //Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TypeId::LookupByName("ns3::TcpNewReno")));

  Config::SetDefault ("ns3::RedQueueDisc::Miu", DoubleValue (bottleneck));
  Config::SetDefault ("ns3::RedQueueDisc::delta", DoubleValue (delta));
  Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (minTh));
  Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (maxTh));
  Config::SetDefault ("ns3::RedQueueDisc::LinkBandwidth", StringValue (bottleNeckLinkBw));
  Config::SetDefault ("ns3::RedQueueDisc::LinkDelay", StringValue (bottleNeckLinkDelay));
  Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (pktSize));

  if (queueDiscType == "FXRED")
    {
      // Turn on FXRED
      Config::SetDefault ("ns3::RedQueueDisc::FXRED", BooleanValue (true));
    }

  // Create the point-to-point link helpers
  PointToPointHelper bottleNeckLink;
  bottleNeckLink.SetDeviceAttribute  ("DataRate", StringValue (bottleNeckLinkBw));
  bottleNeckLink.SetChannelAttribute ("Delay", StringValue (bottleNeckLinkDelay));

  PointToPointHelper pointToPointLeaf;
  pointToPointLeaf.SetDeviceAttribute    ("DataRate", StringValue (Dataratee));
  pointToPointLeaf.SetChannelAttribute   ("Delay", StringValue ("10ms"));

  PointToPointDumbbellHelper d (nLeaf, pointToPointLeaf,
                                nLeaf, pointToPointLeaf,
                                bottleNeckLink);

  // Install Stack
  InternetStackHelper stack;
  for (uint32_t i = 0; i < d.LeftCount (); ++i)
    {
      stack.Install (d.GetLeft (i));
    }
  for (uint32_t i = 0; i < d.RightCount (); ++i)
    {
      stack.Install (d.GetRight (i));
    }

  stack.Install (d.GetLeft ());
  stack.Install (d.GetRight ());
  TrafficControlHelper tchBottleneck;
  QueueDiscContainer queueDiscs;
  tchBottleneck.SetRootQueueDisc ("ns3::RedQueueDisc");
  tchBottleneck.Install (d.GetLeft ()->GetDevice (0));
  queueDiscs = tchBottleneck.Install (d.GetRight ()->GetDevice (0));

  // Assign IP Addresses
  d.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"),
                         Ipv4AddressHelper ("10.2.1.0", "255.255.255.0"),
                         Ipv4AddressHelper ("10.3.1.0", "255.255.255.0"));

  // Install on/off app on all right side nodes
  OnOffHelper clientHelper ("ns3::TcpSocketFactory", Address ());
  clientHelper.SetAttribute ("OnTime", StringValue ("ns3::UniformRandomVariable[Min=0.|Max=1.]"));
  clientHelper.SetAttribute ("OffTime", StringValue ("ns3::UniformRandomVariable[Min=0.|Max=1.]"));
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
  ApplicationContainer sinkApps;
  for (uint32_t i = 0; i < d.LeftCount (); ++i)
    {
      sinkApps.Add (packetSinkHelper.Install (d.GetLeft (i)));
    }
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (30.0));

  ApplicationContainer clientApps;
  for (uint32_t i = 0; i < d.RightCount (); ++i)
    {
      // Create an on/off app sending packets to the left side
      AddressValue remoteAddress (InetSocketAddress (d.GetLeftIpv4Address (i), port));
      clientHelper.SetAttribute ("Remote", remoteAddress);
      clientApps.Add (clientHelper.Install (d.GetRight (i)));
    }
  clientApps.Start (Seconds (1.0)); // Start 1 second after sink
  clientApps.Stop (Seconds (15.0)); // Stop before the sink

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  std::cout << "Running the simulation" << std::endl;

 //  Flow Monitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll ();
    //Simulator::Schedule (Seconds (0 + 0.000001), &TraceThroughput, monitor);
 
//    Simulator::Run ();
    Time stopTime = Seconds (200);
 
    Simulator::Stop (stopTime + TimeStep (1));
 
    //gen pcap file at p2p node
    // wifi_left.EnablePcap("ppp",staDevices_left.Get(1),true);
 
    Simulator::Run ();
    flowmon.SerializeToXmlFile("test.xml", true, true);
 
    //Flow stats
    monitor->CheckForLostPackets ();
    // int total_Drop=0;
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
    int flowc=0;
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
 
        std::cout << "Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
        std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
        std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
        std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
        std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
       flowc++;
        throuput<<flowc<<" "<<i->second.rxBytes * 8.0 / stopTime.GetSeconds () / 1000<<"\n";
 
//        std::cout << "  Delay Sum:   " << i->second.delaySum.GetSeconds() << " s\n";
 
        std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / stopTime.GetSeconds () / 1000 / 1000  << " Mbps\n";
        //thrpt<< i->first<<" "<<i->second.rxBytes * 8.0 / stopTime.GetSeconds ()  / 1000<<"\n";
 
        avgThroughput+=(i->second.rxBytes * 8.0/(i->second.timeLastRxPacket.GetSeconds()-i->second.timeFirstTxPacket.GetSeconds()));
        end_to_end_delay+=i->second.delaySum.GetSeconds();
        sent_packets += i->second.txPackets;
        recv_packets += i->second.rxPackets;
        // lost_packets += (i->second.txPackets - i->second.rxPackets);
        lost_packets += i->second.lostPackets;
 
        
    }
 
    avgThroughput/=flowc;

  QueueDisc::Stats st = queueDiscs.Get (0)->GetStats ();

  if (st.GetNDroppedPackets (RedQueueDisc::UNFORCED_DROP) == 0)
    {
      std::cout << "There should be some unforced drops" << std::endl;
      exit (1);
    }

  if (st.GetNDroppedPackets (QueueDisc::INTERNAL_QUEUE_DROP) != 0)
    {
      std::cout << "There should be zero drops due to queue full" << std::endl;
      exit (1);
    }

  std::cout << "*** Stats from the bottleneck queue disc ***" << std::endl;
  std::cout << st << std::endl;
  double dr = st.nTotalDroppedPackets*100/st.nTotalSentPackets;
  std::cout << "Drop Ratio: " << dr << std::endl;
  std::cout<<"throuput: " << avgThroughput/1000/1000<<std::endl;
  std::cout<<"end_to_end_delay: " << end_to_end_delay<<std::endl;
  std::cout<<"sent_packets: " << sent_packets<<std::endl;
  std::cout<<"recv_packets: " << recv_packets<<std::endl;
  std::cout<<"lost_packets: " << lost_packets<<std::endl;
  std::cout << "Destroying the simulation" << std::endl;

  Simulator::Destroy ();
  return 0;
}
