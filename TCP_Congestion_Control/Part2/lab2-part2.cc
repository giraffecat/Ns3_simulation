/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 ResiliNets, ITTC, University of Kansas
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
 * Authors: Justin P. Rohrer, Truc Anh N. Nguyen <annguyen@ittc.ku.edu>, Siddharth Gangadhar <siddharth@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  http://wiki.ittc.ku.edu/resilinets
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 *
 * “TCP Westwood(+) Protocol Implementation in ns-3”
 * Siddharth Gangadhar, Trúc Anh Ngọc Nguyễn , Greeshma Umapathi, and James P.G. Sterbenz,
 * ICST SIMUTools Workshop on ns-3 (WNS3), Cannes, France, March 2013
 */

#include <iostream>
#include <fstream>
#include <string>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/error-model.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/enum.h"
#include "ns3/event-id.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Lab2_Part1");

static bool firstCwnd = true;
static bool firstSshThr = true;
// static bool firstRtt = true;
// static bool firstRto = true;
//写入文件 => 这里怎么知道需要哪一个flow呢
static Ptr<OutputStreamWrapper> cWndStream;
static Ptr<OutputStreamWrapper> ssThreshStream;
static Ptr<OutputStreamWrapper> rttStream;
static Ptr<OutputStreamWrapper> rtoStream;
static Ptr<OutputStreamWrapper> nextTxStream;
static Ptr<OutputStreamWrapper> nextRxStream;
static Ptr<OutputStreamWrapper> inFlightStream;

static Ptr<OutputStreamWrapper> goodPutStream;

static uint32_t cWndValue;
static uint32_t ssThreshValue;

// static float aggregateGoodput;
static float dest1_aggre_goodput;
static float dest2_aggre_goodput;

static void
CwndTracer (uint32_t oldval, uint32_t newval)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()<< "\t" << newval);
  if (firstCwnd)
    {
      *cWndStream->GetStream () << "0.0 " << oldval << std::endl;
      firstCwnd = false;
    }
  *cWndStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
  cWndValue = newval;

  if (!firstSshThr)
    {
      *ssThreshStream->GetStream () << Simulator::Now ().GetSeconds () << " " << ssThreshValue << std::endl;
    }
}

// static void
// SsThreshTracer (uint32_t oldval, uint32_t newval)
// {
//   if (firstSshThr)
//     {
//       *ssThreshStream->GetStream () << "0.0 " << oldval << std::endl;
//       firstSshThr = false;
//     }
//   *ssThreshStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
//   ssThreshValue = newval;

//   if (!firstCwnd)
//     {
//       *cWndStream->GetStream () << Simulator::Now ().GetSeconds () << " " << cWndValue << std::endl;
//     }
// }

// static void
// RttTracer (Time oldval, Time newval)
// {
//   if (firstRtt)
//     {
//       *rttStream->GetStream () << "0.0 " << oldval.GetSeconds () << std::endl;
//       firstRtt = false;
//     }
//   *rttStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval.GetSeconds () << std::endl;
// }

// static void
// RtoTracer (Time oldval, Time newval)
// {
//   if (firstRto)
//     {
//       *rtoStream->GetStream () << "0.0 " << oldval.GetSeconds () << std::endl;
//       firstRto = false;
//     }
//   *rtoStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval.GetSeconds () << std::endl;
// }

// static void
// NextTxTracer (SequenceNumber32 old, SequenceNumber32 nextTx)
// {
//   NS_UNUSED (old);
//   *nextTxStream->GetStream () << Simulator::Now ().GetSeconds () << " " << nextTx << std::endl;
// }

// static void
// InFlightTracer (uint32_t old, uint32_t inFlight)
// {
//   NS_UNUSED (old);
//   *inFlightStream->GetStream () << Simulator::Now ().GetSeconds () << " " << inFlight << std::endl;
// }

// static void
// NextRxTracer (SequenceNumber32 old, SequenceNumber32 nextRx)
// {
//   NS_UNUSED (old);
//   *nextRxStream->GetStream () << Simulator::Now ().GetSeconds () << " " << nextRx << std::endl;
// }

