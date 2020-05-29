#ifdef USE_PRAGMA_IDENT_HDR
#pragma ident "@(#)c1_GraphBuilder.hpp	1.60 03/12/23 16:39:05 JVM"
#endif
// 
// Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
// SUN PROPRIETARY/CONFIDENTIAL.  Use is subject to license terms.
// 

class ValueMap;

class BlockListBuilder VALUE_OBJ_CLASS_SPEC {
 private:
  IRScope*    _scope;
  BlockList*  _bci2block;
  BlockBegin* _std_entry;
  BlockBegin* _osr_entry;

  // helper functions
  BlockBegin* new_block_at(int bci, BlockBegin::Flag f = BlockBegin::no_flag);
  void set_leaders();
  void set_xhandler_entries();

 public:
  // creation
  BlockListBuilder(IRScope* scope, int osr_bci, bool generate_std_entry = true);

  // accessors
  IRScope*      scope() const                    { return _scope; }
  ciMethod*     method() const                   { return scope()->method(); }
  XHandlers*    xhandlers() const                { return scope()->xhandlers(); }
  BlockList*    bci2block() const                { return _bci2block; }
  BlockBegin*   std_entry() const                { return _std_entry; }
  BlockBegin*   osr_entry() const                { return _osr_entry; }
};


class GraphBuilder VALUE_OBJ_CLASS_SPEC {
 private:
  // Per-scope data. These are pushed and popped as we descend into
  // inlined methods. Currently in order to generate good code in the
  // inliner we have to attempt to inline methods directly into the
  // basic block we are parsing; this adds complexity.
  class ScopeData: public CompilationResourceObj {
   private:
    ScopeData*  _parent;
    bool        _using_cha;  // indicates whether the current scope being parsed
                             // was selected using CHA.
    // bci-to-block mapping
    BlockList*   _bci2block;
    // Scope
    IRScope*     _scope;
    // Whether this scope or any parent scope has exception handlers
    bool         _has_handler;
    // The bytecodes
    ciBytecodeStream* _stream;

    // Work list
    BlockList*   _work_list;

    // Maximum inline size for this scope
    intx         _max_inline_size;
    // Expression stack depth at point where inline occurred
    int          _caller_stack_size;

    // The continuation point for the inline. Currently only used in
    // multi-block inlines, but eventually would like to use this for
    // all inlines for uniformity and simplicity; in this case would
    // get the continuation point from the BlockList instead of
    // fabricating it anew because Invokes would be considered to be
    // BlockEnds.
    BlockBegin*  _continuation;

    // Was this ScopeData created only for the parsing and inlining of
    // a jsr?
    bool         _parsing_jsr;
    // We track the destination bci of the jsr only to determine
    // bailout conditions, since we only handle a subset of all of the
    // possible jsr-ret control structures. Recursive invocations of a
    // jsr are disallowed by the verifier.
    int          _jsr_entry_bci;
    // We need to track the local variable in which the return address
    // was stored to ensure we can handle inlining the jsr, because we
    // don't handle arbitrary jsr/ret constructs.
    int          _jsr_ret_addr_local;
    // If we are parsing a jsr, the continuation point for rets
    BlockBegin*  _jsr_continuation;
    // Cloned XHandlers for jsr-related ScopeDatas
    XHandlers*   _jsr_xhandlers;

    // Number of returns seen in this scope
    int          _num_returns;

    // In order to generate profitable code for inlining, we currently
    // have to perform an optimization for single-block inlined
    // methods where we continue parsing into the same block. This
    // allows us to perform CSE across inlined scopes and to avoid
    // storing parameters to the stack. Having a global register
    // allocator and being able to perform global CSE would allow this
    // code to be removed and thereby simplify the inliner.
    BlockBegin*  _cleanup_block;       // The block to which the return was added
    Instruction* _cleanup_return_prev; // Instruction before return instruction
    ValueStack*  _cleanup_state;       // State of that block (not yet pinned)
   public:
    ScopeData(ScopeData* parent, bool using_cha = false);

