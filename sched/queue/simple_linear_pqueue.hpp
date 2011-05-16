
#ifndef SCHED_QUEUE_SIMPLE_LINEAR_PQUEUE_HPP
#define SCHED_QUEUE_SIMPLE_LINEAR_PQUEUE_HPP

#include "sched/queue/unsafe_queue_count.hpp"

// simple linear bounded priority queue
// not thread safe

namespace sched
{

template <class T>
class simple_linear_pqueue
{
private:
   
   typedef unsafe_queue_count<T> queue;
   
   std::vector<queue> queues;
   size_t total;
   
public:
   
   inline const bool empty(void) const { return total == 0; }
   inline const size_t size(void) const { return total; }
   inline queue& get_queue(const size_t prio) { return queues[prio]; }
   
   void push(const T item, const size_t prio)
   {
      queues[prio].push(item);
      ++total;
   }
   
   // no pop yet!
   
   void clear(void)
   {
      assert(total > 0);
      
      for(size_t i(0); i < queues.size(); ++i)
         get_queue(i).clear();
      total = 0;
   }
   
   explicit simple_linear_pqueue(const size_t range):
      total(0)
   {
      queues.resize(range);
   }
   
   ~simple_linear_pqueue(void)
   {
      assert(empty());
      assert(size() == 0);
   }
};

};

#endif