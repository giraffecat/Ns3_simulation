/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"

// Default Network Topology
//
//       10.1.1.0                         10.1.3.0
// n0 -------------- n1   n2   n3   n4 -------------- n5
//    point-to-point  |    |    |    |   point-to-point
//                    ================
//                      LAN 10.1.2.0


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SecondScriptExample");

int 
main (int argc, char *argv[])
{
  bool verbose = true;
  uint32_t nCsma = 1;
  uint32_t nPacket = 1;
  CommandLine cmd (__FILE__);
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("nPacket", "Number of Packets that nodes send", nPacket);

  cmd.Parse (argc,argv);

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

  //Check whether the input parameters are correct
  if(nPacket <= 0 || nPacket > 20) {
    std::cout << "Please enter the correct parameters (maximum of 20 packets per client)" <<std::endl;
    return 0;
  }

  // random number seed
  RngSeedManager::SetSeed (1234); 

  //Set 0ms allowed to wait before sending an ARP request.
  Config::SetDefault ("ns3::ArpL3Protocol::RequestJitter", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=0.0]"));

  NodeContainer p2pNodes;
  p2pNodes.Create (4);

  NodeContainer csmaNodes;
  csmaNodes.Add (p2pNodes.Get (1));
  //the offset of csma 
  csmaNodes.Create (nCsma - 1);
  csmaNodes.Add (p2pNodes.Get (2));

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices.Add(pointToPoint.Install (p2pNodes.Get(0), p2pNodes.Get(1)));
  p2pDevices.Add(pointToPoint.Install (p2pNodes.Get(2), p2pNodes.Get(3)));

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  InternetStackHelper stack;
  stack.Install (p2pNodes.Get (0));
  stack.Install (p2pNodes.Get (3));
  stack.Install (csmaNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces.Add(address.Assign (p2pDevices.Get(0)));
  p2pInterfaces.Add(address.Assign (p2pDevices.Get(1)));


  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  address.NewNetwork();
  p2pInterfaces.Add(address.Assign (p2pDevices.Get(2)));
  p2pInterfaces.Add(address.Assign (p2pDevices.Get(3)));
  
  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (p2pNodes.Get (3));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (30.0));

  UdpEchoClientHelper echoClient (p2pInterfaces.GetAddress (3), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (nPacket));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (p2pNodes.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (30.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  pointToPoint.EnablePcapAll ("Lab1_part2");
  csma.EnablePcapAll ("Lab1_part2");

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
