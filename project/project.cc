// #include "project.h"

#include "functions.cc"

#include "ns3/applications-module.h"
#include "ns3/buildings-module.h"
#include "ns3/buildings-propagation-loss-model.h"
#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include "ns3/data-rate.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/gnuplot.h"
#include "ns3/internet-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/pcap-file.h" // Include the pcap file header
#include "ns3/point-to-point-module.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/traffic-control-module.h"
#include <random>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("project");

void
ServerConnectionEstablished(Ptr<const ThreeGppHttpServer>, Ptr<Socket>)
{
    NS_LOG_INFO("Client has established a connection to the server.");
}

void
MainObjectGenerated(uint32_t size)
{
    NS_LOG_INFO("Server generated a main object of " << size << " bytes.");
}

void
EmbeddedObjectGenerated(uint32_t size)
{
    NS_LOG_INFO("Server generated an embedded object of " << size << " bytes.");
}

void
ServerTx(Ptr<const Packet> packet)
{
    NS_LOG_INFO("Server sent a packet of " << packet->GetSize() << " bytes.");
}

void
ClientRx(Ptr<const Packet> packet, const Address& address)
{
    NS_LOG_INFO("Client received a packet of " << packet->GetSize() << " bytes from " << address);
}

void
ClientMainObjectReceived(Ptr<const ThreeGppHttpClient>, Ptr<const Packet> packet)
{
    Ptr<Packet> p = packet->Copy();
    ThreeGppHttpHeader header;
    p->RemoveHeader(header);
    if (header.GetContentLength() == p->GetSize() &&
        header.GetContentType() == ThreeGppHttpHeader::MAIN_OBJECT)
    {
        NS_LOG_INFO("Client has successfully received a main object of " << p->GetSize()
                                                                         << " bytes.");
    }
    else
    {
        NS_LOG_INFO("Client failed to parse a main object. ");
    }
}

