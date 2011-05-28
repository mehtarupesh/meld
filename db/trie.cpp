
#include <string>

#include "utils/utils.hpp"
#include "db/trie.hpp"
#include "db/agg_configuration.hpp"

using namespace vm;
using namespace std;
using namespace runtime;
using namespace std::tr1;

namespace db
{

static const size_t STACK_EXTRA_SIZE(3);
static const size_t TRIE_HASH_LIST_THRESHOLD(8);
static const size_t TRIE_HASH_BASE_BUCKETS(64);
static const size_t TRIE_HASH_MAX_NODES_PER_BUCKET(TRIE_HASH_LIST_THRESHOLD / 2);

size_t
trie_hash::count_refs(void) const
{
   size_t ret(0);
   
   for(size_t i(0); i < num_buckets; ++i) {
      trie_node *next(buckets[i]);
      
      while (next) {
         ret += next->count_refs();
         next = next->next;
      }
   }
   
   return ret;
}

size_t
trie_node::count_refs(void) const
{
   if(is_leaf()) {
      return get_leaf()->get_count();
   } else {
      size_t ret(0);
      
      trie_node *cur(get_child());
      
      if(is_hashed()) {
         trie_hash *hash((trie_hash*)cur);
         ret += hash->count_refs();
      } else {
         while (cur) {
            ret += cur->count_refs();
         
            cur = cur->next;
         }
      }
      
      return ret;
   }
}

trie_node*
trie_node::get_by_first_int(const int_val val) const
{
   assert(!is_leaf());
   
   if(get_child() == NULL)
      return NULL;
   
   trie_node *cur(get_child());
   
   if(is_hashed()) {
      trie_hash *hash((trie_hash*)cur);
      
      cur = hash->get_int(val);
   }
   
   while (cur) {
      if(cur->data.int_field == val) {
         // found it !
         return cur;
      }
      
      cur = cur->get_next();
   }
   
   return 0;
}

trie_node*
trie_node::match(const tuple_field& field, const field_type& typ,
      val_stack& vals, type_stack& typs, size_t& count) const
{
   assert(!is_leaf());
   
   count = 0;
   
   trie_node *next(get_child());
   
   if(next == NULL)
      return NULL;
      
   if(is_hashed()) {
      trie_hash *hash((trie_hash*)next);
      switch(typ) {
         case FIELD_INT: next = hash->get_int(field.int_field); break;
         case FIELD_FLOAT: next = hash->get_float(field.float_field); break;
         case FIELD_NODE: next = hash->get_node(field.node_field); break;
#define HASH_LIST(LIST_TYPE) { \
            LIST_TYPE *ls((LIST_TYPE*)field.ptr_field);  \
            next = hash->get_int(LIST_TYPE::is_null(ls) ? 0 : 1); \
         }
         case FIELD_LIST_INT: HASH_LIST(int_list); break;
         case FIELD_LIST_FLOAT: HASH_LIST(float_list); break;
         case FIELD_LIST_NODE: HASH_LIST(node_list); break;
         default: assert(false);
#undef HASH_LIST
      }
    }
   
   while(next != NULL) {
      tuple_field& f(next->data);
      
      ++count;
      
      switch(typ) {
         case FIELD_INT:
            if(f.int_field == field.int_field) {
               return next;
            }
            break;
         case FIELD_FLOAT:
            if(f.float_field == field.float_field)
               return next;
            break;
         case FIELD_NODE:
            if(f.node_field == field.node_field)
               return next;
            break;
            
#define MATCH_LIST(LIST_TYPE, FIELD_LIST_TYPE, FIELD_ITEM_TYPE, ITEM_FIELD) { \
            LIST_TYPE *ls((LIST_TYPE*)field.ptr_field);  \
            if(LIST_TYPE::is_null(ls)) { \
               if(f.int_field == 0) return next; \
            } else { \
                  if(f.int_field == 1) { \
                     tuple_field head, tail; \
                     head.ITEM_FIELD = ls->get_head(); \
                     tail.ptr_field = (ptr_val)ls->get_tail(); \
                     vals.push(tail); \
                     typs.push(FIELD_LIST_TYPE); \
                     vals.push(head); \
                     typs.push(FIELD_ITEM_TYPE); \
                     return next; \
                  } \
               } \
            } \
      
         case FIELD_LIST_INT: MATCH_LIST(int_list, FIELD_LIST_INT, FIELD_INT, int_field); break;
         case FIELD_LIST_FLOAT: MATCH_LIST(float_list, FIELD_LIST_FLOAT, FIELD_FLOAT, float_field); break;
         case FIELD_LIST_NODE: MATCH_LIST(node_list, FIELD_LIST_NODE, FIELD_NODE, node_field); break;
         default: assert(false);
      }
      
