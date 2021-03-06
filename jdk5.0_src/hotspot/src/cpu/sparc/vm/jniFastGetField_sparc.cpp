#ifdef USE_PRAGMA_IDENT_HDR
#pragma ident "@(#)jniFastGetField_sparc.cpp	1.2 04/03/24 16:53:16 JVM"
#endif
/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL.  Use is subject to license terms.
 */

# include "incls/_precompiled.incl"
# include "incls/_jniFastGetField_sparc.cpp.incl"

// TSO ensures that loads are blocking and ordered with respect to
// to earlier loads, so we don't need LoadLoad membars.

#define __ masm->

#define BUFFER_SIZE 30*sizeof(jint)

// Common register usage:
// O0: env
// O1: obj
// O2: jfieldID
// O4: offset (O2 >> 2)
// G4: old safepoint counter

address JNI_FastGetField::generate_fast_get_int_field0(BasicType type) {
  const char *name;
  switch (type) {
    case T_BOOLEAN: name = "jni_fast_GetBooleanField"; break;
    case T_BYTE:    name = "jni_fast_GetByteField";    break;
    case T_CHAR:    name = "jni_fast_GetCharField";    break;
    case T_SHORT:   name = "jni_fast_GetShortField";   break;
    case T_INT:     name = "jni_fast_GetIntField";     break;
    default:        ShouldNotReachHere();
  }
  ResourceMark rm;
  BufferBlob* b = BufferBlob::create(name, BUFFER_SIZE*wordSize);
  address fast_entry = b->instructions_begin();
  CodeBuffer* cbuf = new CodeBuffer(fast_entry, b->instructions_size());
  MacroAssembler* masm = new MacroAssembler(cbuf);

  Label label1, label2;

  address cnt_addr = SafepointSynchronize::safepoint_counter_addr();
  Address ca(O3, cnt_addr);
  __ sethi (ca);
  __ ld (ca, G4);
  __ andcc (G4, 1, G0);
  __ br (Assembler::notZero, false, Assembler::pn, label1);
  __ delayed()->srl (O2, 2, O4);
  __ ld_ptr (O1, 0, O5);

  assert(count < LIST_CAPACITY, "LIST_CAPACITY too small");
  speculative_load_pclist[count] = __ pc();
  switch (type) {
    case T_BOOLEAN: __ ldub (O5, O4, G3);  break;
    case T_BYTE:    __ ldsb (O5, O4, G3);  break;
    case T_CHAR:    __ lduh (O5, O4, G3);  break;
    case T_SHORT:   __ ldsh (O5, O4, G3);  break;
    case T_INT:     __ ld (O5, O4, G3);    break;
    default:        ShouldNotReachHere();
  }

  __ ld (ca, O5);
  __ cmp (O5, G4);
  __ br (Assembler::notEqual, false, Assembler::pn, label2);
  __ delayed()->mov (O7, G1);
  __ retl ();
  __ delayed()->mov (G3, O0);

  slowcase_entry_pclist[count++] = __ pc();
  __ bind (label1);
  __ mov (O7, G1);

  address slow_case_addr;
  switch (type) {
    case T_BOOLEAN: slow_case_addr = jni_GetBooleanField_addr(); break;
    case T_BYTE:    slow_case_addr = jni_GetByteField_addr();    break;
    case T_CHAR:    slow_case_addr = jni_GetCharField_addr();    break;
    case T_SHORT:   slow_case_addr = jni_GetShortField_addr();   break;
    case T_INT:     slow_case_addr = jni_GetIntField_addr();     break;
    default:        ShouldNotReachHere();
  }
  __ bind (label2);
  __ call (slow_case_addr, relocInfo::none);
  __ delayed()->mov (G1, O7);

  __ flush ();

  return fast_entry;
}

address JNI_FastGetField::generate_fast_get_boolean_field() {
  return generate_fast_get_int_field0(T_BOOLEAN);
}

address JNI_FastGetField::generate_fast_get_byte_field() {
  return generate_fast_get_int_field0(T_BYTE);
}

address JNI_FastGetField::generate_fast_get_char_field() {
  return generate_fast_get_int_field0(T_CHAR);
}

address JNI_FastGetField::generate_fast_get_short_field() {
  return generate_fast_get_int_field0(T_SHORT);
}

address JNI_FastGetField::generate_fast_get_int_field() {
  return generate_fast_get_int_field0(T_INT);
}

