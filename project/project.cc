#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;


int
main (int argc, char *argv[])
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

    // Attach one UE per eNodeB
    for (uint16_t i = 0; i < numNodePairs; i++)
      {
        lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(i)); // attach UE nodes to eNB nodes
        // side effect: the default EPS bearer will be activated
      }



    // Run simulation
    Simulator::Stop (simTime);
    Simulator::Run ();
    Simulator::Destroy ();



    return 0;

}