static void
TraceCwnd (std::string cwnd_tr_file_name, int nFlows)
{
  for(int i = 0; i < nFlows; i++) {
    AsciiTraceHelper ascii;
    cWndStream = ascii.CreateFileStream ((cwnd_tr_file_name).c_str ());
    std::string path = "/NodeList/1/$ns3::TcpL4Protocol/SocketList/" + std::to_string(i) + "/CongestionWindow";
    Config::ConnectWithoutContext (path, MakeCallback (&CwndTracer));
  }
}

// static void
// TraceSsThresh (std::string ssthresh_tr_file_name)
// {
//   AsciiTraceHelper ascii;
//   ssThreshStream = ascii.CreateFileStream (ssthresh_tr_file_name.c_str ());
//   Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/SlowStartThreshold", MakeCallback (&SsThreshTracer));
// }

// static void
// TraceRtt (std::string rtt_tr_file_name)
// {
//   AsciiTraceHelper ascii;
//   rttStream = ascii.CreateFileStream (rtt_tr_file_name.c_str ());
//   Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/RTT", MakeCallback (&RttTracer));
// }

// static void
// TraceRto (std::string rto_tr_file_name)
// {
//   AsciiTraceHelper ascii;
//   rtoStream = ascii.CreateFileStream (rto_tr_file_name.c_str ());
//   Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/RTO", MakeCallback (&RtoTracer));
// }

// static void
// TraceNextTx (std::string &next_tx_seq_file_name)
// {
//   AsciiTraceHelper ascii;
//   nextTxStream = ascii.CreateFileStream (next_tx_seq_file_name.c_str ());
//   Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/NextTxSequence", MakeCallback (&NextTxTracer));
// }

// static void
// TraceInFlight (std::string &in_flight_file_name)
// {
//   AsciiTraceHelper ascii;
//   inFlightStream = ascii.CreateFileStream (in_flight_file_name.c_str ());
//   Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/BytesInFlight", MakeCallback (&InFlightTracer));
// }


// static void
// TraceNextRx (std::string &next_rx_seq_file_name)
// {
//   AsciiTraceHelper ascii;
//   nextRxStream = ascii.CreateFileStream (next_rx_seq_file_name.c_str ());
//   Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/RxBuffer/NextRxSequence", MakeCallback (&NextRxTracer));
// }

//goodPutSampling
static void
GoodputSampling (int flow, ApplicationContainer app, Ptr<OutputStreamWrapper> stream)
{
  //record goodput at the end of simulation
  //Simulator::Schedule (Seconds (period), &GoodputSampling, fileName, app, stream, period);
  double goodput;
  uint64_t totalPackets = DynamicCast<PacketSink> (app.Get (0))->GetTotalRx ();
  goodput = totalPackets * 8 / 19; // bit/s
  *stream->GetStream () << Simulator::Now ().GetSeconds () << " " << goodput << std::endl;

  //print
  std::cout << "Flow " << flow << " to dest" << flow % 2 <<":";
  std::cout << " Total Bytes Received: " << totalPackets;
  std::cout << " Goodput:" << goodput << " bits/s" <<std::endl;
  if(flow % 2) {
    dest1_aggre_goodput += goodput;
  } else {
    dest2_aggre_goodput += goodput;
  }
}

//goodPutSampling
// static void
// AggregateGoodput (std::string Vname  , Ptr<OutputStreamWrapper> stream)
// {
//   *stream->GetStream () << Vname << " " << aggregateGoodput << std::endl;
//   std::cout << "aggregateGoodput:" << aggregateGoodput << std::endl;

// }

static void
AverageGoodput (int nFlows , Ptr<OutputStreamWrapper> stream)
{
  //dest1
  *stream->GetStream () << "The Aggregate Goodput of dest1: " << dest1_aggre_goodput << " bits/s" << std::endl;
  std::cout << "The Aggregate Goodput of dest1:" << dest1_aggre_goodput << " bits/s"  << std::endl;

  //dest2
  *stream->GetStream () << "The Aggregate Goodput of dest2: " << dest2_aggre_goodput << " bits/s" << std::endl;
  std::cout << "The Aggregate Goodput of dest2:" << dest2_aggre_goodput << " bits/s" <<  std::endl;
}

