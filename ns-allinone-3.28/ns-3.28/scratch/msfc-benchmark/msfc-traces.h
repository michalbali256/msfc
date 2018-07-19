/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef MSFC_TRACES_H
#define MSFC_TRACES_H

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

using namespace ns3;


void BytesInQueueTrace(Ptr<OutputStreamWrapper> stream, uint32_t oldVal, uint32_t newVal)
{
    *stream->GetStream() << Simulator::Now().GetSeconds() << " " << newVal << std::endl;
}

static void
GoodputSampling(ApplicationContainer app, Ptr<OutputStreamWrapper> stream, float period,
                Time lastTime, uint64_t lastRx)
{

    double goodput;
    uint64_t totalBytes = 0;
    for (size_t i = 0; i < app.GetN(); ++i)
    {
        totalBytes += DynamicCast<PacketSink>(app.Get(i))->GetTotalRx();
    }
    goodput = (totalBytes - lastRx) * 8 / ((Simulator::Now() - lastTime).GetSeconds() * 1024); // Kbit/s

    if (app.GetN() > 1)
        std::cout << Simulator::Now().GetMilliSeconds() << goodput << "\n";

    Simulator::Schedule(Seconds(period), &GoodputSampling, app, stream, period, Simulator::Now(), totalBytes);
    *stream->GetStream() << Simulator::Now().GetSeconds() << " " << goodput << std::endl;
}

void TimeBar(float period)
{
    std::cout << Simulator::Now().GetMilliSeconds() << "\n";
    Simulator::Schedule(Seconds(period), &TimeBar, period);
}


#endif