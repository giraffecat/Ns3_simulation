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
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/ptr.h"
#include "ns3/double.h"
#include "ns3/rng-seed-manager.h"

// Default Network Topology
//
//       10.1.1.0          10.1.2.0
// n1 -------------- n0 --------------- n2
//    point-to-point     point-to-point
//
 
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Lab1_Part1");


int main (int argc, char *argv[])
{
  uint32_t nClient = 1;
  uint32_t nPacket = 1;
  CommandLine cmd (__FILE__);

  cmd.AddValue ("nClient", "The number of clients", nClient);
  cmd.AddValue ("nPacket", "Number of Packets that nodes send", nPacket);

  cmd.Parse (argc, argv);

  //Check whether the input parameters are correct
  if(nClient <= 0 || nClient > 5 || nPacket <= 0 || nPacket > 5) {
    std::cout << "Please enter the correct parameters (maximum of 5 clients and 5 packets per client)" <<std::endl;
    return 0;
  }
  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  // random number seed
  RngSeedManager::SetSeed (1234); 
  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
        x->SetAttribute ("Min", DoubleValue (2.0));
        x->SetAttribute ("Max", DoubleValue (7.0));

  NodeContainer nodes;
  nodes.Create (nClient + 1);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;

  for(int i = 1; i <= nClient; i++) {
    devices.Add(pointToPoint.Install(nodes.Get(i), nodes.Get(0)));
  }

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces;
  for(int i = 0; i < devices.GetN(); i+=2) {
    interfaces.Add(address.Assign (devices.Get(i)));
    interfaces.Add(address.Assign (devices.Get(i+1)));
    address.NewNetwork();
  }

  UdpEchoServerHelper echoServer (9);
  //Change Server port to 15
  echoServer.SetAttribute("Port",UintegerValue(15));

  ApplicationContainer serverApps = echoServer.Install (nodes.Get(0));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (20.0));

  UdpEchoClientHelper echoClient (interfaces.GetAddress(1), 15);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (nPacket));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  for(int i = 1; i < nClient + 1; i++) {
    ApplicationContainer clientApps = echoClient.Install (nodes.Get(i));
    clientApps.Start (Seconds (x->GetValue ()));
    clientApps.Stop (Seconds (20.0));
  }

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
