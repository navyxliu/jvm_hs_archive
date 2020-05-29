#ifdef USE_PRAGMA_IDENT_HDR
#pragma ident "@(#)machnode.hpp	1.182 04/05/04 13:26:17 JVM"
#endif
/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL.  Use is subject to license terms.
 */

class BufferBlob;
class CodeBuffer;
class JVMState;
class MachCallCompiledJavaNode;
class MachCallDynamicJavaNode;
class MachCallInterpreterNode;
class MachCallJavaNode;
class MachCallLeafNode;
class MachCallNode;
class MachCallRuntimeNode;
class MachCallStaticJavaNode;
class MachEpilogNode;
class MachIfNode;
class MachNullCheckNode;
class MachOper;
class MachProjNode;
class MachPrologNode;
class MachReturnNode;
class MachSafePointNode;
class MachSpillCopyNode;
class Matcher;
class PhaseRegAlloc;
class RegMask;
class State;

//---------------------------MachOper------------------------------------------
class MachOper : public ResourceObj {
public:
  // Allocate right next to the MachNodes in the same arena
  void *operator new( size_t x ) { return Compile::current()->node_arena()->Amalloc(x); }

  // Opcode 
  virtual uint opcode() const = 0;

  // Number of input edges
  virtual uint num_edges() const = 0;
  // Array of Register masks 
  virtual const RegMask *in_RegMask(int index) const;

  // Methods to output the encoding of the operand

  // Negate conditional branches.  Error for non-branch Nodes
  virtual void negate();

  // Return the value requested
  // result register lookup, corresponding to int_format
  virtual int  reg(PhaseRegAlloc *ra_, const Node *node)   const;
  // input register lookup, corresponding to ext_format
  virtual int  reg(PhaseRegAlloc *ra_, const Node *node, int idx)   const;
  virtual intptr_t  constant() const;
  virtual bool constant_is_oop() const;
  virtual jdouble constantD() const;
  virtual jfloat  constantF() const;
  virtual jlong   constantL() const;
  virtual TypeOopPtr *oop() const;
  virtual int  ccode() const;
  // A zero, default, indicates this value is not needed.
  // May need to lookup the base register, as done in int_ and ext_format
  virtual int  base (PhaseRegAlloc *ra_, const Node *node, int idx) const;
  virtual int  index(PhaseRegAlloc *ra_, const Node *node, int idx) const;
  virtual int  scale() const;
  // Parameters needed to support MEMORY_INTERFACE access to stackSlot
  virtual int  disp (PhaseRegAlloc *ra_, const Node *node, int idx) const;
  // Check for PC-Relative displacement
  virtual bool disp_is_oop() const;
  virtual int  constant_disp() const;   // usu. 0, may return Type::OffsetBot
  virtual int  base_position()  const;  // base edge position, or -1
  virtual int  index_position() const;  // index edge position, or -1

  // Access the TypeKlassPtr of operands with a base==RegI and disp==RegP
  // Only returns non-null value for i486.ad's indOffset32X 
  virtual const TypePtr *disp_as_type() const { return NULL; }

  // Return the label
  virtual Label *label() const;

  // Return the method's address
  virtual intptr_t  method() const;

  // Hash and compare over operands are currently identical
  virtual uint  hash() const;
  virtual uint  cmp( const MachOper &oper ) const;

  // Virtual clone, since I do not know how big the MachOper is.
  virtual MachOper *clone() const = 0;

  // Return ideal Type from simple operands.  Fail for complex operands.
  virtual const Type *type() const;

  // Set an integer offset if we have one, or error otherwise
  virtual void set_con( jint c0 ) { ShouldNotReachHere();  }

#ifndef PRODUCT
  // Return name of operand
  virtual const char    *Name() const { return "???";}

  // Methods to output the text version of the operand
  virtual void int_format(PhaseRegAlloc *,const MachNode *node) const = 0;
  virtual void ext_format(PhaseRegAlloc *,const MachNode *node,int idx) const=0;

  virtual void dump_spec() const; // Print per-operand info
#endif
};

