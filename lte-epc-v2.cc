/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011-2018 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
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
 *
 * Authors: Jaume Nin <jaume.nin@cttc.cat>
 *          Manuel Requena <manuel.requena@cttc.es>
 *          Pavel Masek <masekpavel@vut.cz>
 *          Dinh Thao Le <243759@vut.cz>
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/lte-module.h"
#include "ns3/netanim-module.h"
#include "ns3/random-waypoint-mobility-model.h"
#include <iostream>
#include <cstdlib>
#include <ctime>



using namespace ns3;

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeBs,
 * attaches one UE per eNodeB starts a flow for each UE to and from a remote host.
 * It also starts another flow between each UE pair.
 */

NS_LOG_COMPONENT_DEFINE ("lte-basic-epc");

void
ServerConnectionEstablished (Ptr<const ThreeGppHttpServer>, Ptr<Socket>)
{
  NS_LOG_INFO ("Client has established a connection to the server.");
}

void
MainObjectGenerated (uint32_t size)
{
  NS_LOG_INFO ("Server generated a main object of " << size << " bytes.");
}

void
EmbeddedObjectGenerated (uint32_t size)
{
  NS_LOG_INFO ("Server generated an embedded object of " << size << " bytes.");
}

void
ServerTx (Ptr<const Packet> packet)
{
  NS_LOG_INFO ("Server sent a packet of " << packet->GetSize () << " bytes.");
}

void
ClientRx (Ptr<const Packet> packet, const Address &address)
{
  NS_LOG_INFO ("Client received a packet of " << packet->GetSize () << " bytes from " << address);
}

void
ClientMainObjectReceived (Ptr<const ThreeGppHttpClient>, Ptr<const Packet> packet)
{
  Ptr<Packet> p = packet->Copy ();
  ThreeGppHttpHeader header;
  p->RemoveHeader (header);
  if (header.GetContentLength () == p->GetSize ()
      && header.GetContentType () == ThreeGppHttpHeader::MAIN_OBJECT)
    {
      NS_LOG_INFO ("Client has successfully received a main object of "
                   << p->GetSize () << " bytes.");
    }
  else
    {
      NS_LOG_INFO ("Client failed to parse a main object. ");
    }
}

void
ClientEmbeddedObjectReceived (Ptr<const ThreeGppHttpClient>, Ptr<const Packet> packet)
{
  Ptr<Packet> p = packet->Copy ();
  ThreeGppHttpHeader header;
  p->RemoveHeader (header);
  if (header.GetContentLength () == p->GetSize ()
      && header.GetContentType () == ThreeGppHttpHeader::EMBEDDED_OBJECT)
    {
      NS_LOG_INFO ("Client has successfully received an embedded object of "
                   << p->GetSize () << " bytes.");
    }
  else
    {
      NS_LOG_INFO ("Client failed to parse an embedded object. ");
    }
}

