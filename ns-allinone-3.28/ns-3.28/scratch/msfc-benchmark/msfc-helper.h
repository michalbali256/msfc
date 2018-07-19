/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef MSFC_HELPER_H
#define MSFC_HELPER_H

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

using namespace ns3;

typedef std::pair<Ipv4Address, Ipv4Address> AddressPair;

const uint32_t MAX_PRIOS = 10;


class FlowType
{

  public:
    std::string Name = "";
    std::string DataRate = "";
    bool IsTCP = true;
    uint32_t AppCount;    
    bool IsBi = true;
    uint32_t PacketSize;
    std::string OnTime;
    std::string OffTime;
    uint32_t Priority;
    FlowType(std::string name, bool tcp, uint32_t appCount, std::string dataRate, bool bi, uint32_t packetSize, std::string onTime, std::string offTime, uint32_t priority)
        : Name(name), DataRate(dataRate), IsTCP(tcp), AppCount(appCount), IsBi(bi), PacketSize(packetSize), OnTime(onTime), OffTime(offTime), Priority(priority)
    {
    }

    //static
    static void LoadTypes(std::string flowsInFileName);
};

std::vector<FlowType> Types;

//counts statistics for one type of flow
struct TypeStats
{
  public:
    TypeStats() : Throughput(0), Loss(0), PacketCount(0), FlowCount(0), Goodput(0), AppCount(0), PrioCount()
    {
    }
    Time Delay;
    Time Jitter;
    int64_t Throughput;
    int64_t Loss;
    int64_t PacketCount;
    int64_t FlowCount;
    int64_t Goodput;
    int64_t AppCount;
    int64_t PrioCount[MAX_PRIOS];
    Ptr<OutputStreamWrapper> DelayStream;
    Ptr<OutputStreamWrapper> JitterStream;
    
};

//represents statistics of one quality of service, e.g. delay, loss, ...
template <typename T>
struct MeasurementStats
{
  public:
    MeasurementStats() : Prios(), Overall(0)
    {
        for (size_t i = 0; i < MAX_PRIOS; i++)
        {
            PriosCount[i] = 0;
        }
    }
    T Prios[MAX_PRIOS];
    int64_t PriosCount[MAX_PRIOS];
    Ptr<OutputStreamWrapper> PriosStream[MAX_PRIOS];
    T Overall;
    Ptr<OutputStreamWrapper> OverallStream;
};


inline QueueDiscContainer
InstallQDisc(Ptr<NetDevice> device, std::string queueDiscType, int queueDiscSize, int msfcMultiplier)
{
    TrafficControlHelper tchBottleneck;
    tchBottleneck.Uninstall(device);
    if (queueDiscType.compare("PfifoFast") == 0)
    {
        tchBottleneck.SetRootQueueDisc("ns3::PfifoFastQueueDisc", "MaxSize", QueueSizeValue(QueueSize(QueueSizeUnit::PACKETS, queueDiscSize)));
        for(int i = 0; i < 16; ++i)
            PfifoFastQueueDisc::prio2band[i] = 0;
    }
    else if (queueDiscType.compare("CoDel") == 0)
    {
        tchBottleneck.SetRootQueueDisc("ns3::CoDelQueueDisc", "MaxSize", QueueSizeValue(QueueSize(QueueSizeUnit::PACKETS, queueDiscSize * 50)));
    }
    else if (queueDiscType.compare("FqCoDel") == 0)
    {
        uint32_t handle = tchBottleneck.SetRootQueueDisc("ns3::FqCoDelQueueDisc", "MaxSize", QueueSizeValue(QueueSize(QueueSizeUnit::PACKETS, queueDiscSize*1024)));
        tchBottleneck.AddPacketFilter(handle, "ns3::FqCoDelIpv4PacketFilter");
        tchBottleneck.AddPacketFilter(handle, "ns3::FqCoDelIpv6PacketFilter");
    }
    else if (queueDiscType.compare("Msfc") == 0)
    {

        uint32_t handle = tchBottleneck.SetRootQueueDisc("ns3::MsfcQueueDisc", "MaxSize", QueueSizeValue(QueueSize(QueueSizeUnit::PACKETS, queueDiscSize*1024)));

        Config::SetDefault("ns3::MsfcQueueDisc::QuantumMultiplier", UintegerValue(msfcMultiplier));
        tchBottleneck.AddPacketFilter(handle, "ns3::FqCoDelIpv4PacketFilter");
        tchBottleneck.AddPacketFilter(handle, "ns3::FqCoDelIpv6PacketFilter");
    }
    else if (queueDiscType.compare("Sfq") == 0)
    {

        uint32_t handle = tchBottleneck.SetRootQueueDisc("ns3::SfqQueueDisc", "MaxSize", QueueSizeValue(QueueSize(QueueSizeUnit::PACKETS, queueDiscSize * 1024)));

        tchBottleneck.AddPacketFilter(handle, "ns3::SfqIpv4PacketFilter");
    }
    else
    {
        NS_ABORT_MSG("--queueDiscType not valid");
    }
    return tchBottleneck.Install(device);
}