//------------------------------MachNode---------------------------------------
// Base type for all machine specific nodes.  All node classes generated by the
// ADLC inherit from this class.
class MachNode : public Node {
public:
  MachNode();
  // Required boilerplate
  virtual uint size_of() const { return sizeof(MachNode); }
  virtual int  Opcode() const;          // Always equal to MachNode
  virtual uint rule() const = 0;        // Machine-specific opcode
  // Number of inputs which come before the first operand.
  // Generally at least 1, to skip the Control input
  virtual uint oper_input_base() const = 0;

  // Return operand position that can convert from reg to memory access
  virtual int  cisc_operand() const { return AdlcVMDeps::Not_cisc_spillable; }
  // Return an equivalent instruction using memory for cisc_operand position
  virtual MachNode *cisc_version(int offset);
  // Modify this instruction's register mask to use stack version for cisc_operand
  virtual void use_cisc_RegMask();

  // Support for short branches
  virtual MachNode *short_branch_version() { return NULL; }
  virtual bool      may_be_short_branch() const { return false; }

  // First index in _in[] corresponding to operand, or -1 if there is none
  int  operand_index(uint operand) const;

  // Register class input is expected in
  virtual const RegMask &in_RegMask(uint) const;
  virtual const RegMask *in_oper_RegMask(uint idx, uint operand, uint skipped) const;

  // If this instruction is a 2-address instruction, then return the
  // index of the input which must match the output.  Not nessecary 
  // for instructions which bind the input and output register to the
  // same singleton regiser (e.g., Intel IDIV which binds AX to be
  // both an input and an output).  It is nessecary when the input and
  // output have choices - but they must use the same choice.
  virtual uint two_adr( ) const { return 0; }

  // Array of complex operand pointers.  Each corresponds to zero or
  // more leafs.  Must be set by MachNode constructor to point to an
  // internal array of MachOpers.  The MachOper array is sized by
  // specific MachNodes described in the ADL.
  MachOper **_opnds;
  virtual uint  num_opnds() const = 0;

  // Emit bytes into cbuf
  virtual void  emit(CodeBuffer &cbuf, PhaseRegAlloc *ra_) const;
  // Size of instruction in bytes
  virtual uint  size(PhaseRegAlloc *ra_) const;
  // Helper function that computes size by emitting code
  virtual uint  emit_size(PhaseRegAlloc *ra_) const;

  // Return the alignment required (in units of relocInfo::addr_unit())
  // for this instruction (must be a power of 2)
  virtual int   alignment_required() const { return 1; }

  // Return the padding (in bytes) to be emitted before this
  // instruction to properly align it.
  virtual int   compute_padding(int current_offset) const { return 0; }

  // Return number of relocatable values contained in this instruction
  virtual int   reloc() const { return 0; }

  // Return number of words used for double constants in this instruction
  virtual int   const_size() const { return 0; }

  // Hash and compare over operands.  Used to do GVN on machine Nodes.
  virtual uint  hash() const;
  virtual uint  cmp( const Node &n ) const;

  // Check for being a MachNode.  The 'inMach' call does an explicit cast to
  // avoid the virtual downcast call at runtime.
  virtual MachNode *is_Mach() { return this; }
  MachNode *inMach(int idx) { assert( in(idx)->is_Mach(), "" ); return (MachNode*)in(idx); }

  // Expand method for MachNode, replaces nodes representing pseudo
  // instructions with a set of nodes which represent real machine
  // instructions and compute the same value.
  virtual MachNode *Expand( State *, Node_List &proj_list ) { return this; }

  // Bottom_type call; value comes from operand0
  virtual const class Type *bottom_type() const { return _opnds[0]->type(); }
  virtual uint ideal_reg() const { const Type *t = _opnds[0]->type(); return t == TypeInt::CC ? Op_RegFlags : Matcher::base2reg[t->base()]; }

