/*
 * Copyright (c) 2003, 2006, Oracle and/or its affiliates. All rights reserved.
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

package sun.jvm.hotspot.runtime.amd64;

import java.util.*;
import sun.jvm.hotspot.code.*;
import sun.jvm.hotspot.compiler.*;
import sun.jvm.hotspot.debugger.*;
import sun.jvm.hotspot.oops.*;
import sun.jvm.hotspot.runtime.*;
import sun.jvm.hotspot.types.*;
import sun.jvm.hotspot.utilities.*;

/** Specialization of and implementation of abstract methods of the
    Frame class for the amd64 CPU. */

public class AMD64Frame extends Frame {
  private static final boolean DEBUG;
  static {
    DEBUG = System.getProperty("sun.jvm.hotspot.runtime.amd64.AMD64Frame.DEBUG") != null;
  }

  // refer to frame_amd64.hpp
  private static final int PC_RETURN_OFFSET           =  0;
  // All frames
  private static final int LINK_OFFSET                =  0;
  private static final int RETURN_ADDR_OFFSET         =  1;
  private static final int SENDER_SP_OFFSET           =  2;

  // Interpreter frames
  private static final int INTERPRETER_FRAME_MIRROR_OFFSET    =  2; // for native calls only
  private static final int INTERPRETER_FRAME_SENDER_SP_OFFSET = -1;
  private static final int INTERPRETER_FRAME_LAST_SP_OFFSET   = INTERPRETER_FRAME_SENDER_SP_OFFSET - 1;
  private static final int INTERPRETER_FRAME_METHOD_OFFSET    = INTERPRETER_FRAME_LAST_SP_OFFSET - 1;
  private static       int INTERPRETER_FRAME_MDX_OFFSET;         // Non-core builds only
  private static       int INTERPRETER_FRAME_CACHE_OFFSET;
  private static       int INTERPRETER_FRAME_LOCALS_OFFSET;
  private static       int INTERPRETER_FRAME_BCX_OFFSET;
  private static       int INTERPRETER_FRAME_INITIAL_SP_OFFSET;
  private static       int INTERPRETER_FRAME_MONITOR_BLOCK_TOP_OFFSET;
  private static       int INTERPRETER_FRAME_MONITOR_BLOCK_BOTTOM_OFFSET;

  // Entry frames
  private static final int ENTRY_FRAME_CALL_WRAPPER_OFFSET   =  -6;

  // Native frames
  private static final int NATIVE_FRAME_INITIAL_PARAM_OFFSET =  2;

  static {
    VM.registerVMInitializedObserver(new Observer() {
        public void update(Observable o, Object data) {
          initialize(VM.getVM().getTypeDataBase());
        }
      });
  }

  private static synchronized void initialize(TypeDataBase db) {
    if (VM.getVM().isCore()) {
      INTERPRETER_FRAME_CACHE_OFFSET = INTERPRETER_FRAME_METHOD_OFFSET - 1;
    } else {
      INTERPRETER_FRAME_MDX_OFFSET   = INTERPRETER_FRAME_METHOD_OFFSET - 1;
      INTERPRETER_FRAME_CACHE_OFFSET = INTERPRETER_FRAME_MDX_OFFSET - 1;
    }
    INTERPRETER_FRAME_LOCALS_OFFSET               = INTERPRETER_FRAME_CACHE_OFFSET - 1;
    INTERPRETER_FRAME_BCX_OFFSET                  = INTERPRETER_FRAME_LOCALS_OFFSET - 1;
    INTERPRETER_FRAME_INITIAL_SP_OFFSET           = INTERPRETER_FRAME_BCX_OFFSET - 1;
    INTERPRETER_FRAME_MONITOR_BLOCK_TOP_OFFSET    = INTERPRETER_FRAME_INITIAL_SP_OFFSET;
    INTERPRETER_FRAME_MONITOR_BLOCK_BOTTOM_OFFSET = INTERPRETER_FRAME_INITIAL_SP_OFFSET;
  }

  // an additional field beyond sp and pc:
  Address raw_fp; // frame pointer
  private Address raw_unextendedSP;