void
WriteStats(bool down, const std::map<FlowId, FlowMonitor::FlowStats> & stats, Ptr<Ipv4FlowClassifier> & classifier, std::vector< int> & flowTypes, std::vector< Ptr<PacketSink> > & flowSinks, std::vector<int> & flowPrios, const std::string & queueDiscType, size_t numberOfPrios, int simDuration)
{   
    std::string du = down ? "down" : "up";
    AsciiTraceHelper ascii;
    
    //statistics and strams initialization

    std::map<uint32_t, TypeStats> typeStats;
    MeasurementStats<Time> delay;
    MeasurementStats<Time> jitter;
    MeasurementStats<int64_t> throughput;
    MeasurementStats<int64_t> loss;
    
    for(size_t i = 0; i < Types.size(); ++i)
    {
        typeStats[i].DelayStream = ascii.CreateFileStream(queueDiscType + "-" + "type" + std::to_string(i) + "-" + "delay" + "-" + du + ".det");
        typeStats[i].JitterStream = ascii.CreateFileStream(queueDiscType + "-" + "type" + std::to_string(i) + "-" + "jitter" + "-" + du + ".det");
    }
    delay.OverallStream = ascii.CreateFileStream(queueDiscType + "-" + "overall" + "-" + "delay" + "-" + du + ".det");
    jitter.OverallStream = ascii.CreateFileStream(queueDiscType + "-" + "overall" + "-" + "jitter" + "-" + du + ".det");
    
    for(size_t i = 0; i < numberOfPrios; ++i)
    {
        delay.PriosStream[i] = ascii.CreateFileStream(queueDiscType + "-" + "delay" + "-" + "prio" + std::to_string(i) + "-" + du + ".det");
        jitter.PriosStream[i] = ascii.CreateFileStream(queueDiscType + "-" + "jitter" + "-" + "prio" + std::to_string(i) + "-" + du + ".det");
    }

    int64_t packetCount = 0;
    int64_t prioPacketCount[MAX_PRIOS]{};
    int64_t prioFlowCount[MAX_PRIOS]{};
    
    //iterates through all flows and counts the QoS of individual types of flows and overall statistics
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);
        unsigned int prio;
        int type;
        uint16_t port;
        if(flowPrios.size() < t.destinationPort)
            continue;
        else
            port = t.destinationPort;

        prio = flowPrios[port];
        type = flowTypes[port];
        
        //filters out the ACK flows
        if ((type <= 0 && down) || (type >= 0 && !down) || type == 0)
        {
            if(type == 0)
                std::cout << "WRONG";
            continue;
        }
        
        if (type < 0)
            type = -type;
        --type; 
        
        auto stats = iter->second;
        
        typeStats[type].Delay += stats.delaySum;
        stats.delayHistogram.SerializeToXmlStream(*(typeStats[type].DelayStream->GetStream()), 0, "type" + std::to_string(type));
        typeStats[type].Jitter += stats.jitterSum;
        stats.jitterHistogram.SerializeToXmlStream(*(typeStats[type].JitterStream->GetStream()), 0, "type" + std::to_string(type));
        typeStats[type].Throughput += stats.rxBytes;
        typeStats[type].Loss += stats.lostPackets;
        typeStats[type].PacketCount += stats.rxPackets;
        ++typeStats[type].PrioCount[prio];
        if(flowSinks[port] != 0)
        {
            
            typeStats[type].Goodput += flowSinks[port]->GetTotalRx();
            ++typeStats[type].AppCount;
            //std::cout << type << " " << (double) flowSinks[std::make_pair(t.sourceAddress, t.destinationAddress)]->GetTotalRx() << "\n";
        }
        
        ++typeStats[type].FlowCount;
        delay.Overall += stats.delaySum;
        stats.delayHistogram.SerializeToXmlStream(*(delay.OverallStream->GetStream()), 0, "overall");
        jitter.Overall += stats.jitterSum;
        stats.jitterHistogram.SerializeToXmlStream(*(jitter.OverallStream->GetStream()), 0, "overall");
        throughput.Overall += stats.rxBytes;
        loss.Overall += stats.lostPackets;

        delay.Prios[prio] += stats.delaySum;
        stats.delayHistogram.SerializeToXmlStream(*(delay.PriosStream[prio]->GetStream()), 0, "prio" + std::to_string(prio));
        jitter.Prios[prio] += stats.jitterSum;
        stats.jitterHistogram.SerializeToXmlStream(*(jitter.PriosStream[prio]->GetStream()), 0, "prio" + std::to_string(prio));
        throughput.Prios[prio] += stats.rxBytes;
        loss.Prios[prio] += stats.lostPackets;


        prioPacketCount[prio] += stats.rxPackets;
        ++prioFlowCount[prio];
        packetCount += stats.rxPackets;

    }
    
    
    
    Ptr<OutputStreamWrapper> allStream = ascii.CreateFileStream(queueDiscType + "-" + du + ".all");
    
    *allStream->GetStream() << "FlowCount " << "\n";
    for (size_t i = 0; i < numberOfPrios; ++i)
    {
        *allStream->GetStream() << "    prio" << i << " " << prioFlowCount[i] << "\n";
    }
    
    *allStream->GetStream() << "Delay " << (packetCount != 0 ? (delay.Overall / packetCount).GetMilliSeconds() : -1) << "\n";
    for (size_t i = 0; i < numberOfPrios; ++i)
    {
        *allStream->GetStream() << "    prio" << i << " " << (prioPacketCount[i] != 0 ? (delay.Prios[i] / prioPacketCount[i]).GetMilliSeconds() : -1) << "\n";
    }

    *allStream->GetStream() << "Jitter " << (packetCount != 0 ? (jitter.Overall / packetCount).GetMilliSeconds() : -1) << "\n";
    for (size_t i = 0; i < numberOfPrios; ++i)
    {
        *allStream->GetStream() << "    prio" << i << " " << (prioPacketCount[i] != 0 ? (jitter.Prios[i] / prioPacketCount[i]).GetMilliSeconds() : -1) << "\n";
    }

    *allStream->GetStream() << "Throughput " << throughput.Overall * 8. / simDuration / 1024 << "\n";
    for (size_t i = 0; i < numberOfPrios; ++i)
    {
        *allStream->GetStream() << "    prio" << i << " " << throughput.Prios[i] * 8. / simDuration / 1024 << "\n";
    }

    *allStream->GetStream() << "Loss " << loss.Overall << "\n";
    for (size_t i = 0; i < numberOfPrios; ++i)
    {
        *allStream->GetStream() << "    prio" << i << " " << loss.Prios[i] << "\n";
    }
    
    for (size_t t = 0; t < Types.size(); ++t)
    {
        *allStream->GetStream() << Types[t].Name << "\n";
        if(typeStats[t].PacketCount != 0)
        {
            *allStream->GetStream() << "    Delay " << (typeStats[t].Delay / typeStats[t].PacketCount).GetMilliSeconds() << "\n";
            *allStream->GetStream() << "    Jitter " << (typeStats[t].Jitter / typeStats[t].PacketCount).GetMilliSeconds() << "\n";
        }
        else
            *allStream->GetStream() << "    Delay" << "\n" << "    Jitter" << "\n";
        *allStream->GetStream() << "    TotalThroughput " << typeStats[t].Throughput * 8. / simDuration / 1024 << "\n";
        if(typeStats[t].FlowCount != 0)
        {
            *allStream->GetStream() << "    PerFlowThroughput " << typeStats[t].Throughput * 8. / typeStats[t].FlowCount / simDuration / 1024 << "\n";
            *allStream->GetStream() << "    PerAppGoodput " << (double) typeStats[t].Goodput * 8. / typeStats[t].AppCount / simDuration / 1024 << "\n";
            
        }
        else
        {
            *allStream->GetStream() << "    PerFlowThroughput " << "\n" << "    PerAppGoodput " << "\n";
        }
        *allStream->GetStream() << "    Loss " << typeStats[t].Loss << "\n";
        *allStream->GetStream() << "    FlowCount " << typeStats[t].FlowCount << "\n";
        for (size_t i = 0; i < numberOfPrios; ++i)
        {
            *allStream->GetStream() << "        prio" << i << " " << typeStats[t].PrioCount[i] << "\n";
        }
        *allStream->GetStream() << "    AppCount " << typeStats[t].AppCount << "\n";
        *allStream->GetStream() << "    PacketCount " << typeStats[t].PacketCount << "\n";
    }
}

