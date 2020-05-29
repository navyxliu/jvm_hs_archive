#ifdef USE_PRAGMA_IDENT_HDR
#pragma ident "@(#)methodLiveness.hpp	1.16 03/12/23 16:42:43 JVM"
#endif
/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL.  Use is subject to license terms.
 */

class MethodLiveness : public ResourceObj {
 public: 
  // The BasicBlock class is used to represent a basic block in the
  // liveness analysis.
  class BasicBlock : public ResourceObj {
   private:
    // This class is only used by the MethodLiveness class.
    friend class MethodLiveness;

    // The analyzer which created this basic block.
    MethodLiveness* _analyzer;
    
    // The range of this basic block is [start_bci,limit_bci)
    int _start_bci;
    int _limit_bci;

    // The liveness at the start of the block;
    BitMap _entry;

    // The summarized liveness effects of our direct successors reached
    // by normal control flow
    BitMap _normal_exit;

    // The summarized liveness effects of our direct successors reached
    // by exceptional control flow
    BitMap _exception_exit;

    // These members hold the results of the last call to 
    // compute_gen_kill_range().  _gen is the set of locals
    // used before they are defined in the range.  _kill is the
    // set of locals defined before they are used.
    BitMap _gen;
    BitMap _kill;

    // A list of all blocks which could come directly before this one
    // in normal (non-exceptional) control flow.  We propagate liveness
    // information to these blocks.
    GrowableArray<BasicBlock*>* _normal_predecessors;
    
    // A list of all blocks which could come directly before this one
    // in exceptional control flow.
    GrowableArray<BasicBlock*>* _exception_predecessors;

    // The following fields are used to manage a work list used in the
    // dataflow.
    BasicBlock *_next;
    bool _on_work_list;

    // Our successors call this method to merge liveness information into
    // our _normal_exit member.
    bool merge_normal(BitMap other);

    // Our successors call this method to merge liveness information into
    // our _exception_exit member.
    bool merge_exception(BitMap other);

    // This helper routine is used to help compute the gen/kill pair for
    // the block.  It is also used to answer queries.
    void compute_gen_kill_range(ciByteCodeStream *bytes);

    // Compute the gen/kill effect of a single instruction.
    void compute_gen_kill_single(ciByteCodeStream *instruction);

    // Helpers for compute_gen_kill_single.
    void load_one(int local);
    void load_two(int local);
    void store_one(int local);
    void store_two(int local);

    BasicBlock(MethodLiveness *analyzer, int start, int limit);

    // -- Accessors

    int start_bci() const { return _start_bci; }

    int limit_bci() const { return _limit_bci; }
    void set_limit_bci(int limit) { _limit_bci = limit; } 

    BasicBlock *next() const { return _next; }
    void set_next(BasicBlock *next) { _next = next; }

    bool on_work_list() const { return _on_work_list; }
    void set_on_work_list(bool val) { _on_work_list = val; }

    // -- Flow graph construction.

    // Add a basic block to our list of normal predecessors.
    void add_normal_predecessor(BasicBlock *pred) {
      _normal_predecessors->append_if_missing(pred);
    }

    // Add a basic block to our list of exceptional predecessors
    void add_exception_predecessor(BasicBlock *pred) {
      _exception_predecessors->append_if_missing(pred);
    }

    // Split the basic block at splitBci.  This basic block
    // becomes the second half.  The first half is newly created.
    BasicBlock *split(int splitBci);

    // -- Dataflow.
    
    void compute_gen_kill(ciMethod* method);

    // Propagate changes from this basic block
    void propagate(MethodLiveness *ml);

    // -- Query.

    BitMap get_liveness_at(ciMethod* method, int bci);

    // -- Debugging.

    void print_on(outputStream *os) const PRODUCT_RETURN;

  }; // End of MethodLiveness::BasicBlock

 private:
  // The method we are analyzing.
  ciMethod* _method;
  ciMethod* method() const { return _method; }

  // The arena for storing structures...
  Arena*       _arena;
  Arena*       arena() const { return _arena; }

  // We cache the length of the method.
  int _code_size;

  // The size of a BitMap.
  int _bit_map_size_bits;
  int _bit_map_size_words;

  // A list of all BasicBlocks.
  GrowableArray<BasicBlock*>* _block_list;

  // Keeps track of bci->block mapping.  One entry for each bci.
  GrowableArray<BasicBlock*>* _block_map;

  // Our work list.
  BasicBlock *_work_list;

  // -- Graph construction & Analysis

  // Compute ranges and predecessors for basic blocks.
  void init_basic_blocks();

  // Construct a new basic block.
  BasicBlock *make_block_at(int bci, BasicBlock *pred);

  // Compute gen/kill information for all basic blocks.
  void init_gen_kill();

  // Perform the dataflow.
  void propagate_liveness();

 // The class MethodLiveness::BasicBlock needs special access to some
 // of our members.
 friend class MethodLiveness::BasicBlock;

  // And accessors.
  int bit_map_size_bits() const { return _bit_map_size_bits; }
  int bit_map_size_words() const { return _bit_map_size_words; }

  // Work list manipulation routines.  Called internally by BasicBlock.
  BasicBlock *work_list_get();
  void work_list_add(BasicBlock *block);

  // -- Timing and Statistics.


  // Timers
  static elapsedTimer _time_build_graph;
  static elapsedTimer _time_gen_kill;
  static elapsedTimer _time_flow;
  static elapsedTimer _time_query;
  static elapsedTimer _time_total;

#ifndef PRODUCT

  // Counts
  static long _total_bytes;
  static int  _total_methods;

  static long _total_blocks;
  static int  _max_method_blocks;

  static long _total_edges;
  static int  _max_block_edges;

  static long _total_exc_edges;
  static int  _max_block_exc_edges;

  static long _total_method_locals;
  static int  _max_method_locals;
  
  static long _total_locals_queried;
  static long _total_live_locals_queried;
  
  static long _total_visits;

#endif

 public:
  // Create a liveness analyzer for a method
  MethodLiveness(Arena* arena, ciMethod* method);

  // Compute liveness information for the method
  void compute_liveness();

  // Find out which locals are live at a specific bci.
  BitMap get_liveness_at(int bci);

  static void print_times() PRODUCT_RETURN;
};