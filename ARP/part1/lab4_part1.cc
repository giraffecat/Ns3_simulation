/*
Lab4 Part1
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

#include"math.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("lab4_part1");

//global varibales
uint128_t prePackets = 0;

AsciiTraceHelper ascii;
Ptr<OutputStreamWrapper> Stream = ascii.CreateFileStream ("lab4_part1_output.txt");

static void GoodputSampling (ApplicationContainer app, NodeContainer nodes, int time, Ptr<OutputStreamWrapper> stream)
{
  double goodput = 0;
  uint128_t totalPackets = DynamicCast<PacketSink> (app.Get (0))->GetTotalRx ();
  uint128_t currentPackets = totalPackets - prePackets;
  goodput = (currentPackets * 8 / time); // bit/s
  Ptr<MobilityModel> model = nodes.Get(0)->GetObject<MobilityModel>();
  Vector position = model->GetPosition ();
  double dis = pow(position.x * position.x + position.y * position.y + (position.z - 3) * (position.z - 3), 1.0 / 2);
  std::cout <<Simulator::Now().GetSeconds() 
  <<"  (" << position.x << ", " << position.y <<", " << position.z << ")"
  <<"  "<<dis
  <<"  "<<goodput<< " bits/s" <<std::endl;
  *stream->GetStream () <<Simulator::Now().GetSeconds() 
  <<"  (" << position.x << ", " << position.y <<", " << position.z << ")"
  <<"  "<<dis
  <<"  "<<goodput<< " bits/s" <<std::endl;
  prePackets = totalPackets;
}

static void ChangePosition (NodeContainer nodes, int x, int y, int z) {
  Ptr<MobilityModel> model = nodes.Get(0)->GetObject<MobilityModel>();
  Vector position = model->GetPosition ();
  position.x = x;
  position.y = y;
  position.z = z;
  model->SetPosition(position);
}

void
RateChange (uint64_t oldVal, uint64_t newVal)
{
  *Stream->GetStream () <<Simulator::Now().GetSeconds()  <<" Rate change from " << oldVal << " to " << newVal <<std::endl;
  std::cout << Simulator::Now().GetSeconds()  <<" Rate change from " << oldVal << " to " << newVal <<std::endl;
}

int main (int argc, char *argv[])
{
  //init params
  uint32_t Interval = 1;
  uint32_t duration = 13;

  LogComponentEnable("lab4_part1", LOG_LEVEL_INFO);

  CommandLine cmd (__FILE__);

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
  position.x = 0;
  position.y = 0;
  position.z = 3;
  mob->SetPosition(position); 
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
  position.x = 10;
  position.y = 0;
  position.z = 1;
  Stamob->SetPosition(position); 

  OnOffHelper onoffAp ("ns3::UdpSocketFactory",
  Address (InetSocketAddress (wifiInterfaces.GetAddress (0), 9)));
  onoffAp.SetConstantRate (DataRate ("400Mb/s"), 1024);
  onoffAp.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]")); 
  onoffAp.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));

  ApplicationContainer appA1 = onoffAp.Install (wifiStaNodes.Get(0));
  appA1.Start (Seconds (0));
  appA1.Stop (Seconds (duration));

  float offset = 0.5;
  float time = 1.0;

  std::string wifiManager ("MinstrelHtWifiManager");

  Config::ConnectWithoutContext ("/NodeList/0/DeviceList/*/$ns3::WifiNetDevice/RemoteStationManager/$ns3::" + wifiManager + "/Rate", MakeCallback (&RateChange));

  //移动位置并且记录goodput 采用迭代的方式吧
  std::cout << "Time  " <<"Position(x, y, z) " <<"Dist. " << "Goodput" << std::endl;
  *Stream->GetStream ()<< "Time  " <<"Position(x, y, z) " <<"Dist. " << "Goodput" << std::endl;

  Simulator::Schedule (Seconds (offset + Interval), &GoodputSampling, sinkApp, wifiStaNodes.Get(0), time + offset, Stream);
  
  //position 15, 0
  Simulator::Schedule (Seconds (offset + Interval), &ChangePosition, wifiStaNodes.Get(0), 15, 0, 1);
  Simulator::Schedule (Seconds (offset + Interval * 2), &GoodputSampling, sinkApp, wifiStaNodes.Get(0), time, Stream);

  //position 20, 0
  Simulator::Schedule (Seconds (offset + Interval * 2), &ChangePosition, wifiStaNodes.Get(0), 20, 0, time);
  Simulator::Schedule (Seconds (offset + Interval * 3), &GoodputSampling, sinkApp, wifiStaNodes.Get(0), time, Stream);

  //position 25, 0
  Simulator::Schedule (Seconds (offset + Interval * 3), &ChangePosition, wifiStaNodes.Get(0), 25, 0, 1);
  Simulator::Schedule (Seconds (offset + Interval * 4), &GoodputSampling, sinkApp, wifiStaNodes.Get(0), time, Stream);

  //position 30, 0
  Simulator::Schedule (Seconds (offset + Interval * 4), &ChangePosition, wifiStaNodes.Get(0), 30, 0, 1);
  Simulator::Schedule (Seconds (offset + Interval * 5), &GoodputSampling, sinkApp, wifiStaNodes.Get(0), time, Stream);

  //position 35, 0
  Simulator::Schedule (Seconds (offset + Interval * 5), &ChangePosition, wifiStaNodes.Get(0), 35, 0, 1);
  Simulator::Schedule (Seconds (offset + Interval * 6), &GoodputSampling, sinkApp, wifiStaNodes.Get(0), time, Stream);

  //position 35, 5
  Simulator::Schedule (Seconds (offset + Interval * 6), &ChangePosition, wifiStaNodes.Get(0), 35, 5, 1);
  Simulator::Schedule (Seconds (offset + Interval * 7), &GoodputSampling, sinkApp, wifiStaNodes.Get(0), time, Stream);

  //position 35, 10
  Simulator::Schedule (Seconds (offset + Interval * 7), &ChangePosition, wifiStaNodes.Get(0), 35, 10, 1);
  Simulator::Schedule (Seconds (offset + Interval * 8), &GoodputSampling, sinkApp, wifiStaNodes.Get(0), time, Stream);

  //position 40, 10
  Simulator::Schedule (Seconds (offset + Interval * 8), &ChangePosition, wifiStaNodes.Get(0), 40, 10, 1);
  Simulator::Schedule (Seconds (offset + Interval * 9), &GoodputSampling, sinkApp, wifiStaNodes.Get(0), time, Stream);

  //position 45, 10
  Simulator::Schedule (Seconds (offset + Interval * 9), &ChangePosition, wifiStaNodes.Get(0), 45, 10, 1);
  Simulator::Schedule (Seconds (offset + Interval * 10), &GoodputSampling, sinkApp, wifiStaNodes.Get(0), time, Stream);

  //position 50, 10
  Simulator::Schedule (Seconds (offset + Interval * 10), &ChangePosition, wifiStaNodes.Get(0), 50, 10, 1);
  Simulator::Schedule (Seconds (offset + Interval * 11), &GoodputSampling, sinkApp, wifiStaNodes.Get(0), time, Stream);


  //position 55, 10
  Simulator::Schedule (Seconds (offset + Interval * 11), &ChangePosition, wifiStaNodes.Get(0), 55, 10, 1);
  Simulator::Schedule (Seconds (offset + Interval * 12), &GoodputSampling, sinkApp, wifiStaNodes.Get(0), time, Stream);

  Simulator::Stop (Seconds (duration));
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
