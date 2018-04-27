#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/lte-enb-phy.h"
#include "ns3/ff-mac-scheduler.h"
#include "ns3/pf-ff-mac-scheduler.h"
//#include "ns3/random-variable.h"
#include "ns3/lte-enb-net-device.h"
//#include "ns3/radio-bearer-status-calculator.h"
#include "ns3/friis-spectrum-propagation-loss.h"
#include "ns3/lte-enb-rrc.h"
#include "ns3/lte-common.h"
#include "ns3/trace-helper.h"
#include "ns3/lte-enb-net-device.h"
#include "ns3/packet-sink-helper.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("lena-cr-test");

int
main (int argc, char *argv[])
{

  uint16_t numberOfNodes = 1;
  uint8_t radius = 50;
  double simTime = 100;
  double distance = 60.0;
  double interPacketInterval = 100;
  double PoweNB = 35;
  double Powue = 20;
  uint8_t mimo = 2;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("numberOfNodes", "Number of eNodeBs + UE pairs", numberOfNodes);
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue("distance", "Distance between eNB and UE [m]", distance);
  cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
  cmd.Parse(argc, argv);

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);

  // Set of Antenna and Bandwidth
  lteHelper->SetEnbAntennaModelType("ns3::IsotropicAntennaModel");
  lteHelper->SetEnbDeviceAttribute("DlBandwidth", UintegerValue(50));
  lteHelper->SetEnbDeviceAttribute("UlBandwidth", UintegerValue(50));
  lteHelper->SetUeAntennaModelType("ns3::IsotropicAntennaModel");

  std::cout << "Set of Antenna and Bandwidth" << std::endl;

  // Configuration MIMO
  Config::SetDefault("ns3::LteEnbRrc::DefaultTransmissionMode", UintegerValue(mimo));
  Config::SetDefault("ns3::LteAmc::AmcModel", EnumValue(LteAmc::PiroEW2010));
  Config::SetDefault("ns3::LteAmc::Ber", DoubleValue(0.00005));

  std::cout << "Configuration MIMO" << std::endl;

  // Create a P-GW
  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  std::cout << "Create a P-GW" << std::endl;

   // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  std::cout << "Create a Single RemoteHost" << std::endl;

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  // Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  // Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create(numberOfNodes);
  ueNodes.Create(numberOfNodes);

  std::cout << "Create the Internet" << std::endl;

  // Position of eNB
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (8.0, 2.0, 0.0));
  MobilityHelper enbMobility;
  enbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbMobility.SetPositionAllocator (positionAlloc);
  enbMobility.Install (enbNodes);

  // Position of UE
  MobilityHelper ue1mobility;
  ue1mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                    "X", DoubleValue (1.0),
                                    "Y", DoubleValue (10.0),
                                    "rho", DoubleValue (radius));
  ue1mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  ue1mobility.Install (ueNodes);

  std::cout << "Set of Position" << std::endl;

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  // Set of Tx Power
  Ptr<LteEnbPhy> enbPhy = enbLteDevs.Get(0)->GetObject<LteEnbNetDevice>()->GetPhy();
  enbPhy->SetTxPower(PoweNB);
  enbPhy->SetAttribute("NoiseFigure", DoubleValue(5.0));

  Ptr<LteUePhy> uePhy = ueLteDevs.Get(0)->GetObject<LteUeNetDevice>()->GetPhy();
  uePhy->SetTxPower(Powue);
  uePhy->SetAttribute("NoiseFigure", DoubleValue(9.0));

  std::cout << "Set of Tx Power" << std::endl;

  // Set of Scheduler
  lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");
  
  std::cout << "Set of Scheduler" << std::endl;

  // Set of Pathloss Model
  lteHelper->SetAttribute ("PathlossModel", StringValue ("FriisPropagationLossModel"));

  std::cout << "Set of Pathloss Model" << std::endl;

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // Attach one UE per eNodeB
  for (uint16_t i = 0; i < numberOfNodes; i++)
      {
        lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(0));
      }
  
   // Activate an EPS bearer on all UEs
/*
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<NetDevice> ueDevice = ueLteDevs.Get (u);
      GbrQosInformation qos;
      qos.gbrDl = 132;  // bit/s, considering IP, UDP, RLC, PDCP header size
      qos.gbrUl = 132;
      qos.mbrDl = qos.gbrDl;
      qos.mbrUl = qos.gbrUl;
*/
      enum EpsBearer::Qci q = EpsBearer::NGBR_VIDEO_TCP_OPERATOR;
      //EpsBearer bearer (q, qos);
      EpsBearer bearer (q);
      //bearer.arp.priorityLevel = 15 - (u + 1);
      //bearer.arp.preemptionCapability = true;
      //bearer.arp.preemptionVulnerability = true;
      lteHelper->ActivateDataRadioBearer (ueLteDevs, bearer);
//    }
  
  std::cout << "ActivateEpsBearer" << std::endl;

  // Install and start applications on UE and remote host
  uint16_t dlPort = 20;
  //uint16_t ulPort = 2000;
  //uint16_t otherPort = 3000;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  serverApps.Start (Seconds (0.01));
  clientApps.Start (Seconds (0.01));

  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      //++ulPort;
      //++otherPort;
      ++dlPort;

      //PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
      //PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort));
      
      //serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));
      //serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
      //serverApps.Add (packetSinkHelper.Install (ueNodes.Get(u)));
/*
      UdpClientHelper dlClient (ueIpIface.GetAddress (u), dlPort);
      dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      dlClient.SetAttribute ("PacketSize", UintegerValue(1250));

      UdpClientHelper ulClient (remoteHostAddr, ulPort);
      ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      ulClient.SetAttribute ("PacketSize", UintegerValue(1250));
*/ 
      /*
      UdpClientHelper client (ueIpIface.GetAddress (u), otherPort);
      client.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      client.SetAttribute ("MaxPackets", UintegerValue(1000000));
      */
      //clientApps.Add (dlClient.Install (remoteHost));
      //clientApps.Add (ulClient.Install (ueNodes.Get(u)));
      /*
      if (u+1 < ueNodes.GetN ())
        {
          clientApps.Add (client.Install (ueNodes.Get(u+1)));
        }
      else
        {
          clientApps.Add (client.Install (ueNodes.Get(0)));
        }
      */
     BulkSendHelper dlClientHelper ("ns3::TcpSocketFactory", InetSocketAddress (ueIpIface.GetAddress (u), dlPort));
     dlClientHelper.SetAttribute ("MaxBytes", UintegerValue (10000000000));
     dlClientHelper.SetAttribute ("SendSize", UintegerValue (2000));

     clientApps.Add (dlClientHelper.Install (remoteHost));
     
     PacketSinkHelper dlPacketSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
     serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));
     
    }
  
  std::cout << "Install and start applications on UE and remote host" << std::endl;

  // Tracing
  lteHelper->EnableMacTraces ();
  lteHelper->EnableRlcTraces ();
  lteHelper->EnablePdcpTraces ();

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  // Simulation Start
  std::cout << "Simulation running" << std::endl;

  Simulator::Stop(Seconds(simTime));

  Simulator::Run();

  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter)
  {
  Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);

          NS_LOG_UNCOND("Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress);
          NS_LOG_UNCOND("Tx Packets = " << iter->second.txPackets);
          NS_LOG_UNCOND("Rx Packets = " << iter->second.rxPackets);
          NS_LOG_UNCOND("Throughput: " << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1024  << " Kbps");
  }

  monitor->SerializeToXmlFile ("result-test.xml" , true, true );
  
  Simulator::Destroy();
  return 0;

}