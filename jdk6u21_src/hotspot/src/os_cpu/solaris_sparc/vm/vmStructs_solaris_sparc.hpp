/*
 * Copyright (c) 2000, 2007, Oracle and/or its affiliates. All rights reserved.
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

// These are the OS and CPU-specific fields, types and integer
// constants required by the Serviceability Agent. This file is
// referenced by vmStructs.cpp.

#define VM_STRUCTS_OS_CPU(nonstatic_field, static_field, unchecked_nonstatic_field, volatile_nonstatic_field, nonproduct_nonstatic_field, c2_nonstatic_field, unchecked_c1_static_field, unchecked_c2_static_field, last_entry) \
                                                                                                                                     \
  /******************************/                                                                                                   \
  /* Threads (NOTE: incomplete) */                                                                                                   \
  /******************************/                                                                                                   \
                                                                                                                                     \
  nonstatic_field(JavaThread,                  _base_of_stack_pointer,                        intptr_t*)                             \
  nonstatic_field(OSThread,                    _thread_id,                                    thread_t)                              \
  /* This must be the last entry, and must be present */                                                                             \
  last_entry()


#define VM_TYPES_OS_CPU(declare_type, declare_toplevel_type, declare_oop_type, declare_integer_type, declare_unsigned_integer_type, declare_c1_toplevel_type, declare_c2_type, declare_c2_toplevel_type, last_entry) \
                                                                          \
  /**********************/                                                \
  /* Solaris Thread IDs */                                                \
  /**********************/                                                \
                                                                          \
  declare_unsigned_integer_type(thread_t)                                 \
                                                                          \
  /* This must be the last entry, and must be present */                  \
  last_entry()


#define VM_INT_CONSTANTS_OS_CPU(declare_constant, declare_preprocessor_constant, declare_c1_constant, declare_c2_constant, declare_c2_preprocessor_constant, last_entry) \
                                                                        \
  /************************/                                            \
  /* JavaThread constants */                                            \
  /************************/                                            \
                                                                        \
  declare_constant(JavaFrameAnchor::flushed)                            \
                                                                        \
  /* This must be the last entry, and must be present */                \
  last_entry()

#define VM_LONG_CONSTANTS_OS_CPU(declare_constant, declare_preprocessor_constant, declare_c1_constant, declare_c2_constant, declare_c2_preprocessor_constant, last_entry) \
                                                                        \
  /* This must be the last entry, and must be present */                \
  last_entry()
