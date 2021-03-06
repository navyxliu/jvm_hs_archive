/*
 * Copyright (c) 2003, Oracle and/or its affiliates. All rights reserved.
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

package sun.jvm.hotspot.memory;

import java.util.*;
import sun.jvm.hotspot.debugger.*;
import sun.jvm.hotspot.oops.*;
import sun.jvm.hotspot.types.*;
import sun.jvm.hotspot.runtime.*;
import sun.jvm.hotspot.utilities.*;

public class PlaceholderEntry extends sun.jvm.hotspot.utilities.HashtableEntry {
  static {
    VM.registerVMInitializedObserver(new Observer() {
        public void update(Observable o, Object data) {
          initialize(VM.getVM().getTypeDataBase());
        }
      });
  }

  private static synchronized void initialize(TypeDataBase db) {
    Type type = db.lookupType("PlaceholderEntry");
    loaderField = type.getOopField("_loader");
  }

  // Field
  private static sun.jvm.hotspot.types.OopField loaderField;

  // Accessor
  public Oop loader() {
    return VM.getVM().getObjectHeap().newOop(loaderField.getValue(addr));
  }

  public PlaceholderEntry(Address addr) {
    super(addr);
  }

  public Symbol klass() {
    return (Symbol) literal();
  }

  /* covariant return type :-(
  public PlaceholderEntry next() {
    return (PlaceholderEntry) super.next();
  }
  For now, let the caller cast it ..
  */
}