  // If this is a memory op, return the base pointer and fixed offset.
  // If there are no such, return NULL.  If there are multiple addresses
  // or the address is indeterminate (rare cases) then return (Node*)-1, 
  // which serves as node bottom.
  // If the offset is not statically determined, set it to Type::OffsetBot.
  // This method is free to ignore stack slots if that helps.
  #define TYPE_PTR_SENTINAL  ((const TypePtr*)-1)
  // Passing TYPE_PTR_SENTINAL as adr_type asks for computation of the adr_type if possible
  const Node* get_base_and_disp(intptr_t &offset, const TypePtr* &adr_type) const;

  // Helper for get_base_and_disp:  Which operand carries the necessary info?
  // By default, returns NULL, which means there is no such operand.
  // If it returns (MachOper*)-1, this means there are multiple memories.
  virtual const MachOper* memory_operand() const { return NULL; }

  // Call "get_base_and_disp" to decide which category of memory is used here.
  virtual const class TypePtr *adr_type() const;

  // Negate conditional branches.  Error for non-branch Nodes
  virtual void negate();

  // Apply peephole rule(s) to this instruction
  virtual MachNode *peephole( Block *block, int block_index, PhaseRegAlloc *ra_, int &deleted );

  // Check for PC-Relative addressing
  virtual bool is_pc_relative() const { return false; }

  // Top-level ideal Opcode matched
  virtual int ideal_Opcode() const = 0;

  // Set the branch inside jump MachNodes.  Error for non-branch Nodes.
  virtual void label_set( Label& label, uint block_num );

  // Returns the label at the switch value
  virtual Label *label_for_case(int switch_val) const;
  
  // Adds the label for the case
  virtual void add_case_label( int switch_val, Label* blockLabel);

  // Set the absolute address for methods (and LoadPC)
  virtual void method_set( intptr_t addr );

  // Should we clone rather than spill this instruction?
  virtual bool rematerialize() const;

  // Get the pipeline info
  static const Pipeline *pipeline_class();
  virtual const Pipeline *pipeline() const;

  // Check for being a special Node type
  virtual const MachIfNode *is_MachIf() const { return 0; }
  virtual const MachTypeNode *is_MachType() const { return 0; }
  virtual MachReturnNode *is_MachReturn() { return 0; }
  virtual MachSafePointNode *is_MachSafePoint() { return 0; }
  virtual MachCallNode *is_MachCall() { return 0; }
  virtual MachCallRuntimeNode *is_MachCallRuntime() { return NULL; }
  virtual MachNullCheckNode *is_MachNullCheck() { return NULL; }
  virtual MachPrologNode *is_MachProlog() { return NULL; }
  virtual MachEpilogNode *is_MachEpilog() { return NULL; }
#ifndef PRODUCT
  virtual const char *Name() const = 0; // Machine-specific name
  virtual void dump_spec() const; // Print per-node info
  void         dump_format(PhaseRegAlloc *ra) const; // access to virtual
#endif
};

//------------------------------MachIdealNode----------------------------
// Machine specific versions of nodes that must be defined by user.
// These are not converted by matcher from ideal nodes to machine nodes
// but are inserted into the code by the compiler.
class MachIdealNode : public MachNode {
public:
  MachIdealNode( ) {}

  // Define the following defaults for non-matched machine nodes
  virtual uint oper_input_base() const { return 0; }
  virtual uint rule()            const { return 9999999; }
  virtual uint num_opnds()       const { return 0; }
  virtual int ideal_Opcode()     const { return Op_Node; }
  virtual const class Type *bottom_type() const { return _opnds == NULL ? Type::CONTROL : MachNode::bottom_type(); }
};

//------------------------------MachTypeNode----------------------------
// Machine Nodes that need to retain a known Type.
class MachTypeNode : public MachNode {
  virtual uint size_of() const { return sizeof(*this); } // Size is bigger
public:
  const Type *_bottom_type;

  virtual const MachTypeNode *is_MachType() const { return this; }
  virtual const class Type *bottom_type() const { return _bottom_type; }
#ifndef PRODUCT
  virtual void dump_spec() const;
#endif
};