    ScopeData* parent() const                      { return _parent;            }
    bool using_cha() const                         { return _using_cha;         }

    BlockList* bci2block() const                   { return _bci2block;         }
    void       set_bci2block(BlockList* bci2block) { _bci2block = bci2block;    }

    // NOTE: this has a different effect when parsing jsrs
    BlockBegin* block_at(int bci);

    IRScope* scope() const                         { return _scope;             }
    // Has side-effect of setting has_handler flag
    void set_scope(IRScope* scope);

    // Whether this or any parent scope has exception handlers
    bool has_handler() const                       { return _has_handler;       }

    // Exception handlers list to be used for this scope
    XHandlers* xhandlers() const;

    // How to get a block to be parsed
    void add_to_work_list(BlockBegin* block);
    // How to remove the next block to be parsed; returns NULL if none left
    BlockBegin* remove_from_work_list();
    // Indicates parse is over
    bool is_work_list_empty() const;

    ciBytecodeStream* stream()                     { return _stream;            }
    void set_stream(ciBytecodeStream* stream)      { _stream = stream;          }

    intx max_inline_size() const                   { return _max_inline_size;   }
    int  caller_stack_size() const;

    BlockBegin* continuation() const               { return _continuation;      }
    void set_continuation(BlockBegin* cont)        { _continuation = cont;      }

    // Indicates whether this ScopeData was pushed only for the
    // parsing and inlining of a jsr
    bool parsing_jsr() const                       { return _parsing_jsr;       }
    void set_parsing_jsr()                         { _parsing_jsr = true;       }
    int  jsr_entry_bci() const                     { return _jsr_entry_bci;     }
    void set_jsr_entry_bci(int bci)                { _jsr_entry_bci = bci;      }
    void set_jsr_return_address_local(int local_no){ _jsr_ret_addr_local = local_no; }
    int  jsr_return_address_local() const          { return _jsr_ret_addr_local; }
    // Must be called after scope is set up for jsr ScopeData
    void setup_jsr_xhandlers();

    // The jsr continuation is only used when parsing_jsr is true, and
    // is different from the "normal" continuation since we can end up
    // doing a return (rather than a ret) from within a subroutine
    BlockBegin* jsr_continuation() const           { return _jsr_continuation;  }
    void set_jsr_continuation(BlockBegin* cont)    { _jsr_continuation = cont;  }

    int num_returns();
    void incr_num_returns();

    void set_inline_cleanup_info(BlockBegin* block,
                                 Instruction* return_prev,
                                 ValueStack* return_state);
    BlockBegin*  inline_cleanup_block() const      { return _cleanup_block; }
    Instruction* inline_cleanup_return_prev() const{ return _cleanup_return_prev; }
    ValueStack*  inline_cleanup_state() const      { return _cleanup_state; }
  };

  // for all GraphBuilders
  static bool       _is_initialized;             // true if trap tables were initialized, false otherwise
  static bool       _can_trap[Bytecodes::number_of_java_codes];
  static bool       _is_async[Bytecodes::number_of_java_codes];

  // for each instance of GraphBuilder
  ScopeData*        _scope_data;                 // Per-scope data; used for inlining
  Compilation*      _compilation;                // the current compilation
  ValueMap*         _vmap;                       // the map of values encountered (for CSE)
  const char*       _bailout_msg;                // non-null if GraphBuilder bailed out, sticky
  const char*       _inline_bailout_msg;         // non-null if most recent inline attempt failed
  int               _instruction_count;          // for bailing out in pathological jsr/ret cases

  // for each call to connect_to_end; can also be set by inliner
  BlockBegin*       _block;                      // the current block
  ValueStack*       _state;                      // the current execution state
  ExceptionScope*   _exception_scope;            // exception handlers covering current bci
  Instruction*      _last;                       // the last instruction added

