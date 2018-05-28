/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 Universita' degli Studi di Napoli Federico II
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA    02111-1307    USA
 *
 * Authors: Pasquale Imputato <p.imputato@gmail.com>
 *                    Stefano Avallone <stefano.avallone@unina.it>
*/

#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/queue.h"
#include "msfc-queue-disc.h"
#include "codel-queue-disc.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-queue-disc-item.h"
#include <cmath>
namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MsfcQueueDisc");

NS_OBJECT_ENSURE_REGISTERED(MsfcFlow);

TypeId MsfcFlow::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::MsfcFlow")
        .SetParent<QueueDiscClass>()
        .SetGroupName("TrafficControl")
        .AddConstructor<MsfcFlow>();
    return tid;
}

MsfcFlow::MsfcFlow()
        : m_deficit(0),
            m_active(false)
{
    NS_LOG_FUNCTION(this);
}

MsfcFlow::~MsfcFlow()
{
    NS_LOG_FUNCTION(this);
}

void MsfcFlow::SetDeficit(uint32_t deficit)
{
    NS_LOG_FUNCTION(this << deficit);
    m_deficit = deficit;
}

int32_t
MsfcFlow::GetDeficit(void) const
{
    NS_LOG_FUNCTION(this);
    return m_deficit;
}

void MsfcFlow::IncreaseDeficit(int32_t deficit)
{
    NS_LOG_FUNCTION(this << deficit);
    m_deficit += deficit;
}

void MsfcFlow::SetActive(bool status)
{
    NS_LOG_FUNCTION(this);
    m_active = status;
}

bool
MsfcFlow::IsActive(void) const
{
    NS_LOG_FUNCTION(this);
    return m_active;
}



NS_OBJECT_ENSURE_REGISTERED(MsfcPrioClass);

TypeId MsfcPrioClass::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::MsfcPrioClass")
        .SetParent<Object>()
        .SetGroupName("TrafficControl")
        .AddConstructor<MsfcPrioClass>();
    return tid;
}

MsfcPrioClass::MsfcPrioClass():m_active(false)
{

}

MsfcPrioClass::~MsfcPrioClass()
{
    NS_LOG_FUNCTION(this);
}

void
MsfcPrioClass::SetActive(bool status)
{
    NS_LOG_FUNCTION(this);
    m_active = status;
}

bool
MsfcPrioClass::IsActive(void) const
{
    NS_LOG_FUNCTION(this);
    return m_active;
}

void 
MsfcPrioClass::SetQuantum(uint32_t quantum)
{
    NS_LOG_FUNCTION(this);
    m_quantum = quantum;
}

uint32_t
MsfcPrioClass::GetQuantum()
{
    NS_LOG_FUNCTION(this);
    return m_quantum;
}

void MsfcPrioClass::SetDeficit(uint32_t deficit)
{
    NS_LOG_FUNCTION(this << deficit);
    m_deficit = deficit;
}

int32_t
MsfcPrioClass::GetDeficit(void) const
{
    NS_LOG_FUNCTION(this);
    return m_deficit;
}

void MsfcPrioClass::IncreaseDeficit(int32_t deficit)
{
    NS_LOG_FUNCTION(this);
    m_deficit += deficit;
}

NS_OBJECT_ENSURE_REGISTERED(MsfcQueueDisc);

TypeId MsfcQueueDisc::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::MsfcQueueDisc")
        .SetParent<QueueDisc>()
        .SetGroupName("TrafficControl")
        .AddConstructor<MsfcQueueDisc>()
        .AddAttribute("Interval",
                        "The CoDel algorithm interval for each FQCoDel queue",
                        StringValue("100ms"),
                        MakeStringAccessor(&MsfcQueueDisc::m_interval),
                        MakeStringChecker())
        .AddAttribute("Target",
                        "The CoDel algorithm target queue delay for each FQCoDel queue",
                        StringValue("5ms"),
                        MakeStringAccessor(&MsfcQueueDisc::m_target),
                        MakeStringChecker())
        .AddAttribute ("MaxSize",
                        "The maximum number of packets accepted by this queue disc",
                        QueueSizeValue (QueueSize ("0p")),
                        MakeQueueSizeAccessor (&QueueDisc::SetMaxSize,
                                                &QueueDisc::GetMaxSize),
                        MakeQueueSizeChecker ())
        .AddAttribute("Flows",
                        "The number of queues into which the incoming packets are classified",
                        UintegerValue(1024),
                        MakeUintegerAccessor(&MsfcQueueDisc::m_flows),
                        MakeUintegerChecker<uint32_t>())
        .AddAttribute("DropBatchSize",
                        "The maximum number of packets dropped from the fat flow",
                        UintegerValue(64),
                        MakeUintegerAccessor(&MsfcQueueDisc::m_dropBatchSize),
                        MakeUintegerChecker<uint32_t>())
        .AddAttribute("QuantumMultiplier",
                        "the bandwidth allocation difference between two sequential prioclasses",
                        UintegerValue(2000),
                        MakeUintegerAccessor(&MsfcQueueDisc::m_quantum_multiplier),
                        MakeUintegerChecker<uint32_t>())
        .AddAttribute("Quantum",
                        "initial quantum value",
                        UintegerValue(0),
                        MakeUintegerAccessor(&MsfcQueueDisc::m_quantum),
                        MakeUintegerChecker<uint32_t>());
    return tid;
}