void
ClientEmbeddedObjectReceived(Ptr<const ThreeGppHttpClient>, Ptr<const Packet> packet)
{
    Ptr<Packet> p = packet->Copy();
    ThreeGppHttpHeader header;
    p->RemoveHeader(header);
    if (header.GetContentLength() == p->GetSize() &&
        header.GetContentType() == ThreeGppHttpHeader::EMBEDDED_OBJECT)
    {
        NS_LOG_INFO("Client has successfully received an embedded object of " << p->GetSize()
                                                                              << " bytes.");
    }
    else
    {
        NS_LOG_INFO("Client failed to parse an embedded object. ");
    }
}
int getRandomNumber(int min, int max) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(min, max);
        return dis(gen);
    }

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
    Time interPacketInterval = MilliSeconds(200);
    uint16_t numberOfUes = 10;

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
    ueNodes.Create(numberOfUes); // number of UEs defined by numNodePairs
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    for (int i = 0; i < 20; i++) {
        positionAlloc->Add(Vector(getRandomNumber(0, 500), getRandomNumber(0, 500), 0.0));
    }
    // Install Mobility Model
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel"); // mobility model (constant)
    mobility.SetPositionAllocator(positionAlloc); ///
    mobility.Install(enbNodes);

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

    
    Ptr<Node> pgw = epcHelper->GetPgwNode(); // get the PGW node

    // Install Mobility Model for PGW
    MobilityHelper pgwMobility;
    pgwMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel"); // mobility model (constant)
    pgwMobility.SetPositionAllocator(positionAlloc);
    pgwMobility.Install(pgw);

    // Create a single RemoteHost
    NodeContainer remoteHostContainer; // container for remote node
    remoteHostContainer.Create(1);     // create 1 node
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer); // aggregate stack implementations (ipv4, ipv6, udp, tcp)
                                           // to remote node

    MobilityHelper remoteHostMobility;
    remoteHostMobility.SetMobilityModel(
        "ns3::ConstantPositionMobilityModel"); // mobility model (constant)
    remoteHostMobility.SetPositionAllocator(positionAlloc);
    remoteHostMobility.Install(remoteHost);

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

    // Install LTE Devices to the nodes
    NetDeviceContainer enbLteDevs =
        lteHelper->InstallEnbDevice(enbNodes); // add eNB nodes to the container
    NetDeviceContainer ueLteDevs =
        lteHelper->InstallUeDevice(ueNodes); // add UE nodes to the container


    /// Install the IP stack on the UEs
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

    // // Attach one UE per eNodeB
    // for (uint16_t i = 0; i < numberOfUes; i++)
    // {
    //     if (i % 2 == 0)
    //     {
    //         lteHelper->Attach(ueLteDevs.Get(i),
    //                           enbLteDevs.Get(0)); // attach UE nodes to the 1st eNB node
    //     }
    //     else
    //     {
    //         lteHelper->Attach(ueLteDevs.Get(i),
    //                           enbLteDevs.Get(1)); // attach UE nodes to the 2nd eNB node
    //     }
    //     // side effect: the default EPS bearer will be activated
    // }
    // Attach UEs to eNodeBs
    lteHelper->Attach(ueLteDevs);
    // Create HTTP server helper
    ThreeGppHttpServerHelper serverHelper(remoteHostAddr);

    // Install HTTP server
    ApplicationContainer serverApps = serverHelper.Install(remoteHostContainer.Get(0));
    Ptr<ThreeGppHttpServer> httpServer = serverApps.Get(0)->GetObject<ThreeGppHttpServer>();

    // Example of connecting to the trace sources
    httpServer->TraceConnectWithoutContext("ConnectionEstablished",
                                           MakeCallback(&ServerConnectionEstablished));
    httpServer->TraceConnectWithoutContext("MainObject", MakeCallback(&MainObjectGenerated));
    httpServer->TraceConnectWithoutContext("EmbeddedObject",
                                           MakeCallback(&EmbeddedObjectGenerated));
    httpServer->TraceConnectWithoutContext("Tx", MakeCallback(&ServerTx));

    // Setup HTTP variables for the server
    PointerValue varPtr;
    httpServer->GetAttribute("Variables", varPtr);
    Ptr<ThreeGppHttpVariables> httpVariables = varPtr.Get<ThreeGppHttpVariables>();
    httpVariables->SetMainObjectSizeMean(102400);  // 100kB
    httpVariables->SetMainObjectSizeStdDev(40960); // 40kB

    // Create HTTP client helper
    ThreeGppHttpClientHelper clientHelper(remoteHostAddr);
    ApplicationContainer clientApps;
    // Install HTTP client
    // Install HTTP client
    // ApplicationContainer clientApps = clientHelper.Install (ueNodes.Get (0));
    // Ptr<ThreeGppHttpClient> httpClient = clientApps.Get (0)->GetObject<ThreeGppHttpClient> ();

    // // Example of connecting to the trace sources
    // httpClient->TraceConnectWithoutContext ("RxMainObject", MakeCallback (&ClientMainObjectReceived));
    // httpClient->TraceConnectWithoutContext ("RxEmbeddedObject", MakeCallback (&ClientEmbeddedObjectReceived));
    // httpClient->TraceConnectWithoutContext ("Rx", MakeCallback (&ClientRx));
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        clientApps = clientHelper.Install(ueNodes.Get(u));
        Ptr<ThreeGppHttpClient> httpClient = clientApps.Get(0)->GetObject<ThreeGppHttpClient>();

        // Example of connecting to the trace sources
        httpClient->TraceConnectWithoutContext("RxMainObject",
                                               MakeCallback(&ClientMainObjectReceived));
        httpClient->TraceConnectWithoutContext("RxEmbeddedObject",
                                               MakeCallback(&ClientEmbeddedObjectReceived));
        httpClient->TraceConnectWithoutContext("Rx", MakeCallback(&ClientRx));
    }

    serverApps.Start(MilliSeconds(500));
    clientApps.Start(MilliSeconds(500));

    double startTime = Simulator::Now().GetSeconds();
    double latencyForOperation = Simulator::Now().GetSeconds() - startTime;
    NS_LOG_INFO("Latency for the operation: " << latencyForOperation);

    // Run simulation
    Simulator::Stop(simTime);
    p2ph.EnablePcapAll("project");
    Ptr<FlowMonitor> monitor; // = flowMonHelper.InstallAll();
    FlowMonitorHelper flowMonHelper;

    // NetAnim animation
    AnimationInterface animation("project.xml"); // 1 Is SGW, 0 is PGW, 3 is Remote Host, 2 is MME,
                                                 // 6 and 7 are UE, 4 are 5 are eNodeB
    animation.UpdateNodeDescription(pgw,"PGW");
    animation.UpdateNodeDescription(1,"SGW");
    animation.UpdateNodeDescription(2,"MME");

    for (uint32_t u = 0; u < 5; ++u){
        animation.UpdateNodeDescription(ueNodes.Get (u),"UE_"+std::to_string(u));
        animation.UpdateNodeColor(ueNodes.Get (u), 0, 0, 255);
    }

    for (uint32_t u = 5; u < ueNodes.GetN (); ++u){
        animation.UpdateNodeDescription(ueNodes.Get (u),"UE_"+std::to_string(u));
        animation.UpdateNodeColor(ueNodes.Get (u), 0, 255, 0);
    }

    for (uint32_t u = 0; u < enbNodes.GetN (); ++u){
        animation.UpdateNodeDescription(enbNodes.Get (u),"eNodeB_"+std::to_string(u));
        animation.UpdateNodeColor(enbNodes.Get (u), 0, 255, 0);
    }

    animation.UpdateNodeDescription(remoteHostContainer.Get (0),"remoteHost"+std::to_string(0));
    animation.UpdateNodeColor(remoteHostContainer.Get (0), 0, 255, 0);

    monitor = flowMonHelper.Install(enbNodes);
    monitor = flowMonHelper.Install(ueNodes);
    monitor = flowMonHelper.Install(remoteHost);
    Simulator::Run();

    // GnuPlot
    std::string jmenoSouboru = "project_delay";
    std::string graphicsFileName = jmenoSouboru + ".png";
    std::string plotFileName = jmenoSouboru + ".plt";
    std::string plotTitle = "Average delay";
    std::string dataTitle = "Delay [ms]";
    Gnuplot gnuplot(graphicsFileName);
    gnuplot.SetTitle(plotTitle);
    gnuplot.SetTerminal("png");
    gnuplot.SetLegend("IDs of data streams", "Delay [ms]");
    gnuplot.AppendExtra("set xrange [1:" + std::to_string(numberOfUes * 4) + "]");
    gnuplot.AppendExtra("set yrange [0:100]");
    gnuplot.AppendExtra("set grid");
    Gnuplot2dDataset dataset_delay;

    jmenoSouboru = "project_data_rate";
    graphicsFileName = jmenoSouboru + ".png";
    std::string plotFileNameDR = jmenoSouboru + ".plt";
    plotTitle = "Data rate for IDs";
    dataTitle = "Data rate [kbps]";
    Gnuplot gnuplot_DR(graphicsFileName);
    gnuplot_DR.SetTitle(plotTitle);
    gnuplot_DR.SetTerminal("png");
    gnuplot_DR.SetLegend("IDs of all streams", "Data rate [kbps]");
    gnuplot_DR.AppendExtra("set xrange [1:" + std::to_string(numberOfUes * 4) + "]");
    gnuplot_DR.AppendExtra("set yrange [0:2500]");
    gnuplot_DR.AppendExtra("set grid");
    Gnuplot2dDataset dataset_rate;

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowMonHelper.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    monitor->SerializeToXmlFile("project.flowmon", true, true);

    std::cout << std::endl << "*** Flow monitor statistic ***" << std::endl;
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin();
         i != stats.end();
         ++i)
    {
        // if (i-> first > 2) {
        double Delay, DataRate;
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        std::cout << "Flow ID: " << i->first << std::endl;
        std::cout << "Src add: " << t.sourceAddress << "-> Dst add: " << t.destinationAddress
                  << std::endl;
        std::cout << "Src port: " << t.sourcePort << "-> Dst port: " << t.destinationPort
                  << std::endl;
        std::cout << "Tx Packets/Bytes: " << i->second.txPackets << "/" << i->second.txBytes
                  << std::endl;
        std::cout << "Rx Packets/Bytes: " << i->second.rxPackets << "/" << i->second.rxBytes
                  << std::endl;
        std::cout << "Throughput: "
                  << i->second.rxBytes * 8.0 /
                         (i->second.timeLastRxPacket.GetSeconds() -
                          i->second.timeFirstTxPacket.GetSeconds()) /
                         1024
                  << "kb/s" << std::endl;
        std::cout << "Delay sum: " << i->second.delaySum.GetMilliSeconds() << "ms" << std::endl;
        std::cout << "Mean delay: "
                  << (i->second.delaySum.GetSeconds() / i->second.rxPackets) * 1000 << "ms"
                  << std::endl;

        // gnuplot Delay
        Delay = (i->second.delaySum.GetSeconds() / i->second.rxPackets) * 1000;
        DataRate =
            i->second.rxBytes * 8.0 /
            (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds()) /
            1024;

        dataset_delay.Add((double)i->first, (double)Delay);
        dataset_rate.Add((double)i->first, (double)DataRate);
        std::cout << "Jitter sum: " << i->second.jitterSum.GetMilliSeconds() << "ms" << std::endl;
        std::cout << "Mean jitter: "
                  << (i->second.jitterSum.GetSeconds() / (i->second.rxPackets - 1)) * 1000 << "ms"
                  << std::endl;
        // std::cout << "Lost Packets: " << i->second.lostPackets << std::endl;
        std::cout << "Lost Packets: " << i->second.txPackets - i->second.rxPackets << std::endl;
        std::cout << "Packet loss: "
                  << (((i->second.txPackets - i->second.rxPackets) * 1.0) / i->second.txPackets) *
                         100
                  << "%" << std::endl;
        std::cout << "------------------------------------------------" << std::endl;
    }

    // Gnuplot - continuation
    gnuplot.AddDataset(dataset_delay);
    std::ofstream plotFile(plotFileName.c_str());
    gnuplot.GenerateOutput(plotFile);
    plotFile.close();
    gnuplot_DR.AddDataset(dataset_rate);
    std::ofstream plotFileDR(plotFileNameDR.c_str());
    gnuplot_DR.GenerateOutput(plotFileDR);
    plotFileDR.close();

    Simulator::Destroy();

    return 0;
}
