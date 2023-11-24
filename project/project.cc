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
  ueNodes.Create (numNodePairs); // number of UEs defined by numNodePairs

  // Install Mobility Model
  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel"); // mobility model (constant)
  mobility.Install(enbNodes);
  mobility.Install(ueNodes);

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

  

  // Run simulation
  Simulator::Stop (simTime);
  p2ph.EnablePcapAll("project");
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