MsfcQueueDisc::MsfcQueueDisc()
        : QueueDisc(QueueDiscSizePolicy::MULTIPLE_QUEUES, QueueSizeUnit::PACKETS), m_quantum(0)
{

    NS_LOG_FUNCTION(this);
}

MsfcQueueDisc::~MsfcQueueDisc()
{
    NS_LOG_FUNCTION(this);
}

void MsfcQueueDisc::SetQuantum(uint32_t quantum)
{
    NS_LOG_FUNCTION(this << quantum);
    m_quantum = quantum;
}

uint32_t
MsfcQueueDisc::GetQuantum(void) const
{
    return m_quantum;
}

uint32_t
MsfcQueueDisc::ClassifyPriority(Ptr<QueueDiscItem> item)
{
    NS_LOG_FUNCTION(this << item);
    Ptr<Ipv4QueueDiscItem> ipv4Item = DynamicCast<Ipv4QueueDiscItem> (item);
   
    NS_ASSERT (ipv4Item != 0);
   
    Ipv4Header hdr = ipv4Item->GetHeader ();
    NS_LOG_DEBUG(hdr);
    return hdr.GetDscp();
}

bool MsfcQueueDisc::DoEnqueue(Ptr<QueueDiscItem> item)
{
    NS_LOG_FUNCTION(this << item);

    int32_t ret = Classify(item);

    uint32_t prio = ClassifyPriority(item);

    if (ret == PacketFilter::PF_NO_MATCH)
    {
        NS_LOG_ERROR("No filter has been able to classify this packet, drop it.");
        DropBeforeEnqueue(item, UNCLASSIFIED_DROP);
        return false;
    }

    uint32_t h = ret % m_flows;

    Ptr<MsfcPrioClass> pclass;
    Ptr<MsfcFlow> flow;

    auto find = m_prioClassMap.find(prio);
    if (find == m_prioClassMap.end())
    {
        NS_LOG_DEBUG("Creating a new prio class with priority " << prio);
        pclass = m_prioClassFactory.Create<MsfcPrioClass>();
        m_prioClassMap[prio] = pclass;

        pclass->SetQuantum(m_quantum * pow(m_quantum_multiplier / 1000, prio));
    }
    else
        pclass = find->second;

    auto findFlow = pclass->m_flowsMap.find(h);
    if (findFlow == pclass->m_flowsMap.end())
    {
        NS_LOG_DEBUG("Creating a new flow queue with index " << h);
        flow = m_flowFactory.Create<MsfcFlow>();
        Ptr<QueueDisc> qd = m_queueDiscFactory.Create<QueueDisc>();
        qd->Initialize();
        flow->SetQueueDisc(qd);
        AddQueueDiscClass(flow);

        pclass->m_flowsMap[h] = flow;
    }
    else
    {
        flow = findFlow->second;
    }

    if(!pclass->IsActive())
    {
        pclass->SetActive(true);
        pclass->SetDeficit(pclass->GetQuantum());//////
        m_prioClasses.push_front(pclass);
        //m_prioClasses.push_back(pclass);
    }

    if (!flow->IsActive())
    {
        flow->SetActive(true);
        flow->SetDeficit(m_quantum);
        pclass->m_flows.push_back(flow);
    }

    flow->GetQueueDisc()->Enqueue(item);

    NS_LOG_DEBUG("Packet enqueued into flow " << h << "; priority: " << prio);

    if (GetCurrentSize() > GetMaxSize())
    {
        //std::cout << GetCurrentSize() << "\n";
        MsfcDrop();
    }

    return true;
}

