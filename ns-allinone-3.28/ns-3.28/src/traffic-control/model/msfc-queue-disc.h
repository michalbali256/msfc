/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef MSFC_QUEUE_DISC
#define MSFC_QUEUE_DISC

#include "ns3/queue-disc.h"
#include "ns3/object-factory.h"
#include <list>
#include <map>

namespace ns3 {

/**
 * \ingroup traffic-control
 *
 * \brief A flow queue used by the Msfc queue disc
 */

class MsfcFlow : public QueueDiscClass {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief MsfcFlow constructor
   */
  MsfcFlow ();

  virtual ~MsfcFlow ();


  /**
   * \brief Set the deficit for this flow
   * \param deficit the deficit for this flow
   */
  void SetDeficit (uint32_t deficit);
  /**
   * \brief Get the deficit for this flow
   * \return the deficit for this flow
   */
  int32_t GetDeficit (void) const;
  /**
   * \brief Increase the deficit for this flow
   * \param deficit the amount by which the deficit is to be increased
   */
  void IncreaseDeficit (int32_t deficit);
  /**
   * \brief Set the status for this flow
   * \param status the status for this flow
   */
  void SetActive (bool status);
  /**
   * \brief Get the status of this flow
   * \return the status of this flow
   */
  bool IsActive (void) const;
  

private:
  int32_t m_deficit;    //!< the deficit for this flow
  bool m_active;  //!< the status of this flow
};

class MsfcPrioClass : public Object
{
public:
  static TypeId GetTypeId (void);

  MsfcPrioClass(void);
  virtual ~MsfcPrioClass ();

  void SetDeficit (uint32_t deficit);

  int32_t GetDeficit (void) const;

  void IncreaseDeficit (int32_t deficit);
  
    void SetQuantum(uint32_t quantum);

  uint32_t GetQuantum();

  std::map<uint32_t, Ptr<MsfcFlow> > m_flowsMap;
  
  std::list<Ptr<MsfcFlow> > m_flows;    //!< list of flows

  void SetActive (bool status);
  /**
   * \brief Get the status of this flow
   * \return the status of this flow
   */
  bool IsActive (void) const;
private:
  int32_t m_deficit;
  uint32_t m_quantum;
  bool m_active;



};

/**
 * \ingroup traffic-control
 *
 * \brief A Msfc packet queue disc
 */

class MsfcQueueDisc : public QueueDisc {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief MsfcQueueDisc constructor
   */
  MsfcQueueDisc ();

  virtual ~MsfcQueueDisc ();

   /**
    * \brief Set the quantum value.
    *
    * \param quantum The number of bytes each queue gets to dequeue on each round of the scheduling algorithm
    */
   void SetQuantum (uint32_t quantum);

   /**
    * \brief Get the quantum value.
    *
    * \returns The number of bytes each queue gets to dequeue on each round of the scheduling algorithm
    */
   uint32_t GetQuantum (void) const;

  // Reasons for dropping packets
  static constexpr const char* UNCLASSIFIED_DROP = "Unclassified drop";  //!< No packet filter able to classify packet
  static constexpr const char* OVERLIMIT_DROP = "Overlimit drop";        //!< Overlimit dropped packets

private:
  virtual bool DoEnqueue (Ptr<QueueDiscItem> item);
  virtual Ptr<QueueDiscItem> DoDequeue (void);
  virtual Ptr<const QueueDiscItem> DoPeek (void);
  virtual bool CheckConfig (void);
  virtual void InitializeParams (void);
  static uint32_t ClassifyPriority(Ptr<QueueDiscItem> item);

  /**
   * \brief Drop a packet from the head of the queue with the largest current byte count
   * \return the index of the queue with the largest current byte count
   */
  uint32_t MsfcDrop (void);

  std::string m_interval;    //!< CoDel interval attribute
  std::string m_target;      //!< CoDel target attribute
  uint32_t m_limit;          //!< Maximum number of packets in the queue disc
  uint32_t m_quantum;        //!< Deficit assigned to flows at each round
  uint32_t m_flows;          //!< Number of flow queues
  uint32_t m_dropBatchSize;  //!< Max number of packets dropped from the fat flow
  uint32_t m_quantum_multiplier; //!< the bandwidth allocation difference between two sequential prioclasses
  uint32_t m_backlog;        //!< Maximum bytes in a single CoDel flow
  Callback<uint32_t, Ptr<QueueDiscItem> > m_priorityClassifier; //<! Function, that is used to determine priority of packets
  
  std::map< uint32_t, Ptr<MsfcPrioClass>> m_prioClassMap; 
  std::list<Ptr<MsfcPrioClass> > m_prioClasses;    //!< list of classes
  
  typedef std::pair<uint32_t, uint32_t> FlowsIndicesKey;

  std::map<FlowsIndicesKey, uint32_t> m_flowsIndices;    //!< Map with the index of class for each flow

  ObjectFactory m_flowFactory;         //!< Factory to create a new flow
  ObjectFactory m_queueDiscFactory;    //!< Factory to create a new queue
  ObjectFactory m_prioClassFactory;
};

} // namespace ns3

#endif /* FQ_CODEL_QUEUE_DISC */
