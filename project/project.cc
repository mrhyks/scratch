// #include "project.h"

#include "functions.cc"

#include "ns3/applications-module.h"
#include "ns3/buildings-module.h"
#include "ns3/buildings-propagation-loss-model.h"
#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/pcap-file.h" // Include the pcap file header
#include "ns3/point-to-point-module.h"
#include "ns3/propagation-loss-model.h"

using namespace ns3;

int
main(int argc, char* argv[])
{
    // Enable LTE and set logging
    LogComponentEnable("LteEnbRrc", LOG_LEVEL_INFO);
    LogComponentEnable("LteUeRrc", LOG_LEVEL_INFO);

    uint16_t numNodePairs = 2;
    Time simTime = Seconds(5.0);
    uint16_t webSocketServer = 1100;
    uint16_t webSocketClient = 2000;
    Time interPacketInterval = MilliSeconds (200);

    // Command line arguments
    CommandLine cmd;
    cmd.AddValue("numNodePairs", "Number of eNodeBs:", numNodePairs);
    cmd.AddValue("simTime", "Total duration of the simulation", simTime);
    cmd.AddValue("webSocketPort", "Web socket port (for web anim)", webSocketServer);
    cmd.AddValue("webSocketClient", "Web socket client (for web anim)", webSocketClient);
    cmd.Parse(argc, argv);

    // parse again so you can override default values from the command line
    cmd.Parse(argc, argv);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>(); // create LteHelper object
    Ptr<PointToPointEpcHelper> epcHelper =
        CreateObject<PointToPointEpcHelper>(); // PointToPointEpcHelper
    lteHelper->SetEpcHelper(epcHelper);        // enable the use of EPC by LTE helper

    // Create eNodeBs
    NodeContainer enbNodes;
    enbNodes.Create(numNodePairs); // number of eNodeBs defined by numNodePairs

    // Create UEs, it was only for try the model
    NodeContainer ueNodes;
    ueNodes.Create(10); // number of UEs defined by numNodePairs

    // Install Mobility Model
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel"); // mobility model (constant)
    mobility.Install(enbNodes);

    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(10.0, 0.0, 0.0));
    positionAlloc->Add(Vector(20.0, 0.0, 0.0));
    positionAlloc->Add(Vector(30.0, 0.0, 0.0));
    positionAlloc->Add(Vector(40.0, 0.0, 0.0));

    // Assign positions to 5 stationary UEs
    NodeContainer stationaryUeNodes;
    stationaryUeNodes.Add(ueNodes.Get(0));
    stationaryUeNodes.Add(ueNodes.Get(1));
    stationaryUeNodes.Add(ueNodes.Get(2));
    stationaryUeNodes.Add(ueNodes.Get(3));
    stationaryUeNodes.Add(ueNodes.Get(4));

    MobilityHelper stationaryMobility;
    stationaryMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    stationaryMobility.SetPositionAllocator(positionAlloc);
    stationaryMobility.Install(stationaryUeNodes);

    // Assign positions to 5 walking UEs
    NodeContainer walkingUeNodes;
    walkingUeNodes.Add(ueNodes.Get(5));
    walkingUeNodes.Add(ueNodes.Get(6));
    walkingUeNodes.Add(ueNodes.Get(7));
    walkingUeNodes.Add(ueNodes.Get(8));
    walkingUeNodes.Add(ueNodes.Get(9));

    MobilityHelper walkingMobility;
    walkingMobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                                     "Bounds",
                                     RectangleValue(Rectangle(-500, 500, -500, 500)));
    walkingMobility.SetPositionAllocator(positionAlloc);
    walkingMobility.Install(walkingUeNodes);

    ueNodes.Add(stationaryUeNodes);
    ueNodes.Add(walkingUeNodes);

    // Install LTE Devices to the nodes
    NetDeviceContainer enbLteDevs =
        lteHelper->InstallEnbDevice(enbNodes); // add eNB nodes to the container
    NetDeviceContainer ueLteDevs =
        lteHelper->InstallUeDevice(ueNodes); // add UE nodes to the container

    // Creating a Building
    Ptr<Building> building = Create<Building>();
    building->SetBoundaries(
        Box(0.0, 50.0, 0.0, 50.0, 0.0, 20.0)); // Set the building boundaries (x, y, z)
    Ptr<Node> pgw = epcHelper->GetPgwNode();   // get the PGW node

    // Create a single RemoteHost
    NodeContainer remoteHostContainer; // container for remote node
    remoteHostContainer.Create(1);     // create 1 node
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer); // aggregate stack implementations (ipv4, ipv6, udp, tcp)
                                           // to remote node

    // Create the Internet
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500)); // p2p mtu
    p2ph.SetChannelAttribute("Delay", TimeValue(MilliSeconds(10)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces =
        ipv4h.Assign(internetDevices); // assign IPs to PGW and remote host
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"),
                                               Ipv4Mask("255.0.0.0"),
                                               1); // add route

    // Install the IP stack on the UEs
    internet.Install(ueNodes);
    // Assign IP address to UEs, set static route
    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));

    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        Ptr<Node> ueNode = ueNodes.Get(u);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(),
                                         1); // default route
    }

    ApplicationContainer clientApps;
    ApplicationContainer serverApps;
    // Attach one UE per eNodeB
    for (uint16_t i = 0; i < numNodePairs; i++)
    {
        lteHelper->Attach(ueLteDevs.Get(i), enbLteDevs.Get(i)); // attach UE nodes to eNB nodes
        // side effect: the default EPS bearer will be activated
    }

    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        // Install and start applications on UEs and remote host
        PacketSinkHelper dlPacketSinkHelper("ns3::UdpSocketFactory",
                                            InetSocketAddress(Ipv4Address::GetAny(), webSocketServer));
        serverApps.Add(dlPacketSinkHelper.Install(ueNodes.Get(u)));

        UdpClientHelper dlClient(ueIpIface.GetAddress(u), webSocketServer); // udp client
        dlClient.SetAttribute("Interval", TimeValue(interPacketInterval));
        dlClient.SetAttribute("MaxPackets", UintegerValue(1000000));
        clientApps.Add(dlClient.Install(remoteHost));

        ++webSocketClient;
        PacketSinkHelper ulPacketSinkHelper("ns3::UdpSocketFactory",
                                            InetSocketAddress(Ipv4Address::GetAny(), webSocketClient));
        serverApps.Add(ulPacketSinkHelper.Install(remoteHost));

        UdpClientHelper ulClient(remoteHostAddr, webSocketClient); // udp client
        ulClient.SetAttribute("Interval", TimeValue(interPacketInterval));
        ulClient.SetAttribute("MaxPackets", UintegerValue(1000000));
        clientApps.Add(ulClient.Install(ueNodes.Get(u)));
    }
