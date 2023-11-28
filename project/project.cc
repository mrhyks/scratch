#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/pcap-file.h" // Include the pcap file header
#include "ns3/buildings-module.h"
#include "ns3/buildings-propagation-loss-model.h"

using namespace ns3;

int main (int argc, char *argv[])
{
  // Enable LTE and set logging
  LogComponentEnable ("LteEnbRrc", LOG_LEVEL_INFO);
  LogComponentEnable ("LteUeRrc", LOG_LEVEL_INFO);

  uint16_t numNodePairs = 2;
  Time simTime = Seconds (5.0);

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue ("numNodePairs", "Number of eNodeBs:", numNodePairs);
  cmd.AddValue ("simTime", "Total duration of the simulation", simTime);
  cmd.Parse (argc, argv);

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> (); // create LteHelper object
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> (); // PointToPointEpcHelper
  lteHelper->SetEpcHelper (epcHelper); // enable the use of EPC by LTE helper

  // Create eNodeBs
  NodeContainer enbNodes;
  enbNodes.Create (numNodePairs); // number of eNodeBs defined by numNodePairs

  // Create UEs, it was only for try the model
  NodeContainer ueNodes;
  ueNodes.Create (10); // number of UEs defined by numNodePairs

  // Install Mobility Model
  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel"); // mobility model (constant)
  mobility.Install(enbNodes);

  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (10.0, 0.0, 0.0));
  positionAlloc->Add (Vector (20.0, 0.0, 0.0));
  positionAlloc->Add (Vector (30.0, 0.0, 0.0));
  positionAlloc->Add (Vector (40.0, 0.0, 0.0));
  
  // Assign positions to 5 stationary UEs
  NodeContainer stationaryUeNodes;
  //stationaryUeNodes.Add (ueNodes.Get (0));
  //stationaryUeNodes.Add (ueNodes.Get (1));
  //stationaryUeNodes.Add (ueNodes.Get (2));
  //stationaryUeNodes.Add (ueNodes.Get (3));
  //stationaryUeNodes.Add (ueNodes.Get (4));

  MobilityHelper stationaryMobility;
  stationaryMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  stationaryMobility.SetPositionAllocator (positionAlloc);
  stationaryMobility.Install (stationaryUeNodes);

  // Assign positions to 5 walking UEs
  NodeContainer walkingUeNodes;
  walkingUeNodes.Add (ueNodes.Get (5));
  walkingUeNodes.Add (ueNodes.Get (6));
  walkingUeNodes.Add (ueNodes.Get (7));
  walkingUeNodes.Add (ueNodes.Get (8));
  walkingUeNodes.Add (ueNodes.Get (9));
  
  MobilityHelper walkingMobility;
  walkingMobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                                   "Bounds", RectangleValue (Rectangle (-500, 500, -500, 500)));
  walkingMobility.SetPositionAllocator (positionAlloc);
  walkingMobility.Install (walkingUeNodes);

  ueNodes.Add (stationaryUeNodes);
  ueNodes.Add (walkingUeNodes);

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes); // add eNB nodes to the container
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes); // add UE nodes to the container

  // Install the IP stack on the UEs
  InternetStackHelper stack;
  stack.Install(ueNodes);

  // Assign IP addresses to UE nodes
  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.0.1.0", "255.255.255.0"); // Set the network address and subnet mask
  Ipv4InterfaceContainer ueInterfaces = ipv4.Assign(ueLteDevs); // Assign IP addresses to UE devices

  // Attach one UE per eNodeB
  for (uint16_t i = 0; i < numNodePairs; i++)
  {
    lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(i)); // attach UE nodes to eNB nodes
    // side effect: the default EPS bearer will be activated
  }

  // Create a remote host on the internet with IP address 1.2.3.4
  NodeContainer remoteHost;
  remoteHost.Create(1);
  Ptr<Node> remoteNode = remoteHost.Get(0);
  InternetStackHelper internet;
  internet.Install(remoteNode);
  Ipv4Address remoteIp("1.2.3.4");
  Ptr<Ipv4StaticRouting> remoteStaticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting> (remoteNode->GetObject<Ipv4> ()->GetRoutingProtocol ());
  remoteStaticRouting->AddHostRouteTo(remoteIp, 1);

  // Create a point-to-point link between the remote host and the eNodeB
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
  p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
  p2ph.SetChannelAttribute("Delay", StringValue("10ms"));
  NetDeviceContainer internetDevices = p2ph.Install(remoteNode, enbNodes.Get(0));

  //Creating a Building
  Ptr<Building> building = Create<Building>();
  building->SetBoundaries(Box(0.0, 50.0, 0.0, 50.0, 0.0, 20.0)); // Set the building boundaries (x, y, z)


  Ptr<BuildingsPropagationLossModel> buildingLossModel = CreateObject<BuildingsPropagationLossModel>();
  buildingLossModel->SetPathLossModel("ns3::LogDistancePropagationLossModel");
  buildingLossModel->SetBuilding(building);

  // Set the propagation loss model for nodes inside the building
  for (uint32_t i = 0; i < ueNodes.GetN(); ++i) {
    Ptr<MobilityModel> mobility = ueNodes.Get(i)->GetObject<MobilityModel>();
    mobility->AggregateObject(buildingLossModel);
  }

  Ptr<ListPositionAllocator> positionAllocInBuilding = CreateObject<ListPositionAllocator>();
  positionAllocInBuilding->Add(Vector(5.0, 5.0, 5.0));
  positionAllocInBuilding->Add(Vector(5.0, 10.0, 5.0)); 
  positionAllocInBuilding->Add(Vector(12.0, 20.0, 5.0)); 
  positionAllocInBuilding->Add(Vector(25.0, 30.0, 5.0)); 
  positionAllocInBuilding->Add(Vector(30.0, 45.0, 5.0)); 
  // Add positions for UEs inside the building

  // Assign these positions to UEs within the building
  MobilityHelper mobilityInBuilding;
  mobilityInBuilding.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobilityInBuilding.SetPositionAllocator(positionAllocInBuilding);
  mobilityInBuilding.Install(ueNodes.Get(0, 4)); // Assuming UEs 0-4 are inside the building
  

  // Run simulation
  Simulator::Stop (simTime);
  p2ph.EnablePcapAll("project");
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