//------------------------------MachBreakpointNode----------------------------
// Machine breakpoint or interrupt Node
class MachBreakpointNode : public MachIdealNode {
public:
  MachBreakpointNode( ) {}
  virtual void emit(CodeBuffer &cbuf, PhaseRegAlloc *ra_) const;
  virtual uint size(PhaseRegAlloc *ra_) const;

#ifndef PRODUCT
  virtual const char *Name() const { return "Breakpoint"; }
  virtual void format( PhaseRegAlloc * ) const;
#endif
};

//------------------------------MachUEPNode-----------------------------------
// Machine Unvalidated Entry Point Node
class MachUEPNode : public MachIdealNode {
public:
  MachUEPNode( ) {}
  virtual void emit(CodeBuffer &cbuf, PhaseRegAlloc *ra_) const;
  virtual uint size(PhaseRegAlloc *ra_) const;

#ifndef PRODUCT
  virtual const char *Name() const { return "Unvalidated-Entry-Point"; }
  virtual void format( PhaseRegAlloc * ) const;
#endif
};

//------------------------------MachC2IEntriesNode----------------------------
// Machine Entry points for the Compiled To Interpreter adapters
class MachC2IEntriesNode : public MachIdealNode {
public:
  MachC2IEntriesNode( ) {}
  virtual void emit(CodeBuffer &cbuf, PhaseRegAlloc *ra_) const;
  virtual uint size(PhaseRegAlloc *ra_) const;

#ifndef PRODUCT
  virtual const char *Name() const { return "MachC2IEntries Entry Points"; }
  virtual void format( PhaseRegAlloc * ) const;
#endif
};

//------------------------------MachC2IcheckICNode----------------------------
// Update inline cache, if compiled code exist and call directly to compiled code
class MachC2IcheckICNode: public MachIdealNode {
public:
  MachC2IcheckICNode( ) {}
  virtual void emit(CodeBuffer &cbuf, PhaseRegAlloc *ra_) const;
  virtual uint size(PhaseRegAlloc *ra_) const;

#ifndef PRODUCT
  virtual const char *Name() const { return "MachC2IcheckICNode"; }
  virtual void format( PhaseRegAlloc * ) const;
#endif
};

//------------------------------MachPrologNode--------------------------------
// Machine function Prolog Node
class MachPrologNode : public MachIdealNode {
public:
  MachPrologNode( ) {}
  virtual void emit(CodeBuffer &cbuf, PhaseRegAlloc *ra_) const;
  virtual uint size(PhaseRegAlloc *ra_) const;
  virtual int reloc() const;
  virtual MachPrologNode *is_MachProlog() { return this; }
  uint implementation( CodeBuffer *cbuf, PhaseRegAlloc *ra_, bool do_size ) const;

#ifndef PRODUCT
  virtual const char *Name() const { return "Prolog"; }
  virtual void format( PhaseRegAlloc * ) const;
#endif
};

//------------------------------MachEpilogNode--------------------------------
// Machine function Epilog Node
class MachEpilogNode : public MachIdealNode {
public:
  MachEpilogNode(bool do_poll = false) : _do_polling(do_poll) {}
  virtual void emit(CodeBuffer &cbuf, PhaseRegAlloc *ra_) const;
  virtual uint size(PhaseRegAlloc *ra_) const;
  virtual int reloc() const;
  virtual MachEpilogNode *is_MachEpilog() { return this; }
  virtual const Pipeline *pipeline() const;

private:
  bool _do_polling;

public:
  bool do_polling() const { return _do_polling; }

  // Offset of safepoint from the beginning of the node
  int safepoint_offset() const;

#ifndef PRODUCT
  virtual const char *Name() const { return "Epilog"; }
  virtual void format( PhaseRegAlloc * ) const;
#endif
};

//------------------------------MachNopNode-----------------------------------
// Machine function Nop Node
class MachNopNode : public MachIdealNode {
public:
  MachNopNode( ) {}
  virtual void emit(CodeBuffer &cbuf, PhaseRegAlloc *ra_) const;
  virtual uint size(PhaseRegAlloc *ra_) const;

  virtual const class Type *bottom_type() const { return Type::CONTROL; }

