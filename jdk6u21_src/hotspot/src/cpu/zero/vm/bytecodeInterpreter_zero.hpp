/*
 * Copyright (c) 2002, Oracle and/or its affiliates. All rights reserved.
 * Copyright 2007, 2008 Red Hat, Inc.
 * ORACLE PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */

// Platform specific for C++ based Interpreter

#if defined(PPC) || defined(SPARC) || defined(IA64)
#define LOTS_OF_REGS   // Use plenty of registers
#else
#undef LOTS_OF_REGS    // Loser platforms
#endif

 private:
  interpreterState _self_link;

 public:
  inline void set_locals(intptr_t* new_locals) {
    _locals = new_locals;
  }
  inline void set_method(methodOop new_method) {
    _method = new_method;
  }
  inline interpreterState self_link() {
    return _self_link;
  }
  inline void set_self_link(interpreterState new_self_link) {
    _self_link = new_self_link;
  }
  inline interpreterState prev_link() {
    return _prev_link;
  }
  inline void set_prev_link(interpreterState new_prev_link) {
    _prev_link = new_prev_link;
  }
  inline void set_stack_limit(intptr_t* new_stack_limit) {
    _stack_limit = new_stack_limit;
  }
  inline void set_stack_base(intptr_t* new_stack_base) {
    _stack_base = new_stack_base;
  }
  inline void set_monitor_base(BasicObjectLock *new_monitor_base) {
    _monitor_base = new_monitor_base;
  }
  inline void set_thread(JavaThread* new_thread) {
    _thread = new_thread;
  }
  inline void set_constants(constantPoolCacheOop new_constants) {
    _constants = new_constants;
  }
  inline oop oop_temp() {
    return _oop_temp;
  }
  inline oop *oop_temp_addr() {
    return &_oop_temp;
  }
  inline void set_oop_temp(oop new_oop_temp) {
    _oop_temp = new_oop_temp;
  }
  inline address callee_entry_point() {
    return _result._to_call._callee_entry_point;
  }
  inline address osr_buf() {
    return _result._osr._osr_buf;
  }
  inline address osr_entry() {
    return _result._osr._osr_entry;
  }

 public:
  const char *name_of_field_at_address(address addr);

// The frame manager handles this
#define SET_LAST_JAVA_FRAME()
#define RESET_LAST_JAVA_FRAME()

// ZeroStack Implementation

#undef STACK_INT
#undef STACK_FLOAT
#undef STACK_ADDR
#undef STACK_OBJECT
#undef STACK_DOUBLE
#undef STACK_LONG

#define GET_STACK_SLOT(offset)    (*((intptr_t*) &topOfStack[-(offset)]))
#define STACK_SLOT(offset)    ((address) &topOfStack[-(offset)])
#define STACK_ADDR(offset)    (*((address *) &topOfStack[-(offset)]))
#define STACK_INT(offset)     (*((jint*) &topOfStack[-(offset)]))
#define STACK_FLOAT(offset)   (*((jfloat *) &topOfStack[-(offset)]))
#define STACK_OBJECT(offset)  (*((oop *) &topOfStack [-(offset)]))
#define STACK_DOUBLE(offset)  (((VMJavaVal64*) &topOfStack[-(offset)])->d)
#define STACK_LONG(offset)    (((VMJavaVal64 *) &topOfStack[-(offset)])->l)

#define SET_STACK_SLOT(value, offset)   (*(intptr_t*)&topOfStack[-(offset)] = *(intptr_t*)(value))
#define SET_STACK_ADDR(value, offset)   (*((address *)&topOfStack[-(offset)]) = (value))
#define SET_STACK_INT(value, offset)    (*((jint *)&topOfStack[-(offset)]) = (value))
#define SET_STACK_FLOAT(value, offset)  (*((jfloat *)&topOfStack[-(offset)]) = (value))
#define SET_STACK_OBJECT(value, offset) (*((oop *)&topOfStack[-(offset)]) = (value))
#define SET_STACK_DOUBLE(value, offset) (((VMJavaVal64*)&topOfStack[-(offset)])->d = (value))
#define SET_STACK_DOUBLE_FROM_ADDR(addr, offset) (((VMJavaVal64*)&topOfStack[-(offset)])->d =  \
                                                 ((VMJavaVal64*)(addr))->d)
#define SET_STACK_LONG(value, offset)   (((VMJavaVal64*)&topOfStack[-(offset)])->l = (value))
#define SET_STACK_LONG_FROM_ADDR(addr, offset)   (((VMJavaVal64*)&topOfStack[-(offset)])->l =  \
                                                 ((VMJavaVal64*)(addr))->l)
// JavaLocals implementation

#define LOCALS_SLOT(offset)    ((intptr_t*)&locals[-(offset)])
#define LOCALS_ADDR(offset)    ((address)locals[-(offset)])
#define LOCALS_INT(offset)     (*((jint*)&locals[-(offset)]))
#define LOCALS_FLOAT(offset)   (*((jfloat*)&locals[-(offset)]))
#define LOCALS_OBJECT(offset)  ((oop)locals[-(offset)])
#define LOCALS_DOUBLE(offset)  (((VMJavaVal64*)&locals[-((offset) + 1)])->d)
#define LOCALS_LONG(offset)    (((VMJavaVal64*)&locals[-((offset) + 1)])->l)
#define LOCALS_LONG_AT(offset) (((address)&locals[-((offset) + 1)]))
#define LOCALS_DOUBLE_AT(offset) (((address)&locals[-((offset) + 1)]))

#define SET_LOCALS_SLOT(value, offset)    (*(intptr_t*)&locals[-(offset)] = *(intptr_t *)(value))
#define SET_LOCALS_ADDR(value, offset)    (*((address *)&locals[-(offset)]) = (value))
#define SET_LOCALS_INT(value, offset)     (*((jint *)&locals[-(offset)]) = (value))
#define SET_LOCALS_FLOAT(value, offset)   (*((jfloat *)&locals[-(offset)]) = (value))
#define SET_LOCALS_OBJECT(value, offset)  (*((oop *)&locals[-(offset)]) = (value))
#define SET_LOCALS_DOUBLE(value, offset)  (((VMJavaVal64*)&locals[-((offset)+1)])->d = (value))
#define SET_LOCALS_LONG(value, offset)    (((VMJavaVal64*)&locals[-((offset)+1)])->l = (value))
#define SET_LOCALS_DOUBLE_FROM_ADDR(addr, offset) (((VMJavaVal64*)&locals[-((offset)+1)])->d = \
                                                  ((VMJavaVal64*)(addr))->d)
#define SET_LOCALS_LONG_FROM_ADDR(addr, offset) (((VMJavaVal64*)&locals[-((offset)+1)])->l = \
                                                ((VMJavaVal64*)(addr))->l)
