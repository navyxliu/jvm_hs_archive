/*
 * Copyright (c) 2000, 2006, Oracle and/or its affiliates. All rights reserved.
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

 private:

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //
  // Sparc load/store emission
  //
  // The sparc ld/st instructions cannot accomodate displacements > 13 bits long.
  // The following "pseudo" sparc instructions (load/store) make it easier to use the indexed addressing mode
  // by allowing 32 bit displacements:
  //
  //    When disp <= 13 bits long, a single load or store instruction is emitted with (disp + [d]).
  //    When disp >  13 bits long, code is emitted to set the displacement into the O7 register,
  //       and then a load or store is emitted with ([O7] + [d]).
  //

  // some load/store variants return the code_offset for proper positioning of debug info for null checks

  // load/store with 32 bit displacement
  int load(Register s, int disp, Register d, BasicType ld_type, CodeEmitInfo* info = NULL);
  void store(Register value, Register base, int offset, BasicType type, CodeEmitInfo *info = NULL);

  // loadf/storef with 32 bit displacement
  void load(Register s, int disp, FloatRegister d, BasicType ld_type, CodeEmitInfo* info = NULL);
  void store(FloatRegister d, Register s1, int disp, BasicType st_type, CodeEmitInfo* info = NULL);

  // convienence methods for calling load/store with an Address
  void load(const Address& a, Register d, BasicType ld_type, CodeEmitInfo* info = NULL, int offset = 0);
  void store(Register d, const Address& a, BasicType st_type, CodeEmitInfo* info = NULL, int offset = 0);
  void load(const Address& a, FloatRegister d, BasicType ld_type, CodeEmitInfo* info = NULL, int offset = 0);
  void store(FloatRegister d, const Address& a, BasicType st_type, CodeEmitInfo* info = NULL, int offset = 0);

  // convienence methods for calling load/store with an LIR_Address
  void load(LIR_Address* a, Register d, BasicType ld_type, CodeEmitInfo* info = NULL);
  void store(Register d, LIR_Address* a, BasicType st_type, CodeEmitInfo* info = NULL);
  void load(LIR_Address* a, FloatRegister d, BasicType ld_type, CodeEmitInfo* info = NULL);
  void store(FloatRegister d, LIR_Address* a, BasicType st_type, CodeEmitInfo* info = NULL);

  int store(LIR_Opr from_reg, Register base, int offset, BasicType type, bool unaligned = false);
  int store(LIR_Opr from_reg, Register base, Register disp, BasicType type);

  int load(Register base, int offset, LIR_Opr to_reg, BasicType type, bool unaligned = false);
  int load(Register base, Register disp, LIR_Opr to_reg, BasicType type);

  void monitorexit(LIR_Opr obj_opr, LIR_Opr lock_opr, Register hdr, int monitor_no);

  int shift_amount(BasicType t);

  static bool is_single_instruction(LIR_Op* op);

 public:
  void pack64( Register rs, Register rd );
  void unpack64( Register rd );

enum {
#ifdef _LP64
         call_stub_size = 68,
#else
         call_stub_size = 20,
#endif // _LP64
         exception_handler_size = DEBUG_ONLY(1*K) NOT_DEBUG(10*4),
         deopt_handler_size = DEBUG_ONLY(1*K) NOT_DEBUG(10*4) };