      next = next->get_next();
   }

   return NULL;
}
   
trie_node*
trie_node::insert(const tuple_field& field, const field_type& type, val_stack& vals, type_stack& typs)
{
   tuple_field f;
   trie_node *new_child;
   
   switch(type) {
#define INSERT_LIST(LIST_TYPE, FIELD_LIST_TYPE, FIELD_ITEM_TYPE, ITEM_FIELD) { \
      LIST_TYPE *ls((LIST_TYPE*)field.ptr_field); \
      if(LIST_TYPE::is_null(ls)) { \
         f.int_field = 0; \
      } else { \
         f.int_field = 1; \
         tuple_field head, tail; \
         head.ITEM_FIELD = ls->get_head(); \
         tail.ptr_field = (ptr_val)ls->get_tail(); \
         vals.push(tail); \
         typs.push(FIELD_LIST_TYPE); \
         vals.push(head); \
         typs.push(FIELD_ITEM_TYPE); \
      } \
   }
         case FIELD_LIST_INT: INSERT_LIST(int_list, FIELD_LIST_INT, FIELD_INT, int_field); break;
         case FIELD_LIST_FLOAT: INSERT_LIST(float_list, FIELD_LIST_FLOAT, FIELD_FLOAT, float_field); break;
         case FIELD_LIST_NODE: INSERT_LIST(node_list, FIELD_LIST_NODE, FIELD_NODE, node_field); break;
         default:
            f = field;
            break;
   }
#undef INSERT_LIST
   
   new_child = new trie_node(f);
   
   assert(!is_leaf());
   
   new_child->parent = this;
   
   if(is_hashed()) {
      trie_hash *hash((trie_hash*)child);
      
      hash->total++;
      
      switch(type) {
         case FIELD_LIST_INT:
         case FIELD_LIST_FLOAT:
         case FIELD_LIST_NODE:
         case FIELD_INT: hash->insert_int(f.int_field, new_child); break;
         case FIELD_FLOAT: hash->insert_float(f.float_field, new_child); break;
         case FIELD_NODE: hash->insert_node(f.node_field, new_child); break;
         default: assert(false);
      }
   } else {
      new_child->next = child;
      new_child->prev = NULL;
   
      if(child)
         child->prev = new_child;
   
      child = new_child;
   }
   
   assert(new_child->get_parent() == this);
   
   return new_child;
}

// put all children into hash table
void
trie_node::convert_hash(const field_type& type)
{
   assert(!is_hashed());
   
   trie_node *next(get_child());
   trie_hash *hash(new trie_hash(type, this));
   size_t total(0);
   
   while (next != NULL) {
      trie_node *tmp(next->next);
      
      switch(type) {
         case FIELD_LIST_INT:
         case FIELD_LIST_FLOAT:
         case FIELD_LIST_NODE:
         case FIELD_INT: hash->insert_int(next->data.int_field, next); break;
         case FIELD_FLOAT: hash->insert_float(next->data.float_field, next); break;
         case FIELD_NODE: hash->insert_node(next->data.node_field, next); break;
         default: assert(false);
      }
      
      ++total;
      next = tmp;
   }
   
   hash->total = total;
   child = (trie_node*)hash;
   hashed = true;
   
   assert(is_hashed());
}

trie_node::~trie_node(void)
{
}

trie_node*
trie_hash::get_int(const int_val& val) const
{
   const size_t bucket(hash_item(std::tr1::hash<int_val>()(val)));
   return buckets[bucket];
}