  private AMD64Frame() {
  }

  private void adjustForDeopt() {
    if ( pc != null) {
      // Look for a deopt pc and if it is deopted convert to original pc
      CodeBlob cb = VM.getVM().getCodeCache().findBlob(pc);
      if (cb != null && cb.isJavaMethod()) {
        NMethod nm = (NMethod) cb;
        if (pc.equals(nm.deoptBegin())) {
          // adjust pc if frame is deoptimized.
          if (Assert.ASSERTS_ENABLED) {
            Assert.that(this.getUnextendedSP() != null, "null SP in Java frame");
          }
          pc = this.getUnextendedSP().getAddressAt(nm.origPCOffset());
          deoptimized = true;
        }
      }
    }
  }

  public AMD64Frame(Address raw_sp, Address raw_fp, Address pc) {
    this.raw_sp = raw_sp;
    this.raw_unextendedSP = raw_sp;
    this.raw_fp = raw_fp;
    this.pc = pc;

    // Frame must be fully constructed before this call
    adjustForDeopt();

    if (DEBUG) {
      System.out.println("AMD64Frame(sp, fp, pc): " + this);
      dumpStack();
    }
  }

  public AMD64Frame(Address raw_sp, Address raw_fp) {
    this.raw_sp = raw_sp;
    this.raw_unextendedSP = raw_sp;
    this.raw_fp = raw_fp;
    this.pc = raw_sp.getAddressAt(-1 * VM.getVM().getAddressSize());

    // Frame must be fully constructed before this call
    adjustForDeopt();

    if (DEBUG) {
      System.out.println("AMD64Frame(sp, fp): " + this);
      dumpStack();
    }
  }

  // This constructor should really take the unextended SP as an arg
  // but then the constructor is ambiguous with constructor that takes
  // a PC so take an int and convert it.
  public AMD64Frame(Address raw_sp, Address raw_fp, long extension) {
    this.raw_sp = raw_sp;
    if ( raw_sp == null) {
      this.raw_unextendedSP = null;
    } else {
      this.raw_unextendedSP = raw_sp.addOffsetTo(extension);
    }
    this.raw_fp = raw_fp;
    this.pc = raw_sp.getAddressAt(-1 * VM.getVM().getAddressSize());

    // Frame must be fully constructed before this call
    adjustForDeopt();

    if (DEBUG) {
      System.out.println("AMD64Frame(sp, fp, extension): " + this);
      dumpStack();
    }

  }

  public Object clone() {
    AMD64Frame frame = new AMD64Frame();
    frame.raw_sp = raw_sp;
    frame.raw_unextendedSP = raw_unextendedSP;
    frame.raw_fp = raw_fp;
    frame.pc = pc;
    frame.deoptimized = deoptimized;
    return frame;
  }

  public boolean equals(Object arg) {
    if (arg == null) {
      return false;
    }

    if (!(arg instanceof AMD64Frame)) {
      return false;
    }

    AMD64Frame other = (AMD64Frame) arg;

    return (AddressOps.equal(getSP(), other.getSP()) &&
            AddressOps.equal(getFP(), other.getFP()) &&
            AddressOps.equal(getUnextendedSP(), other.getUnextendedSP()) &&
            AddressOps.equal(getPC(), other.getPC()));
  }

  public int hashCode() {
    if (raw_sp == null) {
      return 0;
    }

    return raw_sp.hashCode();
  }

  public String toString() {
    return "sp: " + (getSP() == null? "null" : getSP().toString()) +
         ", unextendedSP: " + (getUnextendedSP() == null? "null" : getUnextendedSP().toString()) +
         ", fp: " + (getFP() == null? "null" : getFP().toString()) +
         ", pc: " + (pc == null? "null" : pc.toString());
  }

  // accessors for the instance variables
  public Address getFP() { return raw_fp; }
  public Address getSP() { return raw_sp; }
  public Address getID() { return raw_sp; }

  // FIXME: not implemented yet (should be done for Solaris/AMD64)
  public boolean isSignalHandlerFrameDbg() { return false; }
  public int     getSignalNumberDbg()      { return 0;     }
  public String  getSignalNameDbg()        { return null;  }