  virtual int ideal_Opcode() const { return Op_Con; } // bogus; see output.cpp
  virtual const Pipeline *pipeline() const;
#ifndef PRODUCT
  virtual const char *Name() const { return "Nop"; }
  virtual void format( PhaseRegAlloc * ) const;
  virtual void dump_spec() const { } // No per-operand info
#endif
};

//------------------------------MachSpillCopyNode------------------------------
// Machine SpillCopy Node.  Copies 1 or 2 words from any location to any
// location (stack or register).
class MachSpillCopyNode : public MachIdealNode {
  const RegMask *_in;           // RegMask for input
  const RegMask *_out;          // RegMask for output
  const Type *_type;
public:
  MachSpillCopyNode( Node *n, const RegMask &in, const RegMask &out ) : 
    MachIdealNode(), _in(&in), _out(&out), _type(n->bottom_type()) { add_req(NULL); add_req(n); }
  virtual MachSpillCopyNode *is_SpillCopy() { return this; }
  virtual uint is_Copy() const { return 1; }
  virtual uint size_of() const { return sizeof(*this); } 
  void set_out_RegMask(const RegMask &out) { _out = &out; }
  void set_in_RegMask(const RegMask &in) { _in = &in; }
  virtual const RegMask &out_RegMask() const { return *_out; }
  virtual const RegMask &in_RegMask(uint) const { return *_in; }
  virtual const class Type *bottom_type() const { return _type; }
  virtual uint ideal_reg() const { return Matcher::base2reg[_type->base()]; }
  virtual uint oper_input_base() const { return 1; }
  uint implementation( CodeBuffer *cbuf, PhaseRegAlloc *ra_, bool do_size ) const;

  virtual void emit(CodeBuffer &cbuf, PhaseRegAlloc *ra_) const;
  virtual uint size(PhaseRegAlloc *ra_) const;

#ifndef PRODUCT
  virtual const char *Name() const { return "MachSpillCopy"; }
  virtual void format( PhaseRegAlloc * ) const;
#endif
};

//------------------------------MachNullChkNode--------------------------------
// Machine-dependent null-pointer-check Node.  Points a real MachNode that is
// also some kind of memory op.  Turns the indicated MachNode into a
// conditional branch with good latency on the ptr-not-null path and awful
// latency on the pointer-is-null path.  

class MachNullCheckNode : public MachIdealNode {
public:
  const uint _vidx;             // Index of memop being tested
  MachNullCheckNode( Node *ctrl, Node *memop, uint vidx );

  virtual void emit(CodeBuffer &cbuf, PhaseRegAlloc *ra_) const;
  virtual uint is_Branch() const { return 1; }
  virtual int pinned() const { return 1; };
  virtual bool is_pc_relative() const { return true; }
  virtual void negate() { }
  virtual const class Type *bottom_type() const { return TypeTuple::IFBOTH; }
  virtual uint ideal_reg() const { return NotAMachineReg; }
  virtual MachNullCheckNode *is_MachNullCheck() { return this; }
  virtual const RegMask &in_RegMask(uint) const;
  virtual const RegMask &out_RegMask() const { return RegMask::Empty; }
#ifndef PRODUCT
  virtual const char *Name() const { return "NullCheck"; }
  virtual void format( PhaseRegAlloc * ) const;
#endif
};

