#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/tcp-vegas.h"
#include "ns3/tcp-veno.h"
#include "ns3/tcp-westwood-plus.h"
#include "ns3/tcp-bbr.h"
#include "ns3/tcp-cubic.h"

using namespace ns3;

TypeId GetTcpVariant(const std::string &variant)
{
  if (variant == "TcpVegas") return TcpVegas::GetTypeId();
  if (variant == "TcpVeno") return TcpVeno::GetTypeId();
  if (variant == "TcpWestwoodPlus") return TcpWestwoodPlus::GetTypeId();
  if (variant == "TcpBbr") return TcpBbr::GetTypeId();
  if (variant == "TcpCubic") return TcpCubic::GetTypeId();
  NS_ABORT_MSG("Invalid TCP variant");
}

void RunScenario1(const std::string& variant, double cbrRateMbps)
{
  Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(GetTcpVariant(variant)));
  Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(1));

  NodeContainer hosts, core, agg, edge;
  NodeContainer allNodes;
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("2ms"));
  p2p.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", QueueSizeValue(QueueSize("5p")));

  uint32_t k = 4;
  uint32_t pods = k;
  uint32_t numCore = (k / 2) * (k / 2);
  uint32_t numAggPerPod = k / 2;
  uint32_t numEdgePerPod = k / 2;
  uint32_t numHostsPerEdge = k / 2;

  core.Create(numCore);
  allNodes.Add(core);

  std::vector<NodeContainer> aggSwitches, edgeSwitches, hostGroups;
  for (uint32_t p = 0; p < pods; ++p)
  {
    NodeContainer agg, edge;
    agg.Create(numAggPerPod);
    edge.Create(numEdgePerPod);
    aggSwitches.push_back(agg);
    edgeSwitches.push_back(edge);
    allNodes.Add(agg);
    allNodes.Add(edge);

    for (uint32_t e = 0; e < numEdgePerPod; ++e)
    {
      NodeContainer hosts;
      hosts.Create(numHostsPerEdge);
      hostGroups.push_back(hosts);
      allNodes.Add(hosts);
    }
  }

  InternetStackHelper stack;
  stack.Install(allNodes);

  Ptr<Node> src = hostGroups[0].Get(0);
  Ptr<Node> dst = hostGroups[hostGroups.size() - 1].Get(1);

  NodeContainer router;
  router.Create(1);
  stack.Install(router);
  allNodes.Add(router);

  NetDeviceContainer dev1 = p2p.Install(NodeContainer(src, router.Get(0)));
  Ipv4AddressHelper addr1;
  addr1.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer iface1 = addr1.Assign(dev1);

  NetDeviceContainer dev2 = p2p.Install(NodeContainer(router.Get(0), dst));
  Ipv4AddressHelper addr2;
  addr2.SetBase("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer iface2 = addr2.Assign(dev2);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  uint16_t port = 5000;
  BulkSendHelper tcpSource("ns3::TcpSocketFactory", InetSocketAddress(iface2.GetAddress(1), port));
  tcpSource.SetAttribute("MaxBytes", UintegerValue(0));
  ApplicationContainer tcpApps = tcpSource.Install(src);
  tcpApps.Start(Seconds(1.0));

  PacketSinkHelper tcpSink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
  ApplicationContainer tcpSinkApps = tcpSink.Install(dst);

  for (int i = 0; i < 6; ++i)
  {
    Ptr<Node> udpSrc = hostGroups[(i + 1) % hostGroups.size()].Get(0);
    OnOffHelper udp("ns3::UdpSocketFactory", InetSocketAddress(iface2.GetAddress(1), 9000 + i));
    udp.SetAttribute("DataRate", DataRateValue(DataRate(std::to_string(int(cbrRateMbps)) + "Mbps")));
    udp.SetAttribute("PacketSize", UintegerValue(200));
    udp.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    udp.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    udp.SetAttribute("StartTime", TimeValue(Seconds(0.5)));
    udp.SetAttribute("StopTime", TimeValue(Seconds(20.0)));
    udp.Install(udpSrc);

    PacketSinkHelper udpSink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9000 + i));
    udpSink.Install(dst);
  }

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  Simulator::Stop(Seconds(20.0));
  Simulator::Run();

  monitor->CheckForLostPackets();
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
  for (const auto& stat : stats)
  {
    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(stat.first);
    if (t.protocol != 6) continue;
    if (t.destinationPort != 5000) continue;

    double throughput = stat.second.rxBytes * 8.0 / (20.0 * 1e6);
    double avgRtt = stat.second.delaySum.GetSeconds() / stat.second.rxPackets * 1000;
    double dropRate = stat.second.lostPackets * 1.0 / (stat.second.txPackets + stat.second.lostPackets);
    std::cout << "Scenario3," << variant << "," << cbrRateMbps << "," << throughput << "," << avgRtt << "," << dropRate << std::endl;
  }

  Simulator::Destroy();
}



int main(int argc, char *argv[])
{
  std::vector<std::string> tcpVariants = {"TcpVegas", "TcpWestwoodPlus", "TcpBbr", "TcpCubic", "TcpVeno"};
  std::cout << "Scenario,Variant,CBR(Mbps),Throughput(Mbps),AvgRTT(ms),DropRate" << std::endl;

  for (const auto& variant : tcpVariants)
  {
    for (int rate = 1; rate <= 10; ++rate)
    {
      RunScenario1(variant, rate);
    }
  }

  return 0;
}