trie_node*
trie_hash::get_float(const float_val& val) const
{
   const size_t bucket(hash_item(std::tr1::hash<float_val>()(val)));
   return buckets[bucket];
}

trie_node*
trie_hash::get_node(const node_val& val) const
{
   const size_t bucket(hash_item(std::tr1::hash<node_val>()(val)));
   return buckets[bucket];
}

void
trie_hash::insert_int(const int_val& val, trie_node *node)
{
   const size_t bucket(hash_item(std::tr1::hash<int_val>()(val)));
   
   assert(num_buckets > 0);
   assert(bucket < num_buckets);
   
   trie_node *old(buckets[bucket]);
   
   node->prev = NULL;
   node->next = old;
   if(old)
      old->prev = node;
   node->bucket = buckets + bucket;
   
   buckets[bucket] = node;
}

void
trie_hash::insert_float(const float_val& val, trie_node *node)
{
   const size_t bucket(hash_item(std::tr1::hash<float_val>()(val)));
   
   assert(bucket < num_buckets);
   
   trie_node *old(buckets[bucket]);
   
   node->prev = NULL;
   node->next = old;
   if(old)
      old->prev = node;
   node->bucket = buckets + bucket;
   
   buckets[bucket] = node;
}

void
trie_hash::insert_node(const node_val& val, trie_node *node)
{
   const size_t bucket(hash_item(std::tr1::hash<node_val>()(val)));
   
   assert(bucket < num_buckets);
   
   trie_node *old(buckets[bucket]);
   
   node->prev = NULL;
   node->next = old;
   if(old)
      old->prev = node;
   node->bucket = buckets + bucket;
   
   buckets[bucket] = node;
}

void
trie_hash::expand(void)
{
   const size_t old_num_buckets(num_buckets);
   trie_node **old_buckets(buckets);
   
   num_buckets *= 2;
   buckets = allocator<trie_node*>().allocate(num_buckets);
   memset(buckets, 0, sizeof(trie_node*)*num_buckets);
   
   for(size_t i(0); i < old_num_buckets; ++i) {
      trie_node *next(old_buckets[i]);
      
      while (next) {
         trie_node *tmp(next->next);
         
         switch(type) {
            case FIELD_LIST_INT:
            case FIELD_LIST_FLOAT:
            case FIELD_LIST_NODE:
            case FIELD_INT: insert_int(next->data.int_field, next); break;
            case FIELD_FLOAT: insert_float(next->data.float_field, next); break;
            case FIELD_NODE: insert_node(next->data.node_field, next); break;
            default: assert(false);
         }
         
         next = tmp;
      }
   }
   
   allocator<trie_node*>().deallocate(old_buckets, old_num_buckets);
}

trie_hash::trie_hash(const vm::field_type& _type, trie_node *_parent):
   type(_type), parent(_parent), total(0)
{
   buckets = allocator<trie_node*>().allocate(TRIE_HASH_BASE_BUCKETS);
   num_buckets = TRIE_HASH_BASE_BUCKETS;
   memset(buckets, 0, sizeof(trie_node*)*num_buckets);
}

trie_hash::~trie_hash(void)
{
   allocator<trie_node*>().deallocate(buckets, num_buckets);
}

void
trie::delete_path(trie_node *node)
{  
   trie_node *parent(node->get_parent());
   
   if(node == root) // reached root
      return;
      
   assert(node->child == NULL);
   
   if(node->prev != NULL)
      node->prev->next = node->next;
      
   if(node->next != NULL)
      node->next->prev = node->prev;
      
   if(node->bucket != NULL) {
      trie_hash *hash((trie_hash*)parent->child);
      hash->total--;
   }
   
   if(node->prev == NULL) {
      if(node->bucket != NULL) {
         *(node->bucket) = node->next;
         
         trie_hash *hash((trie_hash*)parent->child);
         
         if(hash->total == 0) {
            delete hash;
            assert(parent != (trie_node*)hash);
            parent->child = NULL;
            delete_path(parent);
         }
      } else {
         assert(parent->child == node);
         
         parent->child = node->next;
         
         if(parent->child == NULL) {
            assert(node->next == NULL);
            delete_path(parent);
         }
      }
   }
   
   delete node;
}

