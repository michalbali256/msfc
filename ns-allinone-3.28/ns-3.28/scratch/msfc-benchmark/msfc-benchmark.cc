/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Universita' degli Studi di Napoli Federico II
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
 * Authors: Pasquale Imputato <p.imputato@gmail.com>
 *          Stefano Avallone <stefano.avallone@unina.it>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include <iostream>

#include "msfc-traces.h"
#include "msfc-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("BenchmarkQueueDiscs");


int main(int argc, char *argv[])
{
    std::string serversDelay = "50ms";
    std::string connectionDelay = "5ms";
    std::string queueDiscType = "PfifoFast";

    std::string flowsDatarate = "20Mbps";

    float startTime = 0.1; // in s
    float simDuration = 60;
    float samplingPeriod = 1;

    std::string serversDatarate = "1Gbps";
    std::string connectionDatarate = "1Gbps";

    bool netDeviceQueueTrace = false;
    bool qDiscQueueTrace = false;
    bool randomPriority = false;
    uint32_t numberOfPrios = 3;
    uint32_t msfcMultiplier = 2000;
    
    std::string flowsInFileName = "../flow_types.in";
    
    

    double netDeviceQueueSize = 0.00125; //1.25 ms
    double queueDiscQueueSize = 0.0001; // 100ms / 1000 (avg size of packet)

    CommandLine cmd;
    cmd.AddValue("queueDiscType", "Bottleneck queue disc type in {PfifoFast, Sfq, CoDel, FqCoDel, Msfc}", queueDiscType);
    cmd.AddValue("netDeviceQueueSize", "Netdevices queue size in seconds", netDeviceQueueSize);
    cmd.AddValue("queueDiscQueueSize", "Multiplier for queue disc size", queueDiscQueueSize);
    cmd.AddValue("startTime", "Simulation start time", startTime);
    cmd.AddValue("simDuration", "Simulation duration in seconds", simDuration);
    cmd.AddValue("samplingPeriod", "Goodput sampling period in seconds", samplingPeriod);
    cmd.AddValue("serversDatarate", "Datarate between the server and root of ISP tree", serversDatarate);
    cmd.AddValue("serversDelay", "Delay between the server and root of ISP tree", serversDelay);
    cmd.AddValue("connectionDatarate", "Datarate of p2p links in ISP tree", connectionDatarate);
    cmd.AddValue("connectionDelay", "Delay of p2p links in ISP tree", connectionDelay);    
    cmd.AddValue("netDeviceQueueTrace", "Determines if amount of bytes in net device queues will be traced", netDeviceQueueTrace);
    cmd.AddValue("qDiscQueueTrace", "Determines if amount of bytes in queuedisc queues will be traced", qDiscQueueTrace);
    cmd.AddValue("randomPriority", "Determines if priorities of flows will be chosen randomly or by flows config", randomPriority);
    cmd.AddValue("numberOfPrios", "If random priority is set, this determines number of used priorities.", numberOfPrios);    
    cmd.AddValue("msfcMultiplier", "multiplier for Msfc", msfcMultiplier);
    cmd.AddValue("flowsInFileName", "The location of the file with flows information", flowsInFileName);
    
    cmd.Parse(argc, argv);
    
    FlowType::LoadTypes(flowsInFileName);
    
    float stopTime = startTime + simDuration;

    NodeContainer root, leaves, all, clients;
    root.Create(1);
    //     depth       0  1  1  1  2  2  2  2  2  2  2  2  2, 3, 3, 3, 3, 3, 3, 3, 3
    size_t nChildren[]{3, 1, 3, 5, 4, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    InternetStackHelper stack;
    
    Ipv4AddressHelper address;
    address.SetBase("10.10.0.0", "255.255.255.252");

    std::queue<Ptr<Node>> next;
    next.push(root.Get(0));
    stack.Install(root);
    
    PointToPointHelper connP2PHelper;
    connP2PHelper.SetChannelAttribute("Delay", StringValue(connectionDelay));
    connP2PHelper.SetDeviceAttribute("DataRate", StringValue(connectionDatarate));

    //generation of ISP tree. Numbers of children is in nChildren, it is generated in BFS-way
    size_t iTree = -1;
    while (!next.empty())
    {

        Ptr<Node> parent = next.front();
        next.pop();

        all.Add(parent);

        ++iTree;
        if (nChildren[iTree] == 0)
        {
            leaves.Add(parent);
            continue;
        }

        for (size_t i = 0; i < nChildren[iTree]; ++i)
        {
            Ptr<Node> child = CreateObject<Node>();
            stack.Install(child);
            NetDeviceContainer link = connP2PHelper.Install(parent, child);

            address.Assign(link);
            address.NewNetwork();

            next.push(child);
        }
    }

    Ptr<UniformRandomVariable> clientNodesRandom = CreateObject<UniformRandomVariable>();
    clientNodesRandom->SetAttribute("Min", DoubleValue(8));
    clientNodesRandom->SetAttribute("Max", DoubleValue(12));

    address.SetBase("10.11.0.0", "255.255.255.0");

    Ipv4InterfaceContainer clientInterfaces;
    AsciiTraceHelper ascii;
    
    //Creation of wifi APs and clients. 8-12 clients random, 
    for (size_t i = 0; i < leaves.GetN(); ++i)
    {
        Ptr<Node> ap = leaves.Get(i);
        Names::Add("AP-" + std::to_string(i), ap);
        int nCl = clientNodesRandom->GetInteger();

        NodeContainer cl;
        cl.Create(nCl);
        stack.Install(cl);

        for (int j = 0; j < nCl; ++j)
        {
            Names::Add("Client-" + std::to_string(i) + "-" + std::to_string(j), cl.Get(j));
        }

        clients.Add(cl);
        all.Add(cl);

        YansWifiPhyHelper phyHelper = YansWifiPhyHelper::Default();
        YansWifiChannelHelper channelHelper = YansWifiChannelHelper::Default();
        phyHelper.SetChannel(channelHelper.Create());

        WifiHelper wifi;
        wifi.SetRemoteStationManager("ns3::IdealWifiManager");
        wifi.SetStandard(ns3::WifiPhyStandard::WIFI_PHY_STANDARD_80211n_2_4GHZ);
        WifiMacHelper mac;
        Ssid ssid = Ssid("ssid-" + std::to_string(i));

        
        mac.SetType("ns3::StaWifiMac",
                    "Ssid", SsidValue(ssid),
                    "ActiveProbing", BooleanValue(false));

        NetDeviceContainer staDevices;
        staDevices = wifi.Install(phyHelper, mac, cl);

        mac.SetType("ns3::ApWifiMac",
                    "Ssid", SsidValue(ssid));

        NetDeviceContainer apDevices;
        apDevices = wifi.Install(phyHelper, mac, ap);

        MobilityHelper mobility;

        mobility.SetPositionAllocator("ns3::UniformDiscPositionAllocator",
                                      "rho", DoubleValue(50),
                                      "X", DoubleValue(i * 300),
                                      "Y", DoubleValue(0.0));

        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility.Install(cl);
        mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                      "MinX", DoubleValue(i * 300),
                                      "MinY", DoubleValue(0.0),
                                      "DeltaX", DoubleValue(30.0),
                                      "DeltaY", DoubleValue(10.0),
                                      "GridWidth", UintegerValue(nCl),
                                      "LayoutType", StringValue("RowFirst"));
        mobility.Install(ap);

        address.Assign(apDevices);
        clientInterfaces.Add(address.Assign(staDevices));
        address.NewNetwork();
    }

    
    //setting up server
    NodeContainer s1;
    s1.Create(1);
    stack.Install(s1);

    Names::Add("Sink", s1.Get(0));

    //server link to root of ISP
    PointToPointHelper s1root;
    s1root.SetChannelAttribute("Delay", StringValue(serversDelay));
    s1root.SetDeviceAttribute("DataRate", StringValue(serversDatarate));

    address.NewNetwork();
    NetDeviceContainer s1p1Link = s1root.Install(s1.Get(0), root.Get(0));

    Ipv4InterfaceContainer interfacesS1 = address.Assign(s1p1Link);

    int a = 0;
    
    //sets netDevices queue size, installs qdisc
    for (auto it = NodeList::Begin(); it != NodeList::End(); it++)
    {

        for (size_t i = 0; i < (*it)->GetNDevices(); ++i)
        {

            Ptr<PointToPointNetDevice> device = StaticCast<PointToPointNetDevice>((*it)->GetDevice(i));
            if (!device->IsPointToPoint())
            {
                auto wifi = device->GetObject<WifiNetDevice>();
                if (wifi == 0)
                    continue;

                QueueDiscContainer queDisc = InstallQDisc(wifi, queueDiscType, 1000, msfcMultiplier);

                if (qDiscQueueTrace)
                {
                    Ptr<OutputStreamWrapper> streamBytesInQueueDisc = ascii.CreateFileStream(queueDiscType + "-" + std::to_string(a) + "-" + std::to_string(i) + "-PacketsInQueueDisc.txt");
                    queDisc.Get(0)->TraceConnectWithoutContext("PacketsInQueue", MakeBoundCallback(&BytesInQueueTrace, streamBytesInQueueDisc));
                }
                continue;
            }
            DataRateValue dataRate;
            device->GetAttribute("DataRate", dataRate);
            uint64_t rate = dataRate.Get().GetBitRate();

            
            int discSize = rate / 8 * queueDiscQueueSize;

            QueueDiscContainer queDisc = InstallQDisc(device, queueDiscType, discSize, msfcMultiplier);

            if (qDiscQueueTrace)
            {
                Ptr<OutputStreamWrapper> streamBytesInQueueDisc = ascii.CreateFileStream(queueDiscType + "-" + std::to_string(a) + "-" + std::to_string(i) + "-PacketsInQueueDisc.txt");
                queDisc.Get(0)->TraceConnectWithoutContext("PacketsInQueue", MakeBoundCallback(&BytesInQueueTrace, streamBytesInQueueDisc));
            }

            int byteSize = rate / 8 * netDeviceQueueSize;
            device->GetQueue()->SetMaxSize(QueueSize(QueueSizeUnit::BYTES, byteSize));

            if (netDeviceQueueTrace)
            {
                Ptr<OutputStreamWrapper> streamBytesInQueue = ascii.CreateFileStream(queueDiscType + "-" + std::to_string(a) + "-" + std::to_string(i) + "-bytesInQueue.txt");
                device->GetQueue()->TraceConnectWithoutContext("BytesInQueue", MakeBoundCallback(&BytesInQueueTrace, streamBytesInQueue));
            }
        }
        ++a;
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpNewReno"));

    uint32_t  clientN = clients.GetN();
    ApplicationContainer clientApps;


    uint16_t port = 10;
    Ptr<OutputStreamWrapper> appsAssign = ascii.CreateFileStream(queueDiscType + "-apps-assign.txt");

    //sets the number of priorities
    size_t type = 0;
    
    if (!randomPriority)
    {
        numberOfPrios = 0;
        for (size_t i = 0; i < Types.size(); ++i)
        {
            if (Types[i].Priority > numberOfPrios)
                numberOfPrios = Types[i].Priority;
        }
        ++numberOfPrios;
    }
    
    Ptr<UniformRandomVariable> prioRandom = CreateObject<UniformRandomVariable>();
    prioRandom->SetAttribute("Min", DoubleValue(0));
    prioRandom->SetAttribute("Max", DoubleValue(numberOfPrios - 1));

    ApplicationContainer upSinks;
    ApplicationContainer downSinks;

    std::map<std::pair<Ipv4Address, Ipv4Address>, int> flowTypes;
    std::map<std::pair<Ipv4Address, Ipv4Address>, Ptr<PacketSink> > flowSinks;
    std::vector<int> flowPrio (port+1);
    
    for (size_t i = 0; i < clientN; ++i)
    {
        ++port;
        uint8_t prio;
        if (randomPriority)
            prio = prioRandom->GetInteger();
        else
            prio = Types[type].Priority;
        flowPrio.push_back(prio);
        
        Ptr<PacketSink> s = SetupOnOff(clientApps, Types[type].OnTime, Types[type].OffTime, prio << 2, Types[type].IsTCP,
                                s1.Get(0), clients.Get(i), clientInterfaces.GetAddress(i), port, Types[type].PacketSize,
                                Types[type].DataRate, queueDiscType, samplingPeriod, stopTime,
                                std::to_string(i) + "-prio" + std::to_string(prio) + "-down");
        downSinks.Add(s);
        flowTypes[std::make_pair(interfacesS1.GetAddress(0), clientInterfaces.GetAddress(i))] = type + 1;
        flowTypes[std::make_pair(clientInterfaces.GetAddress(i), interfacesS1.GetAddress(0))] = -(type + 1);
        flowSinks[std::make_pair(interfacesS1.GetAddress(0), clientInterfaces.GetAddress(i))] = s;
        
        
        if (Types[type].IsBi)
        {
            ++port;
            s = SetupOnOff(clientApps, Types[type].OnTime, Types[type].OffTime, prio << 2, Types[type].IsTCP,
                                    clients.Get(i), s1.Get(0), interfacesS1.GetAddress(0), port, Types[type].PacketSize,
                                    Types[type].DataRate, queueDiscType, samplingPeriod, stopTime,
                                    std::to_string(i) + "-prio" + std::to_string(prio) + "-up");
            flowSinks[std::make_pair(clientInterfaces.GetAddress(i), interfacesS1.GetAddress(0))] = s;
            flowPrio.push_back(prio);
            upSinks.Add(s);
        }

        *appsAssign->GetStream() << i << " " << Names::FindName(clients.Get(i)) << " prio " << (int)prio << " " << type << " " << Types[type].Name << "\n";
        if (++type == Types.size())
        {
            type = 0;
        }
    }
    (*appsAssign->GetStream()).flush();
    Simulator::Schedule(Seconds(0), &TimeBar, 0.2);

    Ptr<OutputStreamWrapper> totalGoodputDownStream = ascii.CreateFileStream(queueDiscType + "-total-godput-down" + ".txt");
    Simulator::Schedule(Seconds(samplingPeriod), &GoodputSampling, downSinks, totalGoodputDownStream, samplingPeriod, Seconds(0), 0);

    Ptr<OutputStreamWrapper> totalGoodputUpStream = ascii.CreateFileStream(queueDiscType + "-total-godput-up" + ".txt");
    Simulator::Schedule(Seconds(samplingPeriod), &GoodputSampling, upSinks, totalGoodputUpStream, samplingPeriod, Seconds(0), 0);

    clientApps.Start(Seconds(0 + 0.1));
    clientApps.Stop(Seconds(stopTime - 0.1));

    // Flow monitor
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();
    
    Simulator::Stop(Seconds(stopTime));
    Simulator::Run();

    flowMonitor->SerializeToXmlFile(queueDiscType + "-flowMonitor.xml", false, true);

    flowMonitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();

    WriteStats(true, stats, classifier, flowTypes, flowSinks, flowPrio, queueDiscType, numberOfPrios, simDuration);
    WriteStats(false, stats, classifier, flowTypes, flowSinks, flowPrio, queueDiscType, numberOfPrios, simDuration);
    Simulator::Destroy();
    return 0;
}