  public boolean isInterpretedFrameValid() {
    if (Assert.ASSERTS_ENABLED) {
      Assert.that(isInterpretedFrame(), "Not an interpreted frame");
    }

    // These are reasonable sanity checks
    if (getFP() == null || getFP().andWithMask(0x3) != null) {
      return false;
    }

    if (getSP() == null || getSP().andWithMask(0x3) != null) {
      return false;
    }

    if (getFP().addOffsetTo(INTERPRETER_FRAME_INITIAL_SP_OFFSET * VM.getVM().getAddressSize()).lessThan(getSP())) {
      return false;
    }

    // These are hacks to keep us out of trouble.
    // The problem with these is that they mask other problems
    if (getFP().lessThanOrEqual(getSP())) {
      // this attempts to deal with unsigned comparison above
      return false;
    }

    if (getFP().minus(getSP()) > 4096 * VM.getVM().getAddressSize()) {
      // stack frames shouldn't be large.
      return false;
    }

    return true;
  }

  // FIXME: not applicable in current system
  //  void    patch_pc(Thread* thread, address pc);

  public Frame sender(RegisterMap regMap, CodeBlob cb) {
    AMD64RegisterMap map = (AMD64RegisterMap) regMap;

    if (Assert.ASSERTS_ENABLED) {
      Assert.that(map != null, "map must be set");
    }

    // Default is we done have to follow them. The sender_for_xxx will
    // update it accordingly
    map.setIncludeArgumentOops(false);

    if (isEntryFrame())       return senderForEntryFrame(map);
    if (isInterpretedFrame()) return senderForInterpreterFrame(map);


    if (!VM.getVM().isCore()) {
      if(cb == null) {
        cb = VM.getVM().getCodeCache().findBlob(getPC());
      } else {
        if (Assert.ASSERTS_ENABLED) {
          Assert.that(cb.equals(VM.getVM().getCodeCache().findBlob(getPC())), "Must be the same");
        }
      }

      if (cb != null) {
         return senderForCompiledFrame(map, cb);
      }
    }

    // Must be native-compiled frame, i.e. the marshaling code for native
    // methods that exists in the core system.
    return new AMD64Frame(getSenderSP(), getLink(), getSenderPC());
  }

  private Frame senderForEntryFrame(AMD64RegisterMap map) {
    if (Assert.ASSERTS_ENABLED) {
      Assert.that(map != null, "map must be set");
    }
    // Java frame called from C; skip all C frames and return top C
    // frame of that chunk as the sender
    AMD64JavaCallWrapper jcw = (AMD64JavaCallWrapper) getEntryFrameCallWrapper();
    if (Assert.ASSERTS_ENABLED) {
      Assert.that(!entryFrameIsFirst(), "next Java fp must be non zero");
      Assert.that(jcw.getLastJavaSP().greaterThan(getSP()), "must be above this frame on stack");
    }
    AMD64Frame fr;
    if (jcw.getLastJavaPC() != null) {
      fr = new AMD64Frame(jcw.getLastJavaSP(), jcw.getLastJavaFP(), jcw.getLastJavaPC());
    } else {
      fr = new AMD64Frame(jcw.getLastJavaSP(), jcw.getLastJavaFP());
    }
    map.clear();
    if (Assert.ASSERTS_ENABLED) {
      Assert.that(map.getIncludeArgumentOops(), "should be set by clear");
    }
    return fr;
  }

  private Frame senderForInterpreterFrame(AMD64RegisterMap map) {
    Address unextendedSP = addressOfStackSlot(INTERPRETER_FRAME_SENDER_SP_OFFSET).getAddressAt(0);
    Address sp = addressOfStackSlot(SENDER_SP_OFFSET);
    // We do not need to update the callee-save register mapping because above
    // us is either another interpreter frame or a converter-frame, but never
    // directly a compiled frame.
    // 11/24/04 SFG. This is no longer true after adapter were removed. However at the moment
    // C2 no longer uses callee save register for java calls so there are no callee register
    // to find.
    return new AMD64Frame(sp, getLink(), unextendedSP.minus(sp));
  }