void
trie::delete_branch(trie_node *node)
{
   if(node->is_leaf()) {
      trie_leaf *leaf(node->get_leaf());
      
      if(leaf->prev)
         leaf->prev->next = leaf->next;
      if(leaf->next)
         leaf->next->prev = leaf->prev;
      
      if(leaf == first_leaf)
         first_leaf = leaf->next;
      if(leaf == last_leaf)
         last_leaf = leaf->prev;
      
      delete leaf;
      return;
   }
   
   trie_node *next(node->get_child());
   
   if(node->is_hashed()) {
      trie_hash *hash((trie_hash*)next);
      
      for(size_t i(0); i < hash->num_buckets; ++i) {
         if(hash->buckets[i]) {
            trie_node *next(hash->buckets[i]);

            while(next != NULL) {
               trie_node *tmp(next->next);

               delete_branch(next);
               delete next;

               next = tmp;
            }
         }
      }
   } else {
      while(next != NULL) {
         trie_node *tmp(next->get_next());
         
         delete_branch(next);
         delete next;
         
         next = tmp;
      }
   }
}

trie_node*
trie::check_insert(void *data, const ref_count many, val_stack& vals, type_stack& typs, bool& found)
{
   basic_invariants();
   
   if(vals.empty()) {
      // 0-arity tuple
      if(!root->is_leaf()) {
         found = false;
         trie_leaf *leaf(create_leaf(data, many));
         root->set_leaf(leaf);
         leaf->node = root;
         leaf->prev = leaf->next = NULL;
         first_leaf = last_leaf = leaf;
      } else {
         found = true;
         trie_leaf *leaf(root->get_leaf());
         
         leaf->add_count(many);
      }
      
      return root;
   }
   
   trie_node *parent(root);
   
   while (!parent->is_leaf()) {
      assert(!vals.empty() && !typs.empty());
      
      tuple_field field(vals.top());
      field_type typ(typs.top());
      trie_node *cur;
      size_t count;
      
      vals.pop();
      typs.pop();
      
      cur = parent->match(field, typ, vals, typs, count);
      
      if(cur == NULL) {
         // insert
         assert(many > 0);
         assert(!parent->is_leaf());
         
         ++count;
         
         if(count > TRIE_HASH_LIST_THRESHOLD && !parent->is_hashed()) {
            parent->convert_hash(typ);
            assert(parent->is_hashed());
         } else if (parent->is_hashed() && count > TRIE_HASH_MAX_NODES_PER_BUCKET) {
            assert(parent->is_hashed());
            ((trie_hash*)parent->child)->expand();
         }
         
         parent = parent->insert(field, typ, vals, typs);

         assert(parent != root);
         
         while(!vals.empty()) {
            field = vals.top();
            typ = typs.top();
            vals.pop();
            typs.pop();
            
            assert(!parent->is_leaf());
            
            parent = parent->insert(field, typ, vals, typs);
         }
         
         assert(vals.empty());
         assert(typs.empty());
         
         // parent is now set as a leaf
         trie_leaf *leaf(create_leaf(data, many));
         leaf->node = parent;
         parent->set_leaf(leaf);
         leaf->next = NULL;
         
         if(first_leaf == NULL) {
            first_leaf = last_leaf = leaf;
            leaf->prev = NULL;
         } else {
            leaf->prev = last_leaf;
            last_leaf->next = leaf;
            last_leaf = leaf;
         }
         
         assert(last_leaf == leaf);
         
         found = false;
         
         assert(!root->is_leaf());
         
         return parent;
      }
      
      parent = cur;
   }
   
   assert(vals.empty());
   assert(typs.empty());
   assert(!root->is_leaf());
   
   found = true;
   
   assert(parent->is_leaf());
   trie_leaf *orig(parent->get_leaf());
   
   orig->add_count(many);
   
   basic_invariants();
   
   return parent;
}

