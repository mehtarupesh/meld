
#include <iostream>
#include <boost/thread/barrier.hpp>

#include "sched/threads.hpp"

#include "vm/state.hpp"
#include "process/machine.hpp"
#include "sched/termination_barrier.hpp"

using namespace boost;
using namespace vm;
using namespace db;
using namespace process;
using namespace std;

//#define DEBUG_ACTIVE 1

namespace sched
{

static vector<threads_static*> others;
static barrier *thread_barrier(NULL);
static termination_barrier *term_barrier(NULL);

static inline void
threads_synchronize(void)
{
   thread_barrier->wait();
}
  
bool
threads_static::all_buffers_emptied(void) const
{
   for(process_id i(0); i < (process_id)buffered_work.size(); ++i) {
      if(!buffered_work[i].empty())
         return false;
   }
   return true;
}

void
threads_static::new_work(node *node, const simple_tuple *tpl, const bool is_agg)
{
   sstatic::new_work(node, tpl, is_agg);
   assert(process_state == PROCESS_ACTIVE);
}

void
threads_static::new_work_other(sched::base *scheduler, node *node, const simple_tuple *stuple)
{
   assert(process_state == PROCESS_ACTIVE);
   
   static const size_t WORK_THRESHOLD(20);
   
   threads_static *other((threads_static*)scheduler);
   work_unit work = {node, stuple, false};
   wqueue_free<work_unit>& q(buffered_work[other->id]);
   
   q.push(work);
   
   if(q.size() > WORK_THRESHOLD)
      flush_this_queue(q, other);
}

void
threads_static::new_work_remote(remote *, const vm::process_id, message *)
{
   assert(0);
}

void
threads_static::assert_end_iteration(void) const
{
   sstatic::assert_end_iteration();
   assert(process_state == PROCESS_INACTIVE);
   assert(all_buffers_emptied());
   assert(term_barrier->all_finished());
}

void
threads_static::assert_end(void) const
{
   sstatic::assert_end();
   assert(process_state == PROCESS_INACTIVE);
   assert(all_buffers_emptied());
   assert(term_barrier->all_finished());
}

void
threads_static::flush_this_queue(wqueue_free<work_unit>& q, threads_static *other)
{
   assert(this != other);
   assert(process_state == PROCESS_ACTIVE);
   
   {
      mutex::scoped_lock l(other->mutex);
      other->make_active();
      other->queue_work.snap(q);
   }
   
   q.clear();
}

void
threads_static::flush_buffered(void)
{
   for(process_id i(0); i < (process_id)buffered_work.size(); ++i) {
      if(i != id) {
         wqueue_free<work_unit>& q(buffered_work[i]);
         if(!q.empty()) {
            assert(process_state == PROCESS_ACTIVE);
            flush_this_queue(q, others[i]);
         }
      }
   }
}
   
bool
threads_static::busy_wait(void)
{
   bool turned_inactive(false);
   
   flush_buffered();
   
   while(queue_work.empty()) {
      
      if(!turned_inactive) {
         mutex::scoped_lock l(mutex);
         if(queue_work.empty() && process_state == PROCESS_ACTIVE) {
            make_inactive();
            turned_inactive = true;
            if(term_barrier->all_finished())
               return false;
         } else if(process_state == PROCESS_INACTIVE && queue_work.empty()) {
            turned_inactive = true;
         }
      }
      
      if(term_barrier->all_finished()) {
         assert(process_state == PROCESS_INACTIVE);
         return false;
      }
   }
   
   assert(process_state == PROCESS_ACTIVE);
   assert(!queue_work.empty());
   
   return true;
}

bool
threads_static::get_work(work_unit& work)
{
   return sstatic::get_work(work);
}

void
threads_static::make_active(void)
{
   if(process_state == PROCESS_INACTIVE) {
      term_barrier->is_active();
      process_state = PROCESS_ACTIVE;
#ifdef DEBUG_ACTIVE
      cout << "Active " << id << endl;
#endif
   }
}

void
threads_static::make_inactive(void)
{
   if(process_state == PROCESS_ACTIVE) {
      term_barrier->is_inactive();
      process_state = PROCESS_INACTIVE;
#ifdef DEBUG_ACTIVE
      cout << "Inactive: " << id << endl;
#endif
   }
}

void
threads_static::begin_get_work(void)
{
}

void
threads_static::work_found(void)
{
   assert(process_state == PROCESS_ACTIVE);
   assert(!queue_work.empty());
}

void
threads_static::init(const size_t num_threads)
{
   sstatic::init(num_threads);
   
   // init buffered queues
   buffered_work.resize(num_threads);
   
   assert(process_state == PROCESS_ACTIVE);
}

void
threads_static::end(void)
{
   sstatic::end();
   assert(process_state == PROCESS_INACTIVE);
}

bool
threads_static::terminate_iteration(void)
{
   // this is needed since one thread can reach make_active
   // and thus other threads waiting for all_finished will fail
   // to get here
   threads_synchronize();
   
   sstatic::terminate_iteration();
   
   assert(all_buffers_emptied());
   assert(process_state == PROCESS_INACTIVE);
   
   generate_aggs();
   
   if(!queue_work.empty()) {
      make_active();
   }
   
   // again, needed since we must wait if any thread
   // is set to active in the previous if
   threads_synchronize();
   
   return !term_barrier->all_finished();
}

threads_static*
threads_static::find_scheduler(const node::node_id id)
{
   return others[remote::self->find_proc_owner(id)];
}

threads_static::threads_static(const process_id _id):
   sstatic(_id),
   process_state(PROCESS_ACTIVE)
{
}

threads_static::~threads_static(void)
{
}

vector<threads_static*>&
threads_static::start(const size_t num_threads)
{
   thread_barrier = new barrier(num_threads);
   term_barrier = new termination_barrier(num_threads);
   others.resize(num_threads);
   
   for(process_id i(0); i < num_threads; ++i)
      others[i] = new threads_static(i);
      
   return others;
}

}