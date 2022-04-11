/* 
Lab3 Part1
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

NS_LOG_COMPONENT_DEFINE ("Lab3 part1");


void
GetPosition (NodeContainer nodes, Ptr<OutputStreamWrapper> stream)
{
  double sum = 0;
  for(int i = 0; i < nodes.GetN(); i++) {
    Ptr<MobilityModel> model = nodes.Get(i)->GetObject<MobilityModel>();
    Vector position = model->GetPosition ();
    double distance = sqrt(position.x * position.x + position.y * position.y);
    sum += distance;
    *stream->GetStream () << position.x <<" " << position.y << std::endl;
    NS_LOG_UNCOND ("wifi station node " << i<<" position at" <<
    " x = " << position.x << ", y = " << position.y << ", dist = " << distance);
  }
  double avg_dis = sum / nodes.GetN();;
  std::cout<<"Average distance = " + std::to_string(avg_dis) << std::endl;
}

int 
main (int argc, char *argv[])
{
  uint32_t numNodes = 100;
  std::string mobilitymode = "walk";
  uint32_t duration = 400;
  uint32_t minSpeed = 4;
  uint32_t maxSpeed = 6;
  uint32_t pause = 2;
  CommandLine cmd (__FILE__);
  cmd.AddValue ("numNodes", "Number of nodes", numNodes);
  cmd.AddValue ("mobility", "mobility mode", mobilitymode);
  cmd.AddValue ("duration", "simulation duration", duration);
  cmd.AddValue ("minSpeed", "minSpeed", minSpeed);
  cmd.AddValue ("maxSpeed", "maxSpeed", maxSpeed);
  cmd.AddValue ("pause", "pause", pause);
  cmd.Parse (argc,argv);

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (numNodes);

  MobilityHelper mobility;
  ObjectFactory pos;
  pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
  pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=-40.0|Max=40.0]"));
  pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=-40.0|Max=40.0]"));

  Ptr <PositionAllocator> taPositionAlloc = pos.Create ()->GetObject <PositionAllocator> ();


  if(mobilitymode == "walk") {
    mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                            "Bounds", RectangleValue (Rectangle (-40, 40, -40, 40)), "Time", TimeValue (Seconds (2.0)));
  }

  if(mobilitymode == "waypoint") {
    std::ostringstream speedUniformRandomVariableStream;
    speedUniformRandomVariableStream << "ns3::UniformRandomVariable[Min="
                                    <<minSpeed
                                    <<"|"
                                    <<"Max="
                                    <<maxSpeed
                                    << "]";
    std::ostringstream pauseUniformRandomVariableStream;
    pauseUniformRandomVariableStream << "ns3::ConstantRandomVariable[Constant="
                                    <<pause
                                    <<"]";
    mobility.SetMobilityModel ("ns3::RandomWaypointMobilityModel", "Speed", StringValue (speedUniformRandomVariableStream.str ()),
                              "Pause", StringValue (pauseUniformRandomVariableStream.str()), "PositionAllocator", PointerValue (taPositionAlloc));
  }
  mobility.SetPositionAllocator (taPositionAlloc);
  mobility.Install (wifiStaNodes);

  //output position
  AsciiTraceHelper ascii;
  Ptr<OutputStreamWrapper> PositionStream = ascii.CreateFileStream (mobilitymode + "-position.data");
  Simulator::Schedule(Seconds(duration), &GetPosition, wifiStaNodes, PositionStream);
  Simulator::Stop (Seconds (duration));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