Ptr<PacketSink>
SetupOnOff(ApplicationContainer &clientApps, std::string onTime, std::string offTime, uint8_t priority, bool tcp, Ptr<Node> source, Ptr<Node> sink, Ipv4Address remoteAddress, unsigned int port, int packetSize, std::string clientDatarate, std::string queueDiscType, float samplingPeriod, float stopTime, std::string streamName)
{

    AsciiTraceHelper ascii;
    ApplicationContainer s1Sink;
    Address addS1(InetSocketAddress(Ipv4Address::GetAny(), port));

    PacketSinkHelper sinkHelperS1("ns3::TcpSocketFactory", addS1);

    if (tcp)
        sinkHelperS1.SetAttribute("Protocol", TypeIdValue(TcpSocketFactory::GetTypeId()));
    else
        sinkHelperS1.SetAttribute("Protocol", TypeIdValue(UdpSocketFactory::GetTypeId()));
    s1Sink.Add(sinkHelperS1.Install(sink));

    s1Sink.Start(Seconds(0));
    s1Sink.Stop(Seconds(stopTime));

    //application
    InetSocketAddress socketAddress = InetSocketAddress(remoteAddress, port);
    socketAddress.SetTos(priority);

    OnOffHelper onOffHelper("ns3::TcpSocketFactory", Address());
    if (tcp)
        onOffHelper.SetAttribute("Protocol", TypeIdValue(TcpSocketFactory::GetTypeId()));
    else
        onOffHelper.SetAttribute("Protocol", TypeIdValue(UdpSocketFactory::GetTypeId()));

    onOffHelper.SetAttribute("Remote", AddressValue(socketAddress));
    onOffHelper.SetAttribute("OnTime", StringValue(onTime));
    onOffHelper.SetAttribute("OffTime", StringValue(offTime));
    onOffHelper.SetAttribute("PacketSize", UintegerValue(packetSize));
    onOffHelper.SetAttribute("DataRate", StringValue(clientDatarate));

    ApplicationContainer app = onOffHelper.Install(source);
    auto socket = DynamicCast<OnOffApplication>(app.Get(0))->GetSocket();

    clientApps.Add(app);
    
    return DynamicCast<PacketSink> (s1Sink.Get(0));
}

void FlowType::LoadTypes(std::string flowsInFileName)
{
    using namespace std;
    ifstream flin(flowsInFileName);

    std::string line;
    while (std::getline(flin, line)) 
    {
        std::istringstream stream(line); 

        string name, dataRate, onTime, offTime;
        string bi, tcp;
        uint32_t packetSize, priority, appCount;
        stream >> name >> tcp >> appCount >> dataRate >> priority >> bi >> packetSize >> onTime >> offTime;

        Types.emplace_back(name, tcp == "TCP", appCount, dataRate, bi == "bi", packetSize, onTime, offTime, priority);
    }
}

#endif