//------------------------------MachProjNode----------------------------------
// Machine-dependent Ideal projections (how is that for an oxymoron).  Really
// just MachNodes made by the Ideal world that replicate simple projections
// but with machine-dependent input & output register masks.  Generally
// produced as part of calling conventions.  Normally I make MachNodes as part
// of the Matcher process, but the Matcher is ill suited to issues involving
// frame handling, so frame handling is all done in the Ideal world with
// occasional callbacks to the machine model for important info.
class MachProjNode : public ProjNode {
public:
  MachProjNode( Node *multi, uint con, const RegMask &out, uint ideal_reg ) : ProjNode(multi,con), _rout(out), _ideal_reg(ideal_reg) {}
  RegMask _rout;
  const uint  _ideal_reg;
  enum projType {
    unmatched_proj = 0,         // Projs for Control, I/O, memory not matched
    fat_proj       = 999        // Projs killing many regs, defined by _rout
  };
  virtual int   Opcode() const;
  virtual const Type *bottom_type() const;
  virtual const TypePtr *adr_type() const;
  virtual const RegMask &in_RegMask(uint) const { return RegMask::Empty; }
  virtual const RegMask &out_RegMask() const { return _rout; }
  virtual uint  ideal_reg() const { return _ideal_reg; }
  // Need size_of() for virtual ProjNode::clone() 
  virtual uint  size_of() const { return sizeof(MachProjNode); }
#ifndef PRODUCT
  virtual void dump_spec() const;
#endif
};

//------------------------------MachIfNode-------------------------------------
// Machine-specific versions of IfNodes
class MachIfNode : public MachNode {
  virtual uint size_of() const { return sizeof(*this); } // Size is bigger
public:
  float _prob;                  // Probability branch goes either way
  float _fcnt;                  // Frequency counter
  virtual const MachIfNode *is_MachIf() const { return this; }
#ifndef PRODUCT
  virtual void dump_spec() const;
#endif
};

//------------------------------MachReturnNode--------------------------------
// Machine-specific versions of subroutine returns
class MachReturnNode : public MachNode {
  virtual uint size_of() const; // Size is bigger
public:
  RegMask *_in_rms;             // Input register masks, set during allocation
  ReallocMark _nesting;         // assertion check for reallocations

  virtual const RegMask &in_RegMask(uint) const;
  virtual int pinned() const { return 1; };
  virtual MachReturnNode *is_MachReturn() { return this; }
  virtual const TypePtr *adr_type() const;
};

//------------------------------MachSafePointNode-----------------------------
// Machine-specific versions of safepoints
class MachSafePointNode : public MachReturnNode {
public:
  OopMap*         _oop_map;     // Array of OopMap info (8-bit char) for GC
  JVMState*       _jvms;        // Pointer to list of JVM State Objects
  uint            _jvmadj;      // Extra delta to jvms indexes (mach. args)
  OopMap*         oop_map() const { return _oop_map; }
  void            set_oop_map(OopMap* om) { _oop_map = om; }

  MachSafePointNode();

  virtual JVMState* jvms() const { return _jvms; }
  void set_jvms(JVMState* s) {
    _jvms = s;
  }
  virtual bool           is_safepoint_node() const { return true; }
  virtual const Type    *bottom_type() const;
  virtual int   pinned() const { return 1; }


  virtual const RegMask &in_RegMask(uint) const;
  virtual MachSafePointNode        *is_MachSafePoint() { return this; }
  virtual MachCallNode             *is_MachCall() { return NULL; }
  virtual MachCallInterpreterNode  *is_MachCallInterpreter() { return 0; }
  virtual MachCallCompiledJavaNode *is_MachCallCompiledJava() { return 0; }
  virtual MachCallRuntimeNode      *is_MachCallRuntime() { return 0; }
  virtual MachCallLeafNode         *is_MachCallLeaf() { return 0; }
  virtual MachCallJavaNode         *is_MachCallJava()    { return 0; }

  // Functionality from old debug nodes
  Node *returnadr() const { return in(TypeFunc::ReturnAdr); }
  Node *frameptr () const { return in(TypeFunc::FramePtr); }