  private Frame senderForCompiledFrame(AMD64RegisterMap map, CodeBlob cb) {
    //
    // NOTE: some of this code is (unfortunately) duplicated in AMD64CurrentFrameGuess
    //

    if (Assert.ASSERTS_ENABLED) {
      Assert.that(map != null, "map must be set");
    }

    // frame owned by optimizing compiler
    Address        sender_sp = null;


    if (VM.getVM().isClientCompiler()) {
      sender_sp        = addressOfStackSlot(SENDER_SP_OFFSET);
    } else {
      if (Assert.ASSERTS_ENABLED) {
        Assert.that(cb.getFrameSize() >= 0, "Compiled by Compiler1: do not use");
      }
      sender_sp = getUnextendedSP().addOffsetTo(cb.getFrameSize());
    }

    // On Intel the return_address is always the word on the stack
    Address sender_pc = sender_sp.getAddressAt(-1 * VM.getVM().getAddressSize());

    if (map.getUpdateMap() && cb.getOopMaps() != null) {
      OopMapSet.updateRegisterMap(this, cb, map, true);
    }

    if (VM.getVM().isClientCompiler()) {
      // Move this here for C1 and collecting oops in arguments (According to Rene)
      map.setIncludeArgumentOops(cb.callerMustGCArguments(map.getThread()));
    }

    Address saved_fp = null;
    if (VM.getVM().isClientCompiler()) {
      saved_fp = getFP().getAddressAt(0);
    } else if (VM.getVM().isServerCompiler() &&
               (VM.getVM().getInterpreter().contains(sender_pc) ||
               VM.getVM().getStubRoutines().returnsToCallStub(sender_pc))) {
      // C2 prologue saves EBP in the usual place.
      // however only use it if the sender had link infomration in it.
      saved_fp = sender_sp.getAddressAt(-2 * VM.getVM().getAddressSize());
    }

    return new AMD64Frame(sender_sp, saved_fp, sender_pc);
  }

  protected boolean hasSenderPD() {
    // FIXME
    // Check for null ebp? Need to do some tests.
    return true;
  }

  public long frameSize() {
    return (getSenderSP().minus(getSP()) / VM.getVM().getAddressSize());
  }

  public Address getLink() {
    return addressOfStackSlot(LINK_OFFSET).getAddressAt(0);
  }

  // FIXME: not implementable yet
  //inline void      frame::set_link(intptr_t* addr)  { *(intptr_t **)addr_at(link_offset) = addr; }

  public Address getUnextendedSP() { return raw_unextendedSP; }

  // Return address:
  public Address getSenderPCAddr() { return addressOfStackSlot(RETURN_ADDR_OFFSET); }
  public Address getSenderPC()     { return getSenderPCAddr().getAddressAt(0);      }

  // return address of param, zero origin index.
  public Address getNativeParamAddr(int idx) {
    return addressOfStackSlot(NATIVE_FRAME_INITIAL_PARAM_OFFSET + idx);
  }

  public Address getSenderSP()     { return addressOfStackSlot(SENDER_SP_OFFSET); }

  public Address compiledArgumentToLocationPD(VMReg reg, RegisterMap regMap, int argSize) {
    if (VM.getVM().isCore() || VM.getVM().isClientCompiler()) {
      throw new RuntimeException("Should not reach here");
    }

    return oopMapRegToLocation(reg, regMap);
  }

  public Address addressOfInterpreterFrameLocals() {
    return addressOfStackSlot(INTERPRETER_FRAME_LOCALS_OFFSET);
  }

  private Address addressOfInterpreterFrameBCX() {
    return addressOfStackSlot(INTERPRETER_FRAME_BCX_OFFSET);
  }

  public int getInterpreterFrameBCI() {
    // FIXME: this is not atomic with respect to GC and is unsuitable
    // for use in a non-debugging, or reflective, system. Need to
    // figure out how to express this.
    Address bcp = addressOfInterpreterFrameBCX().getAddressAt(0);
    OopHandle methodHandle = addressOfInterpreterFrameMethod().getOopHandleAt(0);
    Method method = (Method) VM.getVM().getObjectHeap().newOop(methodHandle);
    return (int) bcpToBci(bcp, method);
  }

  public Address addressOfInterpreterFrameMDX() {
    return addressOfStackSlot(INTERPRETER_FRAME_MDX_OFFSET);
  }

  // FIXME
  //inline int frame::interpreter_frame_monitor_size() {
  //  return BasicObjectLock::size();
  //}

  // expression stack
  // (the max_stack arguments are used by the GC; see class FrameClosure)

  public Address addressOfInterpreterFrameExpressionStack() {
    Address monitorEnd = interpreterFrameMonitorEnd().address();
    return monitorEnd.addOffsetTo(-1 * VM.getVM().getAddressSize());
  }

  public int getInterpreterFrameExpressionStackDirection() { return -1; }

  // top of expression stack
  public Address addressOfInterpreterFrameTOS() {
    return getSP();
  }

  /** Expression stack from top down */
  public Address addressOfInterpreterFrameTOSAt(int slot) {
    return addressOfInterpreterFrameTOS().addOffsetTo(slot * VM.getVM().getAddressSize());
  }

  public Address getInterpreterFrameSenderSP() {
    if (Assert.ASSERTS_ENABLED) {
      Assert.that(isInterpretedFrame(), "interpreted frame expected");
    }
    return addressOfStackSlot(INTERPRETER_FRAME_SENDER_SP_OFFSET).getAddressAt(0);
  }

  // Monitors
  public BasicObjectLock interpreterFrameMonitorBegin() {
    return new BasicObjectLock(addressOfStackSlot(INTERPRETER_FRAME_MONITOR_BLOCK_BOTTOM_OFFSET));
  }

  public BasicObjectLock interpreterFrameMonitorEnd() {
    Address result = addressOfStackSlot(INTERPRETER_FRAME_MONITOR_BLOCK_TOP_OFFSET).getAddressAt(0);
    if (Assert.ASSERTS_ENABLED) {
      // make sure the pointer points inside the frame
      Assert.that(AddressOps.gt(getFP(), result), "result must <  than frame pointer");
      Assert.that(AddressOps.lte(getSP(), result), "result must >= than stack pointer");
    }
    return new BasicObjectLock(result);
  }

  public int interpreterFrameMonitorSize() {
    return BasicObjectLock.size();
  }

  // Method
  public Address addressOfInterpreterFrameMethod() {
    return addressOfStackSlot(INTERPRETER_FRAME_METHOD_OFFSET);
  }

  // Constant pool cache
  public Address addressOfInterpreterFrameCPCache() {
    return addressOfStackSlot(INTERPRETER_FRAME_CACHE_OFFSET);
  }

  // Entry frames
  public JavaCallWrapper getEntryFrameCallWrapper() {
    return new AMD64JavaCallWrapper(addressOfStackSlot(ENTRY_FRAME_CALL_WRAPPER_OFFSET).getAddressAt(0));
  }

  protected Address addressOfSavedOopResult() {
    // offset is 2 for compiler2 and 3 for compiler1
    return getSP().addOffsetTo((VM.getVM().isClientCompiler() ? 2 : 3) *
                               VM.getVM().getAddressSize());
  }

  protected Address addressOfSavedReceiver() {
    return getSP().addOffsetTo(-4 * VM.getVM().getAddressSize());
  }

  private void dumpStack() {
    if (getFP() != null) {
      for (Address addr = getSP().addOffsetTo(-5 * VM.getVM().getAddressSize());
           AddressOps.lte(addr, getFP().addOffsetTo(5 * VM.getVM().getAddressSize()));
           addr = addr.addOffsetTo(VM.getVM().getAddressSize())) {
        System.out.println(addr + ": " + addr.getAddressAt(0));
      }
    } else {
      for (Address addr = getSP().addOffsetTo(-5 * VM.getVM().getAddressSize());
           AddressOps.lte(addr, getSP().addOffsetTo(20 * VM.getVM().getAddressSize()));
           addr = addr.addOffsetTo(VM.getVM().getAddressSize())) {
        System.out.println(addr + ": " + addr.getAddressAt(0));
      }
    }
  }
}