void
trie::commit_delete(trie_node *node)
{
   /*simple_tuple *tpl(node->get_tuple_leaf());
   cout << "To delete: " << *tpl << endl;
   dump(cout);*/

   assert(node->is_leaf());

   trie_leaf *leaf(node->get_leaf());

   if(leaf->next)
      leaf->next->prev = leaf->prev;
   if(leaf->prev)
      leaf->prev->next = leaf->next;
      
   if(leaf == first_leaf)
      first_leaf = first_leaf->next;
   
   if(leaf == last_leaf)
      last_leaf = last_leaf->prev;
      
   if(total == 0) {
      assert(last_leaf == NULL);
      assert(first_leaf == NULL);
   }
   
   //cout << this << " Total " << total << " root " << root << " node " << node << endl;
   
   delete leaf;
   node->child = NULL;
   delete_path(node);
   
   if(total == 0)
      assert(root->child == NULL);
      
   assert(root->next == NULL);
   assert(root->prev == NULL);

#ifdef TRIE_ASSERT
   leaf = first_leaf;
   size_t count(0);
   while(leaf) {
      count += leaf->get_count();
      if(!leaf->next)
         assert(last_leaf == leaf);
      leaf = leaf->next;
   }
   
   assert(count == total);
   
   basic_invariants();
#endif
}

void
trie::delete_by_first_int_arg(const int_val val)
{
   basic_invariants();
   
   trie_node *node(root->get_by_first_int(val));

   if(node == NULL)
      return;
   
   if(node->prev != NULL)
      node->prev->next = node->next;
   if(node->next != NULL)
      node->next->prev = node->prev;
      

   if(node->bucket != NULL) {
      trie_hash *hash((trie_hash*)node->parent->child);
      hash->total--;
   }
   
   if(node->prev == NULL) {
      if(node->bucket != NULL)
         *(node->bucket) = node->next;
      else
         node->parent->child = node->next;
   }
   
   total -= node->count_refs();
      
   delete_branch(node);
   
   basic_invariants();
   
   delete node;
}

void
trie::wipeout(void)
{
   delete_branch(root);
   delete root;
   total = 0;
}

trie::trie(void):
   root(new trie_node()),
   total(0),
   first_leaf(NULL),
   last_leaf(NULL)
{
   basic_invariants();
}

trie::~trie(void)
{
   wipeout();
}

trie_node*
tuple_trie::check_insert(vm::tuple *tpl, const ref_count many, bool& found)
{
   // cout << "Starting insertion of " << *tpl << endl;
 
   val_stack vals(tpl->num_fields() + STACK_EXTRA_SIZE);
   type_stack typs(tpl->num_fields() + STACK_EXTRA_SIZE);
  
   if(tpl->num_fields() > 0) {
      for(int i(tpl->num_fields()-1); i >= 0; --i) {
         vals.push(tpl->get_field(i));
         typs.push(tpl->get_field_type(i));
      }
   }
   
   return trie::check_insert((void*)tpl, many, vals, typs, found);
}

bool
tuple_trie::insert_tuple(vm::tuple *tpl, const ref_count many)
{
   /*cout << this << " To insert " << *tpl << endl;
   dump(cout);
   */
   
   bool found;
   check_insert(tpl, many, found);
   
   if(found) {
      assert(root->child != NULL);
      assert(total > 0);
   }
   
   /*
   cout << "After:" << endl;
   dump(cout);
   cout << endl;
   */
   
   total += many;
   
   basic_invariants();
   
   const bool is_new(!found);
   
#ifdef TRIE_ASSERT
   trie_leaf *leaf(first_leaf);
   size_t count(0);
   while(leaf) {
      count += leaf->get_count();
      if(!leaf->next)
         assert(last_leaf == leaf);
      leaf = leaf->next;
   }
   
   assert(count == total);
#endif
   
   return is_new;
}

trie::delete_info
tuple_trie::delete_tuple(vm::tuple *tpl, const ref_count many)
{
   assert(many > 0);
   basic_invariants();
   
   /*
   cout << "Before " << *tpl << " " << total << endl;
   dump(cout);*/
   
   bool found;
   trie_node *node(check_insert(tpl, -many, found));
   
   assert(found);
   
   total -= many;
   
   trie_leaf *leaf(node->get_leaf());
   
   if(leaf->to_delete())
      return delete_info(this, true, node);
   else
      return delete_info(false);
}

