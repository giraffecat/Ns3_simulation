/*
Lab3 Part2
*/

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Lab3_part2");

static void GoodputSampling (ApplicationContainer app, NodeContainer nodes, int duration)
{
  double goodput = 0;
  uint128_t totalPackets = DynamicCast<PacketSink> (app.Get (0))->GetTotalRx ();
  goodput = (totalPackets * 8 / duration); // bit/s
  Ptr<MobilityModel> model = nodes.Get(0)->GetObject<MobilityModel>();
  Vector position = model->GetPosition ();
  std::cout << "wifi station node position at" 
  <<" x = " << position.x << ", y = " << position.y 
  <<" Goodput: " <<goodput<< " bits/s" <<std::endl;
}

int main (int argc, char *argv[])
{
  //init params
  int stationDist = 5;
  uint32_t duration = 10;

  CommandLine cmd (__FILE__);

  cmd.AddValue ("stationDist", "The distance between the Wifi station and the fixed access point", stationDist);
  cmd.Parse (argc,argv);

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (1);

  NodeContainer wifiApNode;
  wifiApNode.Create(1);

  //wifi default
  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::MinstrelHtWifiManager"); 
  wifi.SetStandard (WIFI_STANDARD_80211ac);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy;
  phy.SetChannel (channel.Create ());

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  MobilityHelper mobility;

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiStaNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);

  //AP location
  Ptr<MobilityModel> mob = wifiApNode.Get(0)->GetObject<MobilityModel>();
  Vector position = mob->GetPosition();
  position.x = -40;
  position.y = 0;
  mob->SetPosition(position); 
  std::cout << "wifi access point position at" <<" x = " << position.x << ", y = " << position.y << std::endl;
  InternetStackHelper stack;
  stack.Install (wifiStaNodes);
  stack.Install (wifiApNode);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer wifiInterfaces;
  wifiInterfaces.Add(address.Assign (apDevices));
  wifiInterfaces.Add(address.Assign (staDevices));

  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), 9));
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", sinkLocalAddress);
  ApplicationContainer sinkApp;
  sinkHelper.SetAttribute ("Protocol", TypeIdValue (UdpSocketFactory::GetTypeId ()));
  sinkApp = sinkHelper.Install (wifiApNode.Get (0));
  sinkApp.Start (Seconds (0));
  sinkApp.Stop (Seconds (duration));

  //changes sta node position 
  Ptr<MobilityModel> Stamob = wifiStaNodes.Get(0)->GetObject<MobilityModel>();
  position = Stamob->GetPosition();
  position.x = -40 + stationDist;
  position.y = 0;
  Stamob->SetPosition(position); 

  OnOffHelper onoffAp ("ns3::UdpSocketFactory",
  Address (InetSocketAddress (wifiInterfaces.GetAddress (0), 9)));
  onoffAp.SetConstantRate (DataRate ("400Mb/s"), 1024);
  onoffAp.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]")); 
  onoffAp.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));

  ApplicationContainer appA1 = onoffAp.Install (wifiStaNodes.Get(0));
  appA1.Start (Seconds (0));
  appA1.Stop (Seconds (duration));

  Simulator::Schedule (Seconds (duration), &GoodputSampling, sinkApp, wifiStaNodes.Get(0), duration);
  Simulator::Stop (Seconds (duration));
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