  // accessors
  ScopeData*        scope_data() const           { return _scope_data; }
  Compilation*      compilation() const          { return _compilation; }
  BlockList*        bci2block() const            { return scope_data()->bci2block(); }
  ValueMap*         vmap() const                 { return _vmap; }
  bool              has_handler() const          { return scope_data()->has_handler(); }

  BlockBegin*       block() const                { return _block; }
  ValueStack*       state() const                { return _state; }
  void              set_state(ValueStack* state) { _state = state; }
  IRScope*          scope() const                { return scope_data()->scope(); }
  ExceptionScope*   exception_scope() const      { return _exception_scope; }
  void              push_exception_scope();
  void              pop_exception_scope();
  ciMethod*         method() const               { return scope()->method(); }
  ciBytecodeStream* stream() const               { return scope_data()->stream(); }
  Instruction*      last() const                 { return _last; }
  Bytecodes::Code   code() const                 { return stream()->code(); }
  int               bci() const                  { return stream()->bci(); }
  int               next_bci() const             { return stream()->next_bci(); }
  
  // stack manipulation helpers
  void ipush(Value t) const                      { state()->ipush(t); }
  void lpush(Value t) const                      { state()->lpush(t); }
  void fpush(Value t) const                      { state()->fpush(t); }
  void dpush(Value t) const                      { state()->dpush(t); }
  void apush(Value t) const                      { state()->apush(t); }
  void  push(ValueType* type, Value t) const     { state()-> push(type, t); }

  Value ipop() const                             { return state()->ipop(); }
  Value lpop() const                             { return state()->lpop(); }
  Value fpop() const                             { return state()->fpop(); }
  Value dpop() const                             { return state()->dpop(); }
  Value apop() const                             { return state()->apop(); }
  Value  pop(ValueType* type) const              { return state()-> pop(type); }

  // instruction helpers
  void load_constant();
  void load_local(ValueType* type, int index);
  void store_local(ValueType* type, int index);
  void store_local(ValueStack* state, Value value, ValueType* type, int index, bool is_inline_argument = false);
  void load_indexed (BasicType type);
  void store_indexed(BasicType type);
  void stack_op(Bytecodes::Code code);
  void arithmetic_op(ValueType* type, Bytecodes::Code code, ValueStack* lock_stack = NULL);
  void negate_op(ValueType* type);
  void shift_op(ValueType* type, Bytecodes::Code code);
  void logic_op(ValueType* type, Bytecodes::Code code);
  void compare_op(ValueType* type, Bytecodes::Code code);
  void convert(Bytecodes::Code op, BasicType from, BasicType to);
  void increment();
  void if_node(Value x, If::Condition cond, Value y, ValueStack* stack_before);
  void if_zero(ValueType* type, If::Condition cond);
  void if_null(ValueType* type, If::Condition cond);
  void if_same(ValueType* type, If::Condition cond);
  void jsr(int dest);
  void ret(int local_index);
  void table_switch();
  void lookup_switch();
  void method_return(Value x);
  void access_field(Bytecodes::Code code);
  void invoke(Bytecodes::Code code);
  void new_instance(int klass_index);
  void new_type_array();
  void new_object_array();
  void check_cast(int klass_index);
  void instance_of(int klass_index);
  void monitorenter(Value x);
  void monitorexit(Value x);
  void new_multi_array(int dimensions);
  void throw_op();

  // stack/code manipulation helpers
  Instruction* append_base(Instruction* instr);
  Instruction* append(Instruction* instr);
  Instruction* append_split(StateSplit* instr);

