
#include "sched/local/mpi_threads_dynamic.hpp"
#include "vm/state.hpp"
#include "process/router.hpp"
#include "utils/utils.hpp"
#include "sched/thread/assert.hpp"

using namespace std;
using namespace vm;
using namespace process;
using namespace db;
using namespace boost;
using namespace utils;

namespace sched
{
   
volatile static bool iteration_finished;
static tokenizer *token;
static mutex tok_mutex;

void
mpi_thread::assert_end(void) const
{
   dynamic_local::assert_end();
   assert(iteration_finished);
   assert_mpi();
}

void
mpi_thread::assert_end_iteration(void) const
{
   dynamic_local::assert_end_iteration();
   assert(iteration_finished);
   assert_mpi();
}

void
mpi_thread::messages_were_transmitted(const size_t total)
{
   mutex::scoped_lock lock(tok_mutex);
   token->messages_transmitted(total);
}

void
mpi_thread::messages_were_received(const size_t total)
{
   mutex::scoped_lock lock(tok_mutex);
   token->messages_received(total);
}

void
mpi_thread::new_mpi_message(message *msg)
{
   thread_node *node((thread_node*)state::DATABASE->find_node(msg->id));
   simple_tuple *stpl(msg->data);
   mutex::scoped_lock lock(node->mtx);

   if(node->get_owner() == this) {
      node->add_work(stpl, false);
      if(!node->in_queue()) {
         node->set_in_queue(true);
         this->add_to_queue(node);
      }
      if(is_inactive()) {
         mutex::scoped_lock lock(mutex);
         if(is_inactive())
            set_active();
      }
      assert(node->in_queue());
      assert(is_active());
   } else {
      mpi_thread *owner = (mpi_thread*)node->get_owner();
   
      node->add_work(stpl, false);
   
      if(!node->in_queue()) {
         node->set_in_queue(true);
         owner->add_to_queue(node);
      }
   
      assert(owner != NULL);
   
      if(owner->is_inactive()) {
         mutex::scoped_lock lock(owner->mutex);
         if(owner->is_inactive() && owner->has_work())
            owner->set_active();
      }
   }
}


void
mpi_thread::change_node(thread_node *node, dynamic_local *_asker)
{
   mpi_thread *asker((mpi_thread*)_asker);
   
   assert(node != current_node);
   assert(node->get_owner() == this);
   
   remove_node(node);
   asker->add_node(node);
   
   {
      mutex::scoped_lock lock(node->mtx);
      node->set_owner((static_local*)asker);
      assert(node->in_queue());
      assert(node->get_owner() == asker);
   }
   
   asker->add_to_queue(node);
}

bool
mpi_thread::busy_wait(void)
{
   volatile bool turned_inactive(false);
   size_t asked_many(0);
   
   transmit_messages();
   if(leader_thread())
      fetch_work();
   update_pending_messages(true);
   
   while(!has_work()) {
      
      if(state::NUM_THREADS > 1 && asked_many < MAX_ASK_STEAL) {
         mpi_thread *target((mpi_thread*)select_steal_target());
         
         if(target->is_active()) {
            target->request_work_to(this);
            ++asked_many;
         }
      }
      
      if(!turned_inactive && !has_work()) {
         assert(!turned_inactive);
         mutex::scoped_lock l(mutex);
         if(!has_work()) {
            if(is_active()) {
               set_inactive();
            }
            turned_inactive = true;
            if(!leader_thread() && iteration_finished)
               return false;
         }
      }
      
      if(leader_thread() && all_threads_finished()) {
         mutex::scoped_lock lock(tok_mutex);
         if(!token->busy_loop_token(all_threads_finished())) {
            assert(all_threads_finished());
            assert(turned_inactive);
            iteration_finished = true;
            return false;
         }
      }
      
      if(!leader_thread() && turned_inactive && is_inactive() && iteration_finished) {
         assert(!leader_thread()); // leader thread does not finish here
         assert(is_inactive());
         assert(turned_inactive);
         assert(iteration_finished);
         assert(all_threads_finished());
         return false;
      }
      
      if(leader_thread())
         fetch_work();
   }
   
   set_active_if_inactive();
   
   assert(is_active());
   assert(has_work());
   
   return true;
}

void
mpi_thread::new_work_remote(remote *rem, const node::node_id, message *msg)
{
   buffer_message(rem, 0, msg);
}

bool
mpi_thread::get_work(work_unit& work)
{  
   do_mpi_worker_cycle();
   
   if(leader_thread())
      do_mpi_leader_cycle();
   
   return dynamic_local::get_work(work);
}

bool
mpi_thread::terminate_iteration(void)
{
   // this is needed since one thread can reach set_active
   // and thus other threads waiting for all_finished will fail
   // to get here
   threads_synchronize();
   update_pending_messages(false); // just delete all requests
   
   if(leader_thread())
      token->token_terminate_iteration();

   assert(iteration_finished);
   assert(is_inactive());

   generate_aggs();

   if(has_work())
      set_active();
      
   assert_thread_iteration(iteration);

   // again, needed since we must wait if any thread
   // is set to active in the previous if
   threads_synchronize();
   
   if(leader_thread()) {
      const bool more_work = state::ROUTER->reduce_continue(!all_threads_finished());
      iteration_finished = !more_work;
   }
   
   // threads must wait for the final answer between processes
   threads_synchronize();

   return !iteration_finished;
}
   
vector<sched::base*>&
mpi_thread::start(const size_t num_threads)
{
   init_barriers(num_threads);
   token = new sched::tokenizer();
   
   iteration_finished = false;
   
   for(process_id i(0); i < num_threads; ++i)
      add_thread(new mpi_thread(i));
   
   return ALL_THREADS;
}

}