void
tuple_trie::print(ostream& cout) const
{
   assert(!empty());
   
   cout << " " << *pred << ":" << endl;
   
   for(const_iterator it(begin());
      it != end();
      it++)
   {
      simple_tuple *stuple(*it);
      cout << "\t" << *stuple << endl;
   }
}

void
tuple_trie::dump(ostream& cout) const
{
   std::list<string> ls;
   
   for(const_iterator it(begin());
      it != end();
      it++)
   {
      simple_tuple *stuple(*it);
      ls.push_back(utils::to_string(*stuple));
   }
   
   ls.sort();
   
   for(std::list<string>::const_iterator it(ls.begin());
      it != ls.end();
      ++it)
   {
      cout << *it << endl;
   }
}

void
tuple_trie::match_predicate(tuple_vector& vec) const
{
   for(const_iterator it(begin());
      it != end();
      it++)
   {
      vec.push_back((*it)->get_tuple());
   }
}

struct continuation_frame {
   size_t size_stacks;
   trie_node *next_node;
};

typedef utils::stack<continuation_frame> continuation_stack;

void
tuple_trie::match_predicate(const match& m, tuple_vector& vec) const
{
   if(!m.has_any_exact()) {
      // just retrieve everything and be done with it
      match_predicate(vec);
      return;
   }
   
   const size_t stack_size(pred->num_fields() + STACK_EXTRA_SIZE);
   match_val_stack vals(stack_size);
   match_type_stack typs(stack_size);
   continuation_stack cont(stack_size);
   match_val_stack val_backup(stack_size);
   match_type_stack typ_backup(stack_size);
   
   trie_node *parent(root);
   trie_node *node(root->child);
   
   // initialize stacks
   m.get_type_stack(typs);
   m.get_val_stack(vals);

   // dump(cout);
   // if it was a leaf, m would have no exact match
   assert(!root->is_leaf());
   
   match_field mtype;
   tuple_field mfield;
   bool going_down = true;
   continuation_frame frm;
   
#define ADD_ALT(NODE) do { \
   assert((NODE) != NULL); \
   frm.next_node = NODE;   \
   frm.size_stacks = val_backup.size();   \
   cont.push(frm); } while(false)
   
match_begin:
   // printf("Start loop\n");
   assert(!vals.empty());
   assert(!typs.empty());
   
   mtype = typs.top();
   
   assert(node != NULL);
   assert(parent != NULL);
   
   if(mtype.exact) {
      // printf("Match exact\n");
      // must do an exact match
      // there will be no continuation frames at this level
      mfield = vals.top();
      switch(mtype.type) {
         case FIELD_INT:
            // printf("Match %d\n", mfield.int_field);
            if(parent->is_hashed()) {
               assert(going_down);
               
               trie_hash *hash((trie_hash*)node);
               
               node = hash->get_int(mfield.int_field);
            }
            
            while(node) {
               if(node->data.int_field == mfield.int_field)
                  goto match_succeeded;
               node = node->next;
            }
            goto try_again;
         case FIELD_FLOAT:
            if(parent->is_hashed()) {
               assert(going_down);
               
               trie_hash *hash((trie_hash*)node);
               
               node = hash->get_float(mfield.float_field);
            }
            
            while(node) {
               if(node->data.float_field == mfield.float_field)
                  goto match_succeeded;
               node = node->next;
            }
            goto try_again;
         case FIELD_NODE:
            // printf("Match node %d\n", mfield.node_field);
            if(parent->is_hashed()) {
               assert(going_down);
               
               trie_hash *hash((trie_hash*)node);
               
               node = hash->get_node(mfield.node_field);
            }
            
            while(node) {
               if(node->data.node_field == mfield.node_field)
                  goto match_succeeded;
               node = node->next;
            }
            goto try_again;
         default: assert(false);
      }
   } else {
      // printf("Match all\n");
      if(parent->is_hashed()) {
         trie_hash *hash((trie_hash*)node);
         
         if(going_down) {
            // must select a valid node
            trie_node **buckets(hash->buckets);
            trie_node **end_buckets(buckets + hash->num_buckets);
            
            // find first valid bucket
            for(; *buckets == NULL && buckets < end_buckets; ++buckets) {
               if(*buckets)
                  break;
            }
            
            assert(buckets < end_buckets);
            
            node = *buckets;
            assert(node != NULL);
            
            if(node->next)
               ADD_ALT(node->next);
            else {
               // find another valid bucket
               ++buckets;
               for(; *buckets == NULL && buckets < end_buckets; ++buckets)
                  ;
               
               if(buckets != end_buckets)
                  ADD_ALT(*buckets);
            }
         } else {
            // must continue traversing the hash table
            assert(parent->child != node);
            
            trie_hash *hash((trie_hash*)parent->child);
            
            assert(hash != NULL);
            
            trie_node **buckets(hash->buckets);
            trie_node **end_buckets(buckets + hash->num_buckets);
            trie_node **current_bucket(node->bucket);
            
            if(node->next)
               ADD_ALT(node->next);
            else {
               // must find new bucket
               ++current_bucket;
               for(; *current_bucket == NULL && current_bucket < end_buckets; ++current_bucket)
                  ;
               if(current_bucket != end_buckets)
                  ADD_ALT(*current_bucket);
            }
         }
      } else {
         // push first alternative
         if(node->next)
            ADD_ALT(node->next);
      }
   }
      
match_succeeded:
   // printf("Nice\n");
   
   if(node->is_leaf())
      // no need to save the stacks
      // since if we try again from the continuation stack
      // we do not need to put things back from the backup stacks
      // printf("Jump out!\n");
      goto leaf_found;
      
   val_backup.push(vals.top());
   typ_backup.push(typs.top());
   vals.pop();
   typs.pop();
      
   parent = node;
   node = node->child;
   going_down = true;
   goto match_begin;
   
try_again:
   // match failed
   // use continuation stack to restore state at a given node

   // printf("Failed\n");
   
   if(cont.empty())
      return;
   
   frm = cont.top();
   cont.pop();
   
   node = frm.next_node;
   parent = node->parent;
   going_down = false;
   
   // restore vals and typs stack
   while(val_backup.size() != frm.size_stacks) {
      assert(!val_backup.empty());
      assert(!typ_backup.empty());
      
      vals.push(val_backup.top());
      typs.push(typ_backup.top());
      // printf("Push back!\n");
      val_backup.pop();
      typ_backup.pop();
   }
   
   goto match_begin;
   
leaf_found:
   // printf("Got here\n");
   assert(node != NULL);
   assert(node->is_leaf());
      
   tuple_trie_leaf *leaf((tuple_trie_leaf*)node->get_leaf());
   vm::tuple *tpl(leaf->get_tuple()->get_tuple());
   // cout << "Added " << *tpl << endl;
   vec.push_back(tpl);
   goto try_again;
}

agg_trie_leaf::~agg_trie_leaf(void)
{
   assert(conf);
   delete conf;
}

agg_trie_leaf*
agg_trie::find_configuration(vm::tuple *tpl)
{
   const predicate *pred(tpl->get_predicate());
   
   const size_t stack_size(pred->get_aggregate_field() + STACK_EXTRA_SIZE);
   val_stack vals(stack_size);
   type_stack typs(stack_size);
  
   for(int i(pred->get_aggregate_field()-1); i >= 0; --i) {
      vals.push(tpl->get_field(i));
      typs.push(tpl->get_field_type(i));
   }
   
   bool found;
   trie_node *node(trie::check_insert(NULL, 1, vals, typs, found));
   
   if(!found)
      ++total;
   
   return (agg_trie_leaf*)node->get_leaf();
}

agg_trie_iterator
agg_trie::erase(agg_trie_iterator& it)
{
   agg_trie_leaf *leaf(it.current_leaf);
   agg_trie_leaf *next_leaf((agg_trie_leaf*)leaf->next);
   trie_node *node(leaf->node);
   
   commit_delete(node);
   total--;
   
   return agg_trie_iterator(next_leaf);
}
   
}