// Run simulation
Simulator::Stop(simTime);
p2ph.EnablePcapAll("project");
Simulator::Run();
Simulator::Destroy();

return 0;
}



// Ptr<BuildingsPropagationLossModel> buildingLossModel = CreateObject<BuildingsPropagationLossModel>();
// buildingLossModel->SetPathLossModel("ns3::LogDistancePropagationLossModel");
// buildingLossModel->SetBuilding(building);

// // Set the propagation loss model for nodes inside the building
// for (uint32_t i = 0; i < ueNodes.GetN(); ++i) {
//   Ptr<MobilityModel> mobility = ueNodes.Get(i)->GetObject<MobilityModel>();
//   mobility->AggregateObject(buildingLossModel);
// }

// Ptr<ListPositionAllocator> positionAllocInBuilding = CreateObject<ListPositionAllocator>();
// positionAllocInBuilding->Add(Vector(5.0, 5.0, 5.0));
// positionAllocInBuilding->Add(Vector(5.0, 10.0, 5.0));
// positionAllocInBuilding->Add(Vector(12.0, 20.0, 5.0));
// positionAllocInBuilding->Add(Vector(25.0, 30.0, 5.0));
// positionAllocInBuilding->Add(Vector(30.0, 45.0, 5.0));
// // Add positions for UEs inside the building

// // Assign these positions to UEs within the building
// MobilityHelper mobilityInBuilding;
// mobilityInBuilding.SetMobilityModel("ns3::ConstantPositionMobilityModel");
// mobilityInBuilding.SetPositionAllocator(positionAllocInBuilding);
// mobilityInBuilding.Install(ueNodes.Get(0, 4)); // Assuming UEs 0-4 are inside the building

// // Create a UDP echo application on each UE node
// uint16_t echoPort = 9; // Echo port number
// UdpEchoServerHelper echoServer(echoPort);
// ApplicationContainer serverApps = echoServer.Install(ueNodes);
// serverApps.Start(Seconds(0.0));
// serverApps.Stop(simTime);

// // Create a UDP client application on the remote host
// uint16_t clientPort = 12345; // Client port number
// UdpEchoClientHelper echoClient(ueInterfaces.GetAddress(0), echoPort);
// echoClient.SetAttribute("MaxPackets", UintegerValue(10));
// echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
// echoClient.SetAttribute("PacketSize", UintegerValue(1024));
// ApplicationContainer clientApps = echoClient.Install(remoteNode);
// clientApps.Start(Seconds(0.0));
// clientApps.Stop(simTime);