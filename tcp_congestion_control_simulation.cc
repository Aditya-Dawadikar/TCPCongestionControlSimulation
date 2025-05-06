#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/tcp-vegas.h"
#include "ns3/tcp-westwood-plus.h"
#include "ns3/tcp-bbr.h"
#include "ns3/tcp-cubic.h"
#include "ns3/tcp-veno.h"
#include "ns3/ping-helper.h"

using namespace ns3;

TypeId GetTcpVariant (const std::string& variant)
{
    if (variant == "TcpVegas") return TcpVegas::GetTypeId();
    if (variant == "TcpWestwoodPlus") return TcpWestwoodPlus::GetTypeId();
    if (variant == "TcpBbr") return TcpBbr::GetTypeId();
    if (variant == "TcpCubic") return TcpCubic::GetTypeId();
    if (variant == "TcpVeno") return TcpVeno::GetTypeId();
    NS_ABORT_MSG ("Unknown TCP variant: " << variant);
}

void RunScenario1 (const std::string& tcpVariant, double cbrRateMbps)
{
    NodeContainer nodes;
    nodes.Create (5);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("10ms"));

    NetDeviceContainer d0 = p2p.Install (NodeContainer (nodes.Get (0), nodes.Get (2)));
    NetDeviceContainer d1 = p2p.Install (NodeContainer (nodes.Get (1), nodes.Get (2)));
    NetDeviceContainer d2 = p2p.Install (NodeContainer (nodes.Get (2), nodes.Get (3)));
    NetDeviceContainer d3 = p2p.Install (NodeContainer (nodes.Get (3), nodes.Get (4)));

    InternetStackHelper stack;
    stack.Install (nodes);

    Ipv4AddressHelper address;
    Ipv4InterfaceContainer i0, i1, i2, i3;
    address.SetBase ("10.1.1.0", "255.255.255.0"); i0 = address.Assign (d0);
    address.SetBase ("10.1.2.0", "255.255.255.0"); i1 = address.Assign (d1);
    address.SetBase ("10.1.3.0", "255.255.255.0"); i2 = address.Assign (d2);
    address.SetBase ("10.1.4.0", "255.255.255.0"); i3 = address.Assign (d3);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (GetTcpVariant (tcpVariant)));

    BulkSendHelper tcpSource ("ns3::TcpSocketFactory",
        InetSocketAddress (i3.GetAddress (1), 8080));
    tcpSource.SetAttribute ("MaxBytes", UintegerValue (0));
    tcpSource.Install (nodes.Get (0)).Start (Seconds (1.0));

    PacketSinkHelper tcpSink ("ns3::TcpSocketFactory",
        InetSocketAddress (Ipv4Address::GetAny (), 8080));
    tcpSink.Install (nodes.Get (4)).Start (Seconds (0.0));

    OnOffHelper udpApp ("ns3::UdpSocketFactory",
        InetSocketAddress (i3.GetAddress (1), 9000));
    udpApp.SetAttribute ("DataRate", DataRateValue (DataRate (std::to_string (int (cbrRateMbps)) + "Mbps")));
    udpApp.SetAttribute ("PacketSize", UintegerValue (950));
    udpApp.SetAttribute ("StartTime", TimeValue (Seconds (1.0)));
    udpApp.SetAttribute ("StopTime", TimeValue (Seconds (50.0)));
    udpApp.Install (nodes.Get (1));

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

    Simulator::Stop (Seconds (50.0));
    Simulator::Run ();

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier());
    auto stats = monitor->GetFlowStats();

    for (auto& entry : stats)
    {
        auto fiveTuple = classifier->FindFlow (entry.first);
        if (fiveTuple.destinationPort == 8080)
        {
            double throughput = entry.second.rxBytes * 8.0 / (49.0 * 1e6);
            double avgRttMs = entry.second.delaySum.GetSeconds () * 1000.0 / entry.second.rxPackets;
            double dropRate = (entry.second.txPackets - entry.second.rxPackets) / (double)entry.second.txPackets;

            std::cout << "Scenario1," << tcpVariant
                      << ", " << cbrRateMbps
                      << ", " << throughput
                      << ", " << avgRttMs
                      << ", " << dropRate << std::endl;
        }
    }

    Simulator::Destroy ();
}

void RunScenario2 (const std::string& tcpVariant, double cbrRateMbps)
{
    NodeContainer nodes;
    nodes.Create (9);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("10ms"));

    std::vector<NetDeviceContainer> devs;
    auto link = [&](int a, int b) {
        NetDeviceContainer d = p2p.Install (NodeContainer (nodes.Get (a), nodes.Get (b)));
        devs.push_back(d);
        return d;
    };

    link(0,2); link(1,2); link(2,3); link(2,4); link(2,5);
    link(3,6); link(4,6); link(5,6); link(6,7); link(6,8);

    InternetStackHelper stack;
    stack.Install (nodes);

    Ipv4AddressHelper address;
    int subnet = 1;
    std::vector<Ipv4InterfaceContainer> interfaces;
    for (auto& d : devs) {
        std::ostringstream subnetStr;
        subnetStr << "10.2." << subnet++ << ".0";
        address.SetBase (subnetStr.str ().c_str (), "255.255.255.0");
        interfaces.push_back (address.Assign (d));
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (GetTcpVariant (tcpVariant)));

    BulkSendHelper tcp1 ("ns3::TcpSocketFactory",
        InetSocketAddress (nodes.Get (7)->GetObject<Ipv4>()->GetAddress (1,0).GetLocal (), 8080));
    tcp1.SetAttribute ("MaxBytes", UintegerValue (0));
    tcp1.Install (nodes.Get (0)).Start (Seconds (1.0));

    PacketSinkHelper sink1 ("ns3::TcpSocketFactory",
        InetSocketAddress (Ipv4Address::GetAny (), 8080));
    sink1.Install (nodes.Get (7)).Start (Seconds (0.0));

    // Now override TCP variant for Flow 2 to Vegas
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpVegas::GetTypeId()));

    BulkSendHelper tcp2 ("ns3::TcpSocketFactory",
        InetSocketAddress (nodes.Get (8)->GetObject<Ipv4>()->GetAddress (1,0).GetLocal (), 8081));
    tcp2.SetAttribute ("MaxBytes", UintegerValue (0));
    tcp2.Install (nodes.Get (1)).Start (Seconds (1.0));

    PacketSinkHelper sink2 ("ns3::TcpSocketFactory",
        InetSocketAddress (Ipv4Address::GetAny (), 8081));
    sink2.Install (nodes.Get (8)).Start (Seconds (0.0));

    // Restore global socket type if needed
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (GetTcpVariant (tcpVariant)));

    OnOffHelper udp ("ns3::UdpSocketFactory",
        InetSocketAddress (nodes.Get (7)->GetObject<Ipv4>()->GetAddress (1,0).GetLocal (), 9000));
    udp.SetAttribute ("DataRate", DataRateValue (DataRate (std::to_string (int(cbrRateMbps)) + "Mbps")));
    udp.SetAttribute ("PacketSize", UintegerValue (950));
    udp.SetAttribute ("StartTime", TimeValue (Seconds (1.0)));
    udp.SetAttribute ("StopTime", TimeValue (Seconds (100.0)));
    udp.Install (nodes.Get (1));

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

    Simulator::Stop (Seconds (100.0));
    Simulator::Run ();

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier());
    auto stats = monitor->GetFlowStats();

    for (auto& entry : stats)
    {
        auto fiveTuple = classifier->FindFlow (entry.first);
        if (fiveTuple.destinationPort == 8080)
        {
            double throughput = entry.second.rxBytes * 8.0 / (99.0 * 1e6);
            double avgRttMs = entry.second.delaySum.GetSeconds () * 1000.0 / entry.second.rxPackets;
            double dropRate = (entry.second.txPackets - entry.second.rxPackets) / (double)entry.second.txPackets;

            std::cout << "Scenario2," << tcpVariant
                      << ", " << cbrRateMbps
                      << ", " << throughput
                      << ", " << avgRttMs
                      << ", " << dropRate << std::endl;
        }
    }

    Simulator::Destroy ();
}

void RunScenario3(const std::string& tcpVariant, double cbrRateMbps)
{
    NodeContainer senders, receivers, routers;
    senders.Create(4);
    receivers.Create(4);
    routers.Create(2); // RouterLeft, RouterRight

    PointToPointHelper accessLink;
    accessLink.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    accessLink.SetChannelAttribute("Delay", StringValue("2ms"));

    PointToPointHelper bottleneck;
    bottleneck.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    bottleneck.SetChannelAttribute("Delay", StringValue("10ms"));

    InternetStackHelper stack;
    stack.InstallAll();

    Ipv4AddressHelper address;
    int subnet = 1;

    auto connect = [&](Ptr<Node> a, Ptr<Node> b) {
        NetDeviceContainer d = accessLink.Install(NodeContainer(a, b));
        std::ostringstream subnetStr;
        subnetStr << "10.5." << subnet++ << ".0";
        address.SetBase(subnetStr.str().c_str(), "255.255.255.0");
        return address.Assign(d);
    };

    for (int i = 0; i < 4; ++i)
    {
        connect(senders.Get(i), routers.Get(0));
        connect(receivers.Get(i), routers.Get(1));
    }

    NetDeviceContainer bottleneckDev = bottleneck.Install(NodeContainer(routers.Get(0), routers.Get(1)));
    address.SetBase("10.5.100.0", "255.255.255.0");
    address.Assign(bottleneckDev);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(GetTcpVariant(tcpVariant)));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1000));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1 << 20));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1 << 20));

    // TCP: Sender 0 -> Receiver 0
    BulkSendHelper tcp("ns3::TcpSocketFactory",
        InetSocketAddress(receivers.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), 8080));
    tcp.SetAttribute("MaxBytes", UintegerValue(0));
    tcp.SetAttribute("SendSize", UintegerValue(1000));
    tcp.Install(senders.Get(0)).Start(Seconds(1.0));

    PacketSinkHelper sink("ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), 8080));
    sink.Install(receivers.Get(0)).Start(Seconds(0.0));

    // Background UDP: Sender 1 → Receiver 1, 2, 3
    for (int i = 1; i < 4; ++i)
    {
        OnOffHelper udp("ns3::UdpSocketFactory",
            InetSocketAddress(receivers.Get(i)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), 9000 + i));
        udp.SetAttribute("DataRate", DataRateValue(DataRate(std::to_string(int(cbrRateMbps)) + "Mbps")));
        udp.SetAttribute("PacketSize", UintegerValue(950));
        udp.SetAttribute("StartTime", TimeValue(Seconds(1.0)));
        udp.SetAttribute("StopTime", TimeValue(Seconds(100.0)));
        udp.Install(senders.Get(i));
    }

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(Seconds(100.0));
    Simulator::Run();

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    auto stats = monitor->GetFlowStats();

    for (auto& entry : stats) {
        auto fiveTuple = classifier->FindFlow(entry.first);
        if (fiveTuple.destinationPort == 8080) {
            double throughput = entry.second.rxBytes * 8.0 / (99.0 * 1e6);
            double avgRttMs = (entry.second.rxPackets > 0) ? entry.second.delaySum.GetSeconds() * 1000.0 / entry.second.rxPackets : -1.0;
            double dropRate = (entry.second.txPackets > 0) ? (entry.second.txPackets - entry.second.rxPackets) / (double)entry.second.txPackets : 1.0;

            std::cout << "Scenario3," << tcpVariant
                      << ", " << cbrRateMbps
                      << ", " << throughput
                      << ", " << avgRttMs
                      << ", " << dropRate << std::endl;
        }
    }

    Simulator::Destroy();
}

void RunScenario4(const std::string& tcpVariant, double cbrRateMbps)
{
    NodeContainer senders, receivers, routers;
    senders.Create(4);
    receivers.Create(4);
    routers.Create(2); // RouterLeft, RouterRight

    PointToPointHelper accessLink;
    accessLink.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    accessLink.SetChannelAttribute("Delay", StringValue("2ms"));

    PointToPointHelper bottleneck;
    bottleneck.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    bottleneck.SetChannelAttribute("Delay", StringValue("10ms"));

    InternetStackHelper stack;
    stack.InstallAll();

    Ipv4AddressHelper address;
    int subnet = 1;

    auto connect = [&](Ptr<Node> a, Ptr<Node> b) {
        NetDeviceContainer d = accessLink.Install(NodeContainer(a, b));
        std::ostringstream subnetStr;
        subnetStr << "10.5." << subnet++ << ".0";
        address.SetBase(subnetStr.str().c_str(), "255.255.255.0");
        return address.Assign(d);
    };

    for (int i = 0; i < 4; ++i)
    {
        connect(senders.Get(i), routers.Get(0));
        connect(receivers.Get(i), routers.Get(1));
    }

    NetDeviceContainer bottleneckDev = bottleneck.Install(NodeContainer(routers.Get(0), routers.Get(1)));
    address.SetBase("10.5.100.0", "255.255.255.0");
    address.Assign(bottleneckDev);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(GetTcpVariant(tcpVariant)));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1000));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1 << 20));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1 << 20));

    // TCP: Sender 0 -> Receiver 0
    BulkSendHelper tcp("ns3::TcpSocketFactory",
        InetSocketAddress(receivers.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), 8080));
    tcp.SetAttribute("MaxBytes", UintegerValue(0));
    tcp.SetAttribute("SendSize", UintegerValue(1000));
    tcp.Install(senders.Get(0)).Start(Seconds(1.0));

    PacketSinkHelper sink("ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), 8080));
    sink.Install(receivers.Get(0)).Start(Seconds(0.0));

    // Background bursty UDP traffic: Sender 1–3 → Receiver 1–3
    for (int i = 1; i < 4; ++i)
    {
        OnOffHelper udp("ns3::UdpSocketFactory",
            InetSocketAddress(receivers.Get(i)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), 9000 + i));
        udp.SetAttribute("DataRate", DataRateValue(DataRate(std::to_string(int(cbrRateMbps)) + "Mbps")));
        udp.SetAttribute("PacketSize", UintegerValue(950));
        udp.SetAttribute("OnTime", StringValue("ns3::UniformRandomVariable[Min=0.5|Max=1.5]"));
        udp.SetAttribute("OffTime", StringValue("ns3::UniformRandomVariable[Min=0.5|Max=1.5]"));
        udp.SetAttribute("StartTime", TimeValue(Seconds(1.0)));
        udp.SetAttribute("StopTime", TimeValue(Seconds(100.0)));
        udp.Install(senders.Get(i));
    }

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(Seconds(100.0));
    Simulator::Run();

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    auto stats = monitor->GetFlowStats();

    for (auto& entry : stats) {
        auto fiveTuple = classifier->FindFlow(entry.first);
        if (fiveTuple.destinationPort == 8080) {
            double throughput = entry.second.rxBytes * 8.0 / (99.0 * 1e6);
            double avgRttMs = (entry.second.rxPackets > 0) ? entry.second.delaySum.GetSeconds() * 1000.0 / entry.second.rxPackets : -1.0;
            double dropRate = (entry.second.txPackets > 0) ? (entry.second.txPackets - entry.second.rxPackets) / (double)entry.second.txPackets : 1.0;

            std::cout << "Scenario4," << tcpVariant
                      << ", " << cbrRateMbps
                      << ", " << throughput
                      << ", " << avgRttMs
                      << ", " << dropRate << std::endl;
        }
    }

    Simulator::Destroy();
}


int main (int argc, char *argv[])
{
    std::vector<std::string> tcpVariants = {"TcpVegas", "TcpWestwoodPlus", "TcpBbr", "TcpCubic", "TcpVeno"};
    std::cout << "Scenario,Variant,CBR(Mbps),Throughput(Mbps),AvgRTT(ms),DropRate" << std::endl;

    for (const auto& variant : tcpVariants)
    {
        for (int rate = 1; rate <= 10; ++rate)
        {
            RunScenario1 (variant, rate);
        }
    }
    for (const auto& variant : tcpVariants)
    {
        for (int rate = 1; rate <= 10; ++rate)
        {
            RunScenario2 (variant, rate);
        }
    }
    for (const auto& variant : tcpVariants)
    {
        for (int rate = 1; rate <= 10; ++rate)
        {
            RunScenario3 (variant, rate);
        }
    }
    for (const auto& variant : tcpVariants)
    {
        for (int rate = 1; rate <= 10; ++rate)
        {
            RunScenario4 (variant, rate);
        }
    }

    // Benchmarking
    int rate = 10;
    for (const auto& variant : tcpVariants)
    {
        for (int i = 1; i <= 10; ++i)
        {
            RunScenario4 (variant, rate);
        }
    }

    return 0;
}