address JNI_FastGetField::generate_fast_get_long_field() {
  const char *name = "jni_fast_GetLongField";
  ResourceMark rm;
  BufferBlob* b = BufferBlob::create(name, BUFFER_SIZE*wordSize);
  address fast_entry = b->instructions_begin();
  CodeBuffer* cbuf = new CodeBuffer(fast_entry, b->instructions_size());
  MacroAssembler* masm = new MacroAssembler(cbuf);

  Label label1, label2;

  address cnt_addr = SafepointSynchronize::safepoint_counter_addr();
  Address ca(G3, cnt_addr);
  __ sethi (ca);
  __ ld (ca, G4);
  __ andcc (G4, 1, G0);
  __ br (Assembler::notZero, false, Assembler::pn, label1);
  __ delayed()->srl (O2, 2, O4);
  __ ld_ptr (O1, 0, O5);
  __ add (O5, O4, O5);

#ifndef _LP64
  assert(count < LIST_CAPACITY-1, "LIST_CAPACITY too small");
  speculative_load_pclist[count++] = __ pc();
  __ ld (O5, 0, G2);

  speculative_load_pclist[count] = __ pc();
  __ ld (O5, 4, O3);
#else
  assert(count < LIST_CAPACITY, "LIST_CAPACITY too small");
  speculative_load_pclist[count] = __ pc();
  __ ldx (O5, 0, O3);
#endif

  __ ld (ca, G1);
  __ cmp (G1, G4);
  __ br (Assembler::notEqual, false, Assembler::pn, label2);
  __ delayed()->mov (O7, G1);

#ifndef _LP64
  __ mov (G2, O0);
  __ retl ();
  __ delayed()->mov (O3, O1);
#else
  __ retl ();
  __ delayed()->mov (O3, O0);
#endif

#ifndef _LP64
  slowcase_entry_pclist[count-1] = __ pc();
  slowcase_entry_pclist[count++] = __ pc() ;
#else
  slowcase_entry_pclist[count++] = __ pc();
#endif

  __ bind (label1);
  __ mov (O7, G1);

  address slow_case_addr = jni_GetLongField_addr();
  __ bind (label2);
  __ call (slow_case_addr, relocInfo::none);
  __ delayed()->mov (G1, O7);

  __ flush ();

  return fast_entry;
}

address JNI_FastGetField::generate_fast_get_float_field0(BasicType type) {
  const char *name;
  switch (type) {
    case T_FLOAT:  name = "jni_fast_GetFloatField";  break;
    case T_DOUBLE: name = "jni_fast_GetDoubleField"; break;
    default:       ShouldNotReachHere();
  }
  ResourceMark rm;
  BufferBlob* b = BufferBlob::create(name, BUFFER_SIZE*wordSize);
  address fast_entry = b->instructions_begin();
  CodeBuffer* cbuf = new CodeBuffer(fast_entry, b->instructions_size());
  MacroAssembler* masm = new MacroAssembler(cbuf);

  Label label1, label2;

  address cnt_addr = SafepointSynchronize::safepoint_counter_addr();
  Address ca(O3, cnt_addr);
  __ sethi (ca);
  __ ld (ca, G4);
  __ andcc (G4, 1, G0);
  __ br (Assembler::notZero, false, Assembler::pn, label1);
  __ delayed()->srl (O2, 2, O4);
  __ ld_ptr (O1, 0, O5);

  assert(count < LIST_CAPACITY, "LIST_CAPACITY too small");
  speculative_load_pclist[count] = __ pc();
  switch (type) {
    case T_FLOAT:  __ ldf (FloatRegisterImpl::S, O5, O4, F0); break;
    case T_DOUBLE: __ ldf (FloatRegisterImpl::D, O5, O4, F0); break;
    default:       ShouldNotReachHere();
  }

  __ ld (ca, O5);
  __ cmp (O5, G4);
  __ br (Assembler::notEqual, false, Assembler::pn, label2);
  __ delayed()->mov (O7, G1);

  __ retl ();
  __ delayed()-> nop ();

  slowcase_entry_pclist[count++] = __ pc();
  __ bind (label1);
  __ mov (O7, G1);

  address slow_case_addr;
  switch (type) {
    case T_FLOAT:  slow_case_addr = jni_GetFloatField_addr();  break;
    case T_DOUBLE: slow_case_addr = jni_GetDoubleField_addr(); break;
    default:       ShouldNotReachHere();
  }
  __ bind (label2);
  __ call (slow_case_addr, relocInfo::none);
  __ delayed()->mov (G1, O7);

  __ flush ();

  return fast_entry;
}

address JNI_FastGetField::generate_fast_get_float_field() {
  return generate_fast_get_float_field0(T_FLOAT);
}

address JNI_FastGetField::generate_fast_get_double_field() {
  return generate_fast_get_float_field0(T_DOUBLE);
}