Ptr<QueueDiscItem>
MsfcQueueDisc::DoDequeue(void)
{
    NS_LOG_FUNCTION(this);

    Ptr<MsfcFlow> flow = 0;
    Ptr<QueueDiscItem> item = 0;
    Ptr<MsfcPrioClass> prioClass = 0;
    do
    {
        bool found = false;
        while (!found && !m_prioClasses.empty())
        {
            prioClass = m_prioClasses.front();

            if(prioClass->GetDeficit() <= 0)
            {
                prioClass->IncreaseDeficit(prioClass->GetQuantum());
                m_prioClasses.push_back(prioClass);
                m_prioClasses.pop_front();
            }
            else
            {
                found = true;
            }
        }

        if(!found)
        {
            //there are no prioclasses
            return 0;
        }
        
        found = false;
        while (!found && !prioClass->m_flows.empty())
        {
            flow = prioClass->m_flows.front();

            if (flow->GetDeficit() <= 0)
            {
                flow->IncreaseDeficit(m_quantum);
                prioClass->m_flows.push_back(flow);
                prioClass->m_flows.pop_front();
            }
            else
            {
                NS_LOG_DEBUG("Found a new flow with positive deficit");
                found = true;
            }
        }

        if(!found)
        {
            //prioclass is empty
            prioClass->SetActive(false);
            m_prioClasses.pop_front();

            continue;
        }

        item = flow->GetQueueDisc()->Dequeue();
        
        if(!item)
        {
            flow->SetActive(false);
            prioClass->m_flows.pop_front();
        }
        else
        {
            NS_LOG_DEBUG("Dequeued packet " << item->GetPacket());
        }
    } while (item == 0);   

    flow->IncreaseDeficit(-item->GetSize());
    prioClass->IncreaseDeficit(-item->GetSize());

    return item;
}

Ptr<const QueueDiscItem>
MsfcQueueDisc::DoPeek(void)
{
    NS_LOG_FUNCTION(this);


    if (m_prioClasses.empty())
    {
        return 0;
    }
    else
    {
        if (m_prioClasses.front()->m_flows.empty())
        {
            return 0;
        }
        else
        {
            return m_prioClasses.front()->m_flows.front()->GetQueueDisc()->Peek();
        }
    }
}

bool MsfcQueueDisc::CheckConfig(void)
{
    NS_LOG_FUNCTION(this);
    if (GetNQueueDiscClasses() > 0)
    {
        NS_LOG_ERROR("MsfcQueueDisc cannot have classes");
        return false;
    }

    if (GetNPacketFilters() == 0)
    {
        NS_LOG_ERROR("MsfcQueueDisc needs at least a packet filter");
        return false;
    }

    if (GetNInternalQueues() > 0)
    {
        NS_LOG_ERROR("MsfcQueueDisc cannot have internal queues");
        return false;
    }

    return true;
}

void MsfcQueueDisc::InitializeParams(void)
{
    NS_LOG_FUNCTION(this);

    // we are at initialization time. If the user has not set a quantum value,
    // set the quantum to the MTU of the device
    if (!m_quantum)
    {
        Ptr<NetDevice> device = GetNetDevice();
        NS_ASSERT_MSG(device, "Device not set for the queue disc");
        m_quantum = device->GetMtu();
        NS_LOG_DEBUG("Setting the quantum to the MTU of the device: " << m_quantum);
    }

    m_flowFactory.SetTypeId("ns3::MsfcFlow");

    m_prioClassFactory.SetTypeId("ns3::MsfcPrioClass");

    m_queueDiscFactory.SetTypeId("ns3::CoDelQueueDisc");
    /*m_queueDiscFactory.Set("Mode", EnumValue(CoDelQueueDisc::QUEUE_DISC_MODE_PACKETS));
    m_queueDiscFactory.Set("MaxPackets", UintegerValue(m_limit + 1));*/
    std::cout << "MAXSIZE" << GetMaxSize() << "\n";
    m_queueDiscFactory.Set("MaxSize", QueueSizeValue(GetMaxSize()));
    m_queueDiscFactory.Set("Interval", StringValue(m_interval));
    m_queueDiscFactory.Set("Target", StringValue(m_target));
}

uint32_t
MsfcQueueDisc::MsfcDrop(void)
{
    NS_LOG_FUNCTION(this);

    uint32_t maxBacklog = 0, index = 0;
    Ptr<QueueDisc> qd;

    /* Queue is full! Find the fat flow and drop packet(s) from it */
    for (uint32_t i = 0; i < GetNQueueDiscClasses(); i++)
    {
        qd = GetQueueDiscClass(i)->GetQueueDisc();
        uint32_t bytes = qd->GetNBytes();
        if (bytes > maxBacklog)
        {
            maxBacklog = bytes;
            index = i;
        }
    }

    /* Our goal is to drop half of this fat flow backlog */
    uint32_t len = 0, count = 0, threshold = maxBacklog >> 1;
    qd = GetQueueDiscClass(index)->GetQueueDisc();
    Ptr<QueueDiscItem> item;

    do
    {
        item = qd->GetInternalQueue(0)->Dequeue();
        DropAfterDequeue(item, OVERLIMIT_DROP);
        len += item->GetSize();
    } while (++count < m_dropBatchSize && len < threshold);

    return index;
}

} // namespace ns3