int main (int argc, char *argv[])
{
  std::string transport_prot = "TcpWestwood";
  double errorRate =  0.00001;
  std::string dataRate = "1Mbps";
  std::string delay = "20ms";
  std::string access_bandwidth = "100Mbps";
  std::string access_delay = "0.01ms";
  std::string access_bandwidth2 = "100Mbps";
  std::string access_delay2 = "50ms";
  bool tracing = false;
  std::string prefix_file_name = "Lab2_Part1";
  uint64_t data_mbytes = 0;
  uint32_t mtu_bytes = 400;
  uint16_t nFlows = 1;
  double duration = 10.0;
  uint32_t run = 0;
  bool flow_monitor = false;
  bool pcap = false;
  bool sack = true;
  std::string queue_disc_type = "ns3::PfifoFastQueueDisc";
  std::string recovery = "ns3::TcpClassicRecovery";


  CommandLine cmd (__FILE__);
  cmd.AddValue ("transport_prot", "Transport protocol to use: TcpNewReno, TcpLinuxReno, "
                "TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, "
                "TcpBic, TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus, TcpLedbat, "
		"TcpLp, TcpDctcp, TcpCubic, TcpBbr", transport_prot);
  cmd.AddValue ("errorRate", "Packet error rate", errorRate);
  cmd.AddValue ("dataRate", "Bottleneck dataRate", dataRate);
  cmd.AddValue ("delay", "Bottleneck delay", delay);
  cmd.AddValue ("access_bandwidth", "Access link bandwidth", access_bandwidth);
  cmd.AddValue ("access_delay", "Access link delay", access_delay);
  cmd.AddValue ("tracing", "Flag to enable/disable tracing", tracing);
  cmd.AddValue ("prefix_name", "Prefix of output trace file", prefix_file_name);
  cmd.AddValue ("data", "Number of Megabytes of data to transmit", data_mbytes);
  cmd.AddValue ("mtu", "Size of IP packets to send in bytes", mtu_bytes);
  cmd.AddValue ("nFlows", "Number of flows", nFlows);
  cmd.AddValue ("duration", "Time to allow flows to run in seconds", duration);
  cmd.AddValue ("run", "Run index (for setting repeatable seeds)", run);
  cmd.AddValue ("flow_monitor", "Enable flow monitor", flow_monitor);
  cmd.AddValue ("pcap_tracing", "Enable or disable PCAP tracing", pcap);
  cmd.AddValue ("queue_disc_type", "Queue disc type for gateway (e.g. ns3::CoDelQueueDisc)", queue_disc_type);
  cmd.AddValue ("sack", "Enable or disable SACK option", sack);
  cmd.AddValue ("recovery", "Recovery algorithm type to use (e.g., ns3::TcpPrrRecovery", recovery);
  cmd.Parse (argc, argv);

  transport_prot = std::string ("ns3::") + transport_prot;

  SeedManager::SetSeed(time(NULL));  
  // SeedManager::SetRun (run);
  // std::cout << "time" << time(NULL) <<std::endl;
  // User may find it convenient to enable logging
  //LogComponentEnable("TcpVariantsComparison", LOG_LEVEL_ALL);
  //LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
  //LogComponentEnable("PfifoFastQueueDisc", LOG_LEVEL_ALL);

  // Calculate the ADU size
  Header* temp_header = new Ipv4Header ();
  uint32_t ip_header = temp_header->GetSerializedSize ();
  NS_LOG_LOGIC ("IP Header size is: " << ip_header);
  delete temp_header;
  temp_header = new TcpHeader ();
  uint32_t tcp_header = temp_header->GetSerializedSize ();
  NS_LOG_LOGIC ("TCP Header size is: " << tcp_header);
  delete temp_header;
  uint32_t tcp_adu_size = mtu_bytes - 20 - (ip_header + tcp_header);
  NS_LOG_LOGIC ("TCP ADU size is: " << tcp_adu_size);

  // Set the simulation start and stop time
  double start_time = 1.0;
  double stop_time = 20.0;

  // 2 MB of TCP buffer
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1 << 21));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1 << 21));
  Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (sack));

  Config::SetDefault ("ns3::TcpL4Protocol::RecoveryType",
                      TypeIdValue (TypeId::LookupByName (recovery)));
  // Select TCP variant
  if (transport_prot.compare ("ns3::TcpWestwoodPlus") == 0)
    { 
      // TcpWestwoodPlus is not an actual TypeId name; we need TcpWestwood here
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId ()));
      // the default protocol type in ns3::TcpWestwood is WESTWOOD
      Config::SetDefault ("ns3::TcpWestwood::ProtocolType", EnumValue (TcpWestwood::WESTWOODPLUS));
    }
  else
    {
      TypeId tcpTid;
      NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (transport_prot, &tcpTid), "TypeId " << transport_prot << " not found");
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (transport_prot)));
    }

  // Lab2 Part1 Topology
  //          100Mbps                                       100Mbps
  //          0.01ms            Bottleneck link             0.01ms
  // source ------------ temp1-------------------temp2 --------------- des1
  //         point-to-point     point-to-point     |    point-to-point
  //                                               |
  //                                               |100Mbps
  //                                               |50ms
  //                                               |
  //                                              des2
  // Create gateways, sources, and sinks
  NodeContainer gateways;
  gateways.Create (1);
  NodeContainer sources;
  sources.Create (1);
  // std::cout <<"node:"<< sourcesGet (nWifi - 1)->GetId<<std::endl;
  NodeContainer sinks;
  sinks.Create (1);

  NodeContainer sinks2;
  sinks2.Create (1);

  NodeContainer gateways2;
  gateways2.Create (1);
  
  // Configure the error model
  // Here we use RateErrorModel with packet error rate
  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (errorRate));

  PointToPointHelper UnReLink;
  UnReLink.SetDeviceAttribute ("DataRate", StringValue (dataRate));
  UnReLink.SetChannelAttribute ("Delay", StringValue (delay));
  UnReLink.SetDeviceAttribute ("ReceiveErrorModel", PointerValue (em));


  InternetStackHelper stack;
  stack.InstallAll ();


  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");

  // Configure the sources and sinks net devices
  // and the channels between the sources/sinks and the gateways
  PointToPointHelper LocalLink;
  LocalLink.SetDeviceAttribute ("DataRate", StringValue (access_bandwidth));
  LocalLink.SetChannelAttribute ("Delay", StringValue (access_delay));

  PointToPointHelper LocalLink2;
  LocalLink2.SetDeviceAttribute ("DataRate", StringValue (access_bandwidth2));
  LocalLink2.SetChannelAttribute ("Delay", StringValue (access_delay2));

  Ipv4InterfaceContainer sink_interfaces;


  NetDeviceContainer devices;
  devices = LocalLink.Install (sources.Get (0), gateways.Get (0));
  address.NewNetwork ();
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  // NetDeviceContainer devices2;
  devices = UnReLink.Install (gateways.Get (0), gateways2.Get (0));
  address.NewNetwork ();
  Ipv4InterfaceContainer interfaces2 = address.Assign (devices);

  devices = LocalLink.Install (gateways2.Get (0), sinks.Get (0));
  address.NewNetwork ();
  interfaces = address.Assign (devices);
  sink_interfaces.Add (interfaces.Get (1));

  //point to point link between temp2 and dest2
  devices = LocalLink.Install (gateways2.Get (0), sinks2.Get (0));
  address.NewNetwork ();
  interfaces = address.Assign (devices);
  sink_interfaces.Add (interfaces.Get (1));
    

  NS_LOG_INFO ("Initialize Global Routing.");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  uint16_t port = 50000;


  //nFlows
  ApplicationContainer sourceApp[nFlows];
  ApplicationContainer sinkApp[nFlows];

  for(unsigned int i = 0; i < nFlows / 2; i++){
    Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port + i));
    PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);

    AddressValue remoteAddress (InetSocketAddress (sink_interfaces.GetAddress (0, 0), port + i));
    //dest2
    AddressValue remoteAddress2 (InetSocketAddress (sink_interfaces.GetAddress (1, 0), port + i));

    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (tcp_adu_size));
    BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
    ftp.SetAttribute ("Remote", remoteAddress);
    ftp.SetAttribute ("SendSize", UintegerValue (tcp_adu_size));
    ftp.SetAttribute ("MaxBytes", UintegerValue (data_mbytes * 1000000));    

    sourceApp[i * 2] = ftp.Install (sources.Get (0));
    sourceApp[i * 2].Start (Seconds (start_time));
    sourceApp[i * 2].Stop (Seconds (stop_time));

    //dest2
    ftp.SetAttribute ("Remote", remoteAddress2);
    sourceApp[i * 2 + 1] = ftp.Install (sources.Get (0));
    sourceApp[i * 2 + 1].Start (Seconds (start_time));
    sourceApp[i * 2 + 1].Stop (Seconds (stop_time));


    sinkHelper.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
    sinkApp[i * 2]= sinkHelper.Install (sinks.Get (0));
    sinkApp[i * 2].Start (Seconds (0));
    sinkApp[i * 2].Stop (Seconds (stop_time));

    //dest2
    sinkHelper.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
    sinkApp[i * 2 + 1]= sinkHelper.Install (sinks2.Get (0));
    sinkApp[i * 2 + 1].Start (Seconds (0));
    sinkApp[i * 2 + 1].Stop (Seconds (stop_time));
  }

  AsciiTraceHelper ascii;
  // float samplingPeriod = 0.1;
  for(int i = 0; i < nFlows; i ++) {
    // Ptr<OutputStreamWrapper> GoodputStream = ascii.CreateFileStream (prefix_file_name +"_" + transport_prot +"_"+std::to_string(nFlows)+ "flow_" + std::to_string(i)+"_"  + delay + "-Goodput.txt");
      Ptr<OutputStreamWrapper> GoodputStream = ascii.CreateFileStream (transport_prot +"_"+std::to_string(nFlows)+ "_" + std::to_string(i)+ "_flow"+ "-Goodput.txt");
      Simulator::Schedule (Seconds (stop_time), &GoodputSampling, i, sinkApp[i],
                        GoodputStream);
  }

  //aggregate goodput
  Ptr<OutputStreamWrapper> AverageGoodputStream = ascii.CreateFileStream (prefix_file_name +"_" + transport_prot +"_"+ std::to_string(nFlows)+ "flow_"+ "average" + "-Goodput.txt");
  Simulator::Schedule (Seconds (stop_time), &AverageGoodput, nFlows,
                      AverageGoodputStream);
  // Set up tracing if enabled
  if (tracing)
    {
      // std::ofstream ascii;
      // Ptr<OutputStreamWrapper> ascii_wrap;
      // ascii.open ((prefix_file_name + "-ascii").c_str ());
      // ascii_wrap = new OutputStreamWrapper ((prefix_file_name + "-ascii").c_str (),
      //                                       std::ios::out);
      // stack.EnableAsciiIpv4All (ascii_wrap);

      Simulator::Schedule (Seconds (start_time + 0.000001), &TraceCwnd, prefix_file_name+"_"+ transport_prot +"_"+"0"+"_"+ std::to_string(nFlows)+ "flows_" + delay +"-cwnd.data", nFlows);
      // Simulator::Schedule (Seconds (start_time + 0.000001), &TraceSsThresh, prefix_file_name + "-ssth.data");
      // Simulator::Schedule (Seconds (start_time + 0.000001), &TraceRtt, prefix_file_name + "-rtt.data");
      // Simulator::Schedule (Seconds (start_time + 0.000001), &TraceRto, prefix_file_name + "-rto.data");
      // Simulator::Schedule (Seconds (start_time + 0.000001), &TraceNextTx, prefix_file_name + "-next-tx.data");
      // Simulator::Schedule (Seconds (start_time + 0.000001), &TraceInFlight, prefix_file_name + "-inflight.data");
      // Simulator::Schedule (Seconds (start_time + 0.000001), &TraceNextRx, prefix_file_name + "-next-rx.data");
    }

  if (pcap)
    {
      UnReLink.EnablePcapAll (prefix_file_name, true);
      LocalLink.EnablePcapAll (prefix_file_name, true);
    }

  // Flow monitor
  FlowMonitorHelper flowHelper;
  if (flow_monitor)
    {
      flowHelper.InstallAll ();
    }

  Simulator::Stop (Seconds (stop_time));
  Simulator::Run ();

  if (flow_monitor)
    {
      flowHelper.SerializeToXmlFile (prefix_file_name + ".flowmonitor", true, true);
    }

  Simulator::Destroy ();
  return 0;
}