  Node *local(const JVMState* jvms, uint idx) const {
    assert(verify_jvms(jvms), "jvms must match");
    return in(_jvmadj + jvms->locoff() + idx);
  }
  Node *stack(const JVMState* jvms, uint idx) const {
    assert(verify_jvms(jvms), "jvms must match");
    return in(_jvmadj + jvms->stkoff() + idx);
 }
  Node *monitor_obj(const JVMState* jvms, uint idx) const {
    assert(verify_jvms(jvms), "jvms must match");
    return in(_jvmadj + jvms->monitor_obj_offset(idx));
  }
  Node *monitor_box(const JVMState* jvms, uint idx) const {
    assert(verify_jvms(jvms), "jvms must match");
    return in(_jvmadj + jvms->monitor_box_offset(idx));
  }
  void  set_local(const JVMState* jvms, uint idx, Node *c) {
    assert(verify_jvms(jvms), "jvms must match");
    set_req(_jvmadj + jvms->locoff() + idx, c);
  }
  void  set_stack(const JVMState* jvms, uint idx, Node *c) {
    assert(verify_jvms(jvms), "jvms must match");
    set_req(_jvmadj + jvms->stkoff() + idx, c);
  }
  void  set_monitor(const JVMState* jvms, uint idx, Node *c) {
    assert(verify_jvms(jvms), "jvms must match");
    set_req(_jvmadj + jvms->monoff() + idx, c);
  }
};

//------------------------------MachCallNode----------------------------------
// Machine-specific versions of subroutine calls
class MachCallNode : public MachSafePointNode {
protected:
  virtual uint hash() const { return NO_HASH; }  // CFG nodes do not hash
  virtual uint cmp( const Node &n ) const;
  virtual uint size_of() const = 0; // Size is bigger
public:
  const TypeFunc *_tf;        // Function type
  address      _entry_point;  // Address of the method being called
  uint         _argsize;      // Size of argument block on stack

  const TypeFunc* tf()        const { return _tf; }
  const address entry_point() const { return _entry_point; }
  uint argsize()              const { return _argsize; }

  void set_tf(const TypeFunc* tf) { _tf = tf; }
  void set_entry_point(address p) { _entry_point = p; }
  void set_argsize(int s)         { _argsize = s; }

  MachCallNode();

  virtual const Type *bottom_type() const;
  virtual int   pinned() const { return 0; }
  virtual const Type *Value( PhaseTransform *phase ) const;
  virtual const RegMask &in_RegMask(uint) const;
  virtual CallNode *is_Call() { return (CallNode*)-1; }
  virtual MachCallNode *is_MachCall() { return this; }
  virtual int ret_addr_offset() { return 0; }
  virtual bool returns_float_or_double() const;
  virtual bool returns_long() const;
#ifndef PRODUCT
  virtual void dump_spec() const;
#endif
};

//------------------------------MachCallJavaNode------------------------------
// "Base" class for machine-specific versions of subroutine calls
class MachCallJavaNode : public MachCallNode {
protected:
  virtual uint hash() const { return NO_HASH; }  // CFG nodes do not hash
  virtual uint cmp( const Node &n ) const;
  virtual uint size_of() const; // Size is bigger
public:
  ciMethod* _method;             // Method being direct called
  int        _bci;               // Byte Code index of call byte code
  bool       _optimized_virtual; // Tells if node is a static call or an optimized virtual
  virtual MachCallJavaNode        *is_MachCallJava()        { return this; }
  virtual MachCallStaticJavaNode  *is_MachCallStaticJava()  { return 0; }
  virtual MachCallDynamicJavaNode *is_MachCallDynamicJava() { return 0; }
#ifndef PRODUCT
  virtual void dump_spec() const;
#endif
};

//------------------------------MachCallStaticJavaNode------------------------
// Machine-specific versions of monomorphic subroutine calls
class MachCallStaticJavaNode : public MachCallJavaNode {
  virtual uint hash() const { return NO_HASH; }  // CFG nodes do not hash
  virtual uint cmp( const Node &n ) const;
  virtual uint size_of() const; // Size is bigger
public:
  const char *_name;            // Runtime wrapper name
  virtual MachCallStaticJavaNode  *is_MachCallStaticJava() { return this; }
  virtual int ret_addr_offset();
#ifndef PRODUCT
  virtual void dump_spec() const;
#endif
};

//------------------------------MachCallDynamicJavaNode------------------------
// Machine-specific versions of possibly megamorphic subroutine calls
class MachCallDynamicJavaNode : public MachCallJavaNode {
public:
  virtual MachCallDynamicJavaNode *is_MachCallDynamicJava() { return this; }
  virtual int ret_addr_offset();
#ifndef PRODUCT
  virtual void dump_spec() const;
#endif
};

//------------------------------MachCallCompiledJavaNode-----------------------
// Machine-specific versions of subroutine calls
class MachCallCompiledJavaNode : public MachCallNode {
public:
  virtual MachCallCompiledJavaNode *is_MachCallCompiledJava() { return this; }
  virtual int ret_addr_offset();
#ifndef PRODUCT
  virtual void dump_spec() const;
#endif
};

//------------------------------MachCallInterpreterNode------------------------
// Machine-specific versions of subroutine calls
class MachCallInterpreterNode : public MachCallNode {
public:
  virtual MachCallInterpreterNode *is_MachCallInterpreter() { return this; }
  virtual int ret_addr_offset();
#ifndef PRODUCT
  virtual void dump_spec() const;
#endif
};

//------------------------------MachCallRuntimeNode----------------------------
// Machine-specific versions of subroutine calls
class MachCallRuntimeNode : public MachCallNode {
  virtual uint hash() const { return NO_HASH; }  // CFG nodes do not hash
  virtual uint cmp( const Node &n ) const;
  virtual uint size_of() const; // Size is bigger
public:
  const char *_name;            // Printable name, if _method is NULL
  virtual MachCallRuntimeNode *is_MachCallRuntime() { return this; }
  virtual int ret_addr_offset();
#ifndef PRODUCT
  virtual void dump_spec() const;
#endif
};

class MachCallLeafNode: public MachCallRuntimeNode {
public:
  virtual MachCallLeafNode *is_MachCallLeaf() { return this; }
};

//------------------------------MachCallNativeNode-----------------------------
// Machine-specific versions of subroutine calls
class MachCallNativeNode : public MachCallRuntimeNode {
public:
  virtual int ret_addr_offset();
};

//------------------------------MachHaltNode-----------------------------------
// Machine-specific versions of halt nodes
class MachHaltNode : public MachReturnNode {
public:
  virtual JVMState* jvms() const;
};

//------------------------------MachCallClearArrayNode-------------------------
// clear array data; can be implemented as inlined code
class MachCallClearArrayNode: public MachCallLeafNode {
};

//------------------------------labelOper--------------------------------------
// Machine-independent version of label operand
class labelOper : public MachOper { 
private:
  virtual uint           num_edges() const { return 0; }
public:
  // Supported for fixed size branches
  Label* _label;                // Label for branch(es)

  uint _block_num;

  labelOper() : _block_num(0), _label(0) {}

  labelOper(Label* label, uint block_num) : _label(label), _block_num(block_num) {}

  labelOper(labelOper* l) : _label(l->_label) , _block_num(l->_block_num) {}

  virtual MachOper *clone() const;

  virtual Label *label() const { return _label; }

  virtual uint           opcode() const;

  virtual uint           hash()   const;
  virtual uint           cmp( const MachOper &oper ) const;
#ifndef PRODUCT
  virtual const char    *Name()   const { return "Label";}

  virtual void int_format(PhaseRegAlloc *ra, const MachNode *node) const;
  virtual void ext_format(PhaseRegAlloc *ra, const MachNode *node, int idx) const { int_format( ra, node ); }
#endif
};


//------------------------------methodOper--------------------------------------
// Machine-independent version of method operand
class methodOper : public MachOper { 
private:
  virtual uint           num_edges() const { return 0; }
public:
  intptr_t _method;             // Address of method
  methodOper() :   _method(0) {}
  methodOper(intptr_t method) : _method(method)  {}

  virtual MachOper *clone() const;

  virtual intptr_t method() const { return _method; }

  virtual uint           opcode() const;

  virtual uint           hash()   const;
  virtual uint           cmp( const MachOper &oper ) const;
#ifndef PRODUCT
  virtual const char    *Name()   const { return "Method";}

  virtual void int_format(PhaseRegAlloc *ra, const MachNode *node) const;
  virtual void ext_format(PhaseRegAlloc *ra, const MachNode *node, int idx) const { int_format( ra, node ); }
#endif
};
