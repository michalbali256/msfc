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

// This example serves as a benchmark for all the queue discs (with BQL enabled or not)
//
// Network topology
//
//                192.168.1.0                             192.168.2.0
// n1 ------------------------------------ n2 ----------------------------------- n3
//   point-to-point (access link)                point-to-point (bottleneck link)
//   100 Mbps, 0.1 ms                            bandwidth [10 Mbps], delay [5 ms]
//   qdiscs PfifoFast with capacity              qdiscs queueDiscType in {PfifoFast, ARED, CoDel, FqCoDel, PIE} [PfifoFast]
//   of 1000 packets                             with capacity of queueDiscSize packets [1000]
//   netdevices queues with size of 100 packets  netdevices queues with size of netdevicesQueueSize packets [100]
//   without BQL                                 bql BQL [false]
//   *** fixed configuration ***
//
//
// If you use an AQM as queue disc on the bottleneck netdevices, you can observe that the ping Rtt
// decrease. A further decrease can be observed when you enable BQL.

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


class FlowType
{

  public:
    std::string Name = "";
    std::string DataRate = "";
    bool IsTCP = true;
    bool IsBi = true;
    uint32_t PacketSize;
    std::string OnTime;
    std::string OffTime;
    uint32_t Priority;
    FlowType(std::string name, bool tcp, std::string dataRate, bool bi, uint32_t packetSize, std::string onTime, std::string offTime, uint32_t priority)
        : Name(name), DataRate(dataRate), IsTCP(tcp), IsBi(bi), PacketSize(packetSize), OnTime(onTime), OffTime(offTime), Priority(priority)
    {
    }

    //static
    static void LoadTypes(std::string flowsInFileName);
};

std::vector<FlowType> Types;

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
    int64_t PrioCount[10];
    
};

template <typename T>
struct MeasurementStats
{
  public:
    MeasurementStats() : Prios(), Overall(0)
    {
        for (size_t i = 0; i < 10; i++)
        {
            PriosCount[i] = 0;
        }
    }
    T Prios[10];
    int64_t PriosCount[10];
    T Overall;
    
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
    std::map<size_t, TypeStats> typeStats;
    MeasurementStats<Time> delay;
    MeasurementStats<Time> jitter;
    MeasurementStats<int64_t> throughput;
    MeasurementStats<int64_t> loss;

    int64_t packetCount = 0;
    int64_t prioPacketCount[10]{};
    int64_t prioFlowCount[10]{};
    
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
        std::cout << port << "\n";
        prio = flowPrios[port];
        type = flowTypes[port];
        
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
        typeStats[type].Jitter += stats.jitterSum;
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
        jitter.Overall += stats.jitterSum;
        throughput.Overall += stats.rxBytes;
        loss.Overall += stats.lostPackets;

        delay.Prios[prio] += stats.delaySum;
        jitter.Prios[prio] += stats.jitterSum;
        throughput.Prios[prio] += stats.rxBytes;
        loss.Prios[prio] += stats.lostPackets;


        prioPacketCount[prio] += stats.rxPackets;
        ++prioFlowCount[prio];
        packetCount += stats.rxPackets;

    }
    std::string du = down ? "down" : "up";
    
    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> allStream = ascii.CreateFileStream(queueDiscType + "-" + du + ".all");
    
    *allStream->GetStream() << "FlowCount " << "\n";
    for (size_t i = 0; i < numberOfPrios; ++i)
    {
        *allStream->GetStream() << "    prio" << i << " " << prioFlowCount[i] << "\n";
    }
    
    *allStream->GetStream() << "Delay " << (delay.Overall / packetCount).GetMilliSeconds() << "\n";
    for (size_t i = 0; i < numberOfPrios; ++i)
    {
        *allStream->GetStream() << "    prio" << i << " " << (delay.Prios[i] / prioPacketCount[i]).GetMilliSeconds() << "\n";
    }

    *allStream->GetStream() << "Jitter " << (jitter.Overall / packetCount).GetMilliSeconds() << "\n";
    for (size_t i = 0; i < numberOfPrios; ++i)
    {
        *allStream->GetStream() << "    prio" << i << " " << (jitter.Prios[i] / prioPacketCount[i]).GetMilliSeconds() << "\n";
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
    /*auto estimator = CreateObject<DelayJitterEstimation>();
    Ptr<OutputStreamWrapper> delayStream = ascii.CreateFileStream (queueDiscType + "-" + streamName + "-delay" + ".txt");
    Ptr<OutputStreamWrapper> jitterStream = ascii.CreateFileStream (queueDiscType + "-" + streamName + "-jitter" + ".txt");

    Simulator::Schedule (Seconds (samplingPeriod + 0.4), &DelayJitterSample, delayStream, jitterStream, estimator, 0.01);*/

    //sink
    ApplicationContainer s1Sink;
    Address addS1(InetSocketAddress(Ipv4Address::GetAny(), port));

    PacketSinkHelper sinkHelperS1("ns3::TcpSocketFactory", addS1);

    if (tcp)
        sinkHelperS1.SetAttribute("Protocol", TypeIdValue(TcpSocketFactory::GetTypeId()));
    else
        sinkHelperS1.SetAttribute("Protocol", TypeIdValue(UdpSocketFactory::GetTypeId()));
    s1Sink.Add(sinkHelperS1.Install(sink));

    //s1Sink.Get(0)->TraceConnectWithoutContext("Rx", MakeBoundCallback(&RxRecord, estimator));

    s1Sink.Start(Seconds(0));
    s1Sink.Stop(Seconds(stopTime));

    //Ptr<OutputStreamWrapper> goodputStream = ascii.CreateFileStream (queueDiscType + "-" + streamName + "-goodput" + ".txt");
    //Simulator::Schedule (Seconds (samplingPeriod), &GoodputSampling, s1Sink, goodputStream, samplingPeriod, Seconds(0), 0);

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

    //Ptr<OutputStreamWrapper> RTTeStream = ascii.CreateFileStream (queueDiscType+ "-" + streamName + "-RTTe" + ".txt");
    //Simulator::Schedule (Seconds (0.2), &RTTEstSampl, RTTeStream, DynamicCast<OnOffApplication> (app.Get(0)));

    //app.Get(0)->TraceConnectWithoutContext("Tx",MakeBoundCallback(&TxPrepare,estimator));

    /*4PingHelper ping = V4PingHelper(remoteAddress);
    
    ApplicationContainer pingContainer = ping.Install(source);
    Ptr<V4Ping> png = DynamicCast<V4Ping>(pingContainer.Get(0));
    Ptr<OutputStreamWrapper> RTTmStream = ascii.CreateFileStream (queueDiscType+ "-" + streamName + "-RTTm" + ".txt");
    png->TraceConnectWithoutContext("Rtt", MakeBoundCallback (&PingRtt, RTTmStream));*/
    
    return DynamicCast<PacketSink> (s1Sink.Get(0));
}

void FlowType::LoadTypes(std::string flowsInFileName)
{
    using namespace std;
    ifstream flin(flowsInFileName);

    while (!flin.eof()) // && i < 100)
    {
        string name, dataRate, onTime, offTime;
        string bi, tcp;
        uint32_t packetSize, priority;
        flin >> name >> tcp >> dataRate >> priority >> bi >> packetSize >> onTime >> offTime;
        //cout << name << tcp << dataRate << bi << packetSize << onTime << " " << offTime << "\n";
        Types.emplace_back(name, tcp == "TCP", dataRate, bi == "bi", packetSize, onTime, offTime, priority);
    }
}

#endif