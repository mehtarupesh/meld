
#ifndef SCHED_NODES_THREAD_HPP
#define SCHED_NODES_THREAD_HPP

#include "mem/base.hpp"
#include "sched/nodes/in_queue.hpp"
#include "sched/queue/bounded_pqueue.hpp"
#include "db/tuple.hpp"
#include "utils/spinlock.hpp"
#include "sched/base.hpp"

namespace sched
{

// forward declarations
class static_local; 
class dynamic_local;
class threads_single;
class mpi_thread_dynamic;
class mpi_thread_static;
class mpi_thread_single;

class thread_node: public in_queue_node
{
private:
   
   friend class static_local;
   friend class dynamic_local;
   friend class threads_single;
   friend class mpi_thread_static;
   friend class mpi_thread_dynamic;
   friend class mpi_thread_single;
   
   sched::base *owner;
	utils::spinlock spin;
   safe_bounded_pqueue<node_work_unit>::type queue;
   
public:
   
   inline void set_owner(sched::base *_owner) { owner = _owner; }
   
   inline sched::base* get_owner(void) { return owner; }
   
   inline void add_work(const db::simple_tuple *tpl, const bool is_agg = false)
   {
      node_work_unit w = {tpl, is_agg};
      queue.push(w, tpl->get_strat_level());
   }
   
   inline bool has_work(void) const { return !queue.empty(); }
   
   inline node_work_unit get_work(void)
   {
      return queue.pop();
   }
   
   virtual void assert_end(void) const
   {
      in_queue_node::assert_end();
      assert(!has_work());
   }
   
   virtual void assert_end_iteration(void) const
   {
      in_queue_node::assert_end_iteration();
      assert(!has_work());
   }
   
   explicit thread_node(const db::node::node_id _id, const db::node::node_id _trans):
      in_queue_node(_id, _trans),
      owner(NULL),
      queue(vm::predicate::MAX_STRAT_LEVEL)
   {}
   
   virtual ~thread_node(void) { }
};

}

#endif