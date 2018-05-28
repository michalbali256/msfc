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

/*static void PingRtt ( Ptr<OutputStreamWrapper> stream, Time rtt)
{
  *stream->GetStream() << Simulator::Now().GetMilliSeconds() << " " << rtt.GetMilliSeconds () << "\n";
}*/

void RTTEst( Ptr<OutputStreamWrapper> stream, Time old, Time neu)
{
  *stream->GetStream() << Simulator::Now().GetMilliSeconds() << " " << neu.GetMilliSeconds() << "\n";
}

void RTTEstSampl (Ptr<OutputStreamWrapper> stream, Ptr<OnOffApplication> app)
{
    auto socket = app->GetSocket();

    socket->TraceConnectWithoutContext("RTT", MakeBoundCallback (&RTTEst, stream));
    
}

void TxPrepare (Ptr<DelayJitterEstimation> estimator, Ptr<const Packet > packet)
{
    estimator->PrepareTx(packet);
}

void RxRecord (Ptr<DelayJitterEstimation> estimator, Ptr<const Packet > packet, const Address &address)
{
    estimator->RecordRx(packet);
}

void DelayJitterSample(Ptr<OutputStreamWrapper> streamDelay, Ptr<OutputStreamWrapper> streamJitter,
     Ptr<DelayJitterEstimation> estimator, float period)
{
  Simulator::Schedule(Seconds(period), &DelayJitterSample, streamDelay, streamJitter, estimator, period);
  
  *streamDelay->GetStream() << Simulator::Now().GetMilliSeconds() << " " << estimator->GetLastDelay().GetMilliSeconds() << "\n";
  *streamJitter->GetStream() << Simulator::Now().GetMilliSeconds() << " " << estimator->GetLastJitter() << "\n";
}

void TimeBar(float period)
{
    std::cout << Simulator::Now().GetMilliSeconds() << "\n";
    Simulator::Schedule(Seconds(period), &TimeBar, period);
}


#endif