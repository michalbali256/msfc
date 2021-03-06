/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/queue.h"
#include "msfc-queue-disc.h"
#include "codel-queue-disc.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-queue-disc-item.h"
#include "ns3/ipv6-queue-disc-item.h"
#include <cmath>
namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MsfcQueueDisc");

NS_OBJECT_ENSURE_REGISTERED(MsfcFlow);

uint32_t DEFAULT_BACKLOG_MTU_PACKETS = 1000;

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
        .AddAttribute ("Backlog",
                        "The maximum number of bytes in a single CoDel flow",
                        UintegerValue(0),
                        MakeUintegerAccessor (&MsfcQueueDisc::m_backlog),
                        MakeUintegerChecker<uint32_t>() )
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
                        "The bandwidth allocation difference between two sequential prioclasses",
                        UintegerValue(2000),
                        MakeUintegerAccessor(&MsfcQueueDisc::m_quantum_multiplier),
                        MakeUintegerChecker<uint32_t>())
        .AddAttribute("Quantum",
                        "Initial quantum value",
                        UintegerValue(0),
                        MakeUintegerAccessor(&MsfcQueueDisc::m_quantum),
                        MakeUintegerChecker<uint32_t>())
        .AddAttribute("PriorityClassifier",
                        "Callback, that is used to determine priority of packets",
                        CallbackValue(MakeCallback(&MsfcQueueDisc::ClassifyPriority)),
                        MakeCallbackAccessor(&MsfcQueueDisc::m_priorityClassifier),
                        MakeCallbackChecker());
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
    NS_LOG_FUNCTION( item);
    Ptr<Ipv4QueueDiscItem> ipv4Item = DynamicCast<Ipv4QueueDiscItem> (item);
    if(ipv4Item != 0)
    {
        Ipv4Header hdr = ipv4Item->GetHeader ();
        NS_LOG_DEBUG(hdr);
        return hdr.GetDscp();
    }
    else
    {
        Ptr<Ipv6QueueDiscItem> ipv6Item = DynamicCast<Ipv6QueueDiscItem> (item);
        return ipv6Item->GetHeader().GetDscp();
    }
}

bool MsfcQueueDisc::DoEnqueue(Ptr<QueueDiscItem> item)
{
    NS_LOG_FUNCTION(this << item);

    int32_t ret = Classify(item); //hash

    uint32_t prio = m_priorityClassifier(item); //priority

    if (ret == PacketFilter::PF_NO_MATCH)
    {
        NS_LOG_ERROR("No filter has been able to classify this packet, drop it.");
        DropBeforeEnqueue(item, UNCLASSIFIED_DROP);
        return false;
    }

    uint32_t h = ret % m_flows;

    Ptr<MsfcPrioClass> pclass;
    Ptr<MsfcFlow> flow;

    //we determine the priority class of the packet and create it if needed
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

    //we determine the flow of the packet in the priority class and create it if needed
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
        //we enqueue the priority class into active prioclasses if it was not active before
        pclass->SetActive(true);
        if(pclass->GetDeficit() > (int32_t) pclass->GetQuantum())
            pclass->SetDeficit(pclass->GetQuantum());
        m_prioClasses.push_back(pclass);
    }

    //we enqueue the flow into active flows if it was not active before
    if (!flow->IsActive())
    {
        flow->SetActive(true);
        if(flow->GetDeficit() > (int32_t) m_quantum)
            flow->SetDeficit(m_quantum);
        pclass->m_flows.push_back(flow);
    }

    flow->GetQueueDisc()->Enqueue(item);

    NS_LOG_DEBUG("Packet enqueued into flow " << h << "; priority: " << prio);

    if (GetCurrentSize() > GetMaxSize())
    {
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

    //while cycle is needed, because codel may delete all 
    do //while
    {
        bool found = false;
        while (!found && !m_prioClasses.empty())
        {
            prioClass = m_prioClasses.front();

            if(prioClass->GetDeficit() <= 0)
            {
                //we have exhausted the credits the priority class had for this round
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
            //there are no prioclasses and thus no packets available
            return 0;
        }
        
        found = false;
        while (!found && !prioClass->m_flows.empty())
        {
            flow = prioClass->m_flows.front();

            if (flow->GetDeficit() <= 0)
            {
                //we have exhausted the credits the flo had for this round inside the prioclass
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
            //flow is empty, deactivate it
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
        m_backlog = DEFAULT_BACKLOG_MTU_PACKETS * m_quantum;
        NS_LOG_DEBUG("Setting the quantum to the MTU of the device: " << m_quantum);
    }

    m_flowFactory.SetTypeId("ns3::MsfcFlow");

    m_prioClassFactory.SetTypeId("ns3::MsfcPrioClass");

    m_queueDiscFactory.SetTypeId("ns3::CoDelQueueDisc");
    m_queueDiscFactory.Set("MaxSize", QueueSizeValue(std::to_string(m_backlog) + "B"));
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