  // other helpers
  static bool is_initialized()                   { return _is_initialized; }
  static bool can_trap(Bytecodes::Code code) {
    assert(0 <= code && code < Bytecodes::number_of_java_codes, "illegal bytecode");
    return _can_trap[code];
  }
  static bool is_async(Bytecodes::Code code) {
    assert(0 <= code && code < Bytecodes::number_of_java_codes, "illegal bytecode");
    return _is_async[code];
  }
  void bailout(const char* msg);
  BlockBegin* block_at(int bci)                  { return scope_data()->block_at(bci); }
  void handle_exception(bool is_async);
  BlockEnd* connect_to_end(BlockBegin* beg);
  // Assumes _block, _state, _method, etc. have already been set up
  BlockEnd* iterate_bytecodes_for_block(int bci);
  void iterate_all_blocks(bool start_in_current_block_for_inlining = false);

  void add_dependent(ciInstanceKlass* klass, ciMethod* method);
  bool direct_compare(ciKlass* k);

  ValueStack* lock_stack();

  //
  // Inlining support
  //

  // accessors
  bool parsing_jsr() const                               { return scope_data()->parsing_jsr();           }
  BlockBegin* continuation() const                       { return scope_data()->continuation();          }
  BlockBegin* jsr_continuation() const                   { return scope_data()->jsr_continuation();      }
  int caller_stack_size() const                          { return scope_data()->caller_stack_size();     }
  void set_continuation(BlockBegin* continuation)        { scope_data()->set_continuation(continuation); }
  void set_inline_cleanup_info(BlockBegin* block,
                               Instruction* return_prev,
                               ValueStack* return_state) { scope_data()->set_inline_cleanup_info(block,
                                                                                                  return_prev,
                                                                                                  return_state); }
  BlockBegin*  inline_cleanup_block() const              { return scope_data()->inline_cleanup_block();  }
  Instruction* inline_cleanup_return_prev() const        { return scope_data()->inline_cleanup_return_prev(); }
  ValueStack*  inline_cleanup_state() const              { return scope_data()->inline_cleanup_state();  }
  void incr_num_returns()                                { scope_data()->incr_num_returns();             }
  int  num_returns() const                               { return scope_data()->num_returns();           }
  intx max_inline_size() const                           { return scope_data()->max_inline_size();       }
  int  inline_level() const                              { return scope()->level();                      }
  int  recursive_inline_level(ciMethod* callee) const;

  // inliners
  bool try_inline(ciMethod* callee, bool using_cha);
  bool try_inline_intrinsics(ciMethod* callee);
  bool try_inline_full      (ciMethod* callee, bool using_cha);
  bool try_inline_jsr(int jsr_dest_bci);

  // helpers
  void inline_bailout(const char* msg);
  void clear_inline_bailout();
  void push_root_scope(IRScope* scope, BlockList* bci2block, BlockBegin* start);
  void push_scope(ciMethod* callee, BlockBegin* continuation, bool using_cha);
  void push_scope_for_jsr(BlockBegin* jsr_continuation, int jsr_dest_bci);
  void pop_scope();
  void pop_scope_for_jsr();

  bool append_unsafe_get_obj32(ciMethod* callee, BasicType t);
  bool append_unsafe_put_obj32(ciMethod* callee, BasicType t);
  bool append_unsafe_get_obj(ciMethod* callee, BasicType t, bool is_volatile);
  bool append_unsafe_put_obj(ciMethod* callee, BasicType t, bool is_volatile);
  bool append_unsafe_get_raw(ciMethod* callee, BasicType t);
  bool append_unsafe_put_raw(ciMethod* callee, BasicType t);
  void append_unsafe_CAS(ciMethod* callee);

  NOT_PRODUCT(void print_inline_result(ciMethod* callee, bool res);)

 public:
  NOT_PRODUCT(void print_stats();)

  // initialization
  static void initialize();

  // creation
  GraphBuilder(Compilation* compilation, IRScope* scope, BlockList* bci2block, BlockBegin* start);
  bool bailed_out() const                        { return _bailout_msg != NULL; }
  const char* bailout_msg() const                { return _bailout_msg; }
};