int
main (int argc, char *argv[])
{
  uint16_t numNodePairs = 2;
  Time simTime = MilliSeconds (10000);
  double distance = 60.0;
  Time interPacketInterval = MilliSeconds (200);
  bool disableDl = false;
  bool disableUl = false;
  bool disablePl = false;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue ("numNodePairs", "Number of eNodeBs + UE pairs", numNodePairs);
  cmd.AddValue ("simTime", "Total duration of the simulation", simTime);
  cmd.AddValue ("distance", "Distance between eNBs [m]", distance);
  cmd.AddValue ("interPacketInterval", "Inter packet interval", interPacketInterval);
  cmd.AddValue ("disableDl", "Disable downlink data flows", disableDl);
  cmd.AddValue ("disableUl", "Disable uplink data flows", disableUl);
  cmd.AddValue ("disablePl", "Disable data flows between peer UEs", disablePl);
  cmd.Parse (argc, argv);

  // ConfigStore inputConfig;
  // inputConfig.ConfigureDefaults ();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);

  

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> (); // create LteHelper object
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> (); // PointToPointEpcHelper
  lteHelper->SetEpcHelper (epcHelper); // enable the use of EPC by LTE helper

  Ptr<Node> pgw = epcHelper->GetPgwNode (); // get the PGW node

   // Create a single RemoteHost
  NodeContainer remoteHostContainer; // container for remote node
  remoteHostContainer.Create (1); // create 1 node
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer); // aggregate stack implementations (ipv4, ipv6, udp, tcp) to remote node
  

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s"))); 
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500)); // p2p mtu
  p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (10)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0"); 
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices); // assign IPs to PGW and remote host
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1); // add route

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create (numNodePairs); // create eNB nodes
  ueNodes.Create (10); // create UE nodes
  
  // Install Mobility Model
  Ptr<ListPositionAllocator> positionAllocEnb = CreateObject<ListPositionAllocator> ();
  Ptr<ListPositionAllocator> positionAllocUe = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < numNodePairs; i++)
    {
      positionAllocEnb->Add (Vector (distance * i, 5, 0));
      positionAllocUe->Add (Vector (distance * i, 0, 0));
    }
  MobilityHelper mobility;
  // Install the mobility model to the nodes
  mobility.SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
                              "Speed", StringValue ("ns3::UniformRandomVariable[Min=10|Max=20]"),
                              "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.005]"),
                              "Bounds", RectangleValue (Rectangle (-10, 70, -25, 25)));
  mobility.SetPositionAllocator(positionAllocUe);
  mobility.Install (ueNodes);

  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel"); // mobility model (constant)
  mobility.SetPositionAllocator(positionAllocEnb);
  mobility.Install(enbNodes);

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes); // add eNB nodes to the container
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes); // add UE nodes to the container

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  // Assign IP address to UEs, set static route
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1); // default route
    }

  // Attach one UE per eNodeB
  for (uint16_t i = 0; i < numNodePairs; i++)
  {
    if (i % 2 == 0)
    {
      lteHelper->Attach(ueLteDevs.Get(i), enbLteDevs.Get(0)); // attach UE nodes to the 1st eNB node
    }
    else
    {
      lteHelper->Attach(ueLteDevs.Get(i), enbLteDevs.Get(1)); // attach UE nodes to the 2nd eNB node
    }
    // side effect: the default EPS bearer will be activated
  }
  // Create HTTP server helper
  ThreeGppHttpServerHelper serverHelper (remoteHostAddr);

  // Install HTTP server
  ApplicationContainer serverApps = serverHelper.Install (remoteHostContainer.Get (0));
  Ptr<ThreeGppHttpServer> httpServer = serverApps.Get (0)->GetObject<ThreeGppHttpServer> ();

  // Example of connecting to the trace sources
  httpServer->TraceConnectWithoutContext ("ConnectionEstablished",
                                          MakeCallback (&ServerConnectionEstablished));
  httpServer->TraceConnectWithoutContext ("MainObject", MakeCallback (&MainObjectGenerated));
  httpServer->TraceConnectWithoutContext ("EmbeddedObject", MakeCallback (&EmbeddedObjectGenerated));
  httpServer->TraceConnectWithoutContext ("Tx", MakeCallback (&ServerTx));

  // Setup HTTP variables for the server
  PointerValue varPtr;
  httpServer->GetAttribute ("Variables", varPtr);
  Ptr<ThreeGppHttpVariables> httpVariables = varPtr.Get<ThreeGppHttpVariables> ();
  httpVariables->SetMainObjectSizeMean (102400); // 100kB
  httpVariables->SetMainObjectSizeStdDev (40960); // 40kB


  // Create HTTP client helper
  ThreeGppHttpClientHelper clientHelper (remoteHostAddr);
  ApplicationContainer clientApps;
  // Install HTTP client
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
  { 
  
    // Seed the random number generator
    std::srand(std::time(0));

    // Generate a random number in the range from 1 to 2
    int randomNumber = 1 + std::rand() % 2;

    if(randomNumber == 1){
      clientApps = clientHelper.Install (ueNodes.Get (u));
      Ptr<ThreeGppHttpClient> httpClient = clientApps.Get (0)->GetObject<ThreeGppHttpClient> ();

      // Example of connecting to the trace sources
      httpClient->TraceConnectWithoutContext ("RxMainObject", MakeCallback (&ClientMainObjectReceived));
      httpClient->TraceConnectWithoutContext ("RxEmbeddedObject", MakeCallback (&ClientEmbeddedObjectReceived));
      httpClient->TraceConnectWithoutContext ("Rx", MakeCallback (&ClientRx));
    }

    
  }

  serverApps.Start (MilliSeconds (500));
 clientApps.Start (MilliSeconds (500));
  // lteHelper->EnableTraces ();

  
  p2ph.EnablePcapAll("lte-epc");

  Simulator::Stop (simTime);

  
  Simulator::Run ();

  // GtkConfigStore config;
  // config.ConfigureAttributes();

  // AnimationInterface anim("lte-epc.xml");
  // anim.UpdateNodeDescription (pgw, "PGW");
  // anim.UpdateNodeDescription (remoteHost, "Remote Host");
  // anim.UpdateNodeDescription (1, "SGW");
  // anim.UpdateNodeDescription (2, "MME");

  // for (uint32_t u=0; u<ueNodes.GetN(); ++u)
  // {
  //   anim.UpdateNodeDescription (ueNodes.Get(u), "UE");
  //   anim.UpdateNodeColor (ueNodes.Get(u), 255, 0, 0);
  // }

  // for (uint32_t e=0; e<enbNodes.GetN(); ++e)
  // {
  //   anim.UpdateNodeDescription (enbNodes.Get(e), "eNodeB_"+ std::to_string(e));
  //   anim.UpdateNodeColor (enbNodes.Get(e), 0, 255, 0);
  // }

  Simulator::Destroy ();
  
  NS_LOG_UNCOND("End");
  
  return 0;
}
