#ifdef USE_PRAGMA_IDENT_HDR
#pragma ident "@(#)genRemSet.hpp	1.16 04/01/27 14:34:51 JVM"
#endif
/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL.  Use is subject to license terms.
 */

// A GenRemSet provides ways of iterating over pointers accross generations.
// (This is especially useful for older-to-younger.)

class Generation;
class BarrierSet;
class OopsInGenClosure;
class CardTableRS;

class GenRemSet: public CHeapObj {
  friend class Generation;

  BarrierSet* _bs;
  
public:
  enum Name {
    CardTable,
    Other
  };

  GenRemSet(BarrierSet * bs) : _bs(bs) {}

  virtual Name rs_kind() = 0;

  // These are for dynamic downcasts.  Unfortunately that it names the
  // possible subtypes (but not that they are subtypes!)  Return NULL if
  // the cast is invalide.
  virtual CardTableRS* as_CardTableRS() { return NULL; }

  // Return the barrier set associated with "this."
  BarrierSet* bs() { return _bs; }

  // Do any (sequential) processing necessary to prepare for (possibly
  // "parallel", if that arg is true) calls to younger_refs_iterate.
  virtual void prepare_for_younger_refs_iterate(bool parallel) = 0;

  // Apply the "do_oop" method of "blk" to (exactly) all oop locations
  //  1) that are in objects allocated in "g" at the time of the last call
  //     to "save_Marks", and
  //  2) that point to objects in younger generations.
  virtual void younger_refs_iterate(Generation* g, OopsInGenClosure* blk) = 0;

  virtual void younger_refs_in_space_iterate(Space* sp,
					     OopsInGenClosure* cl) = 0;

  // This method is used to notify the remembered set that "new_val" has
  // been written into "field" by the garbage collector.
  void write_ref_field_gc(oop* field, oop new_val);
protected:
  virtual void write_ref_field_gc_work(oop* field, oop new_val) = 0;
public:

  // A version of the above suitable for use by parallel collectors.
  virtual void write_ref_field_gc_par(oop* field, oop new_val) = 0;

  // Resize one of the regions covered by the remembered set.
  virtual void resize_covered_region(MemRegion new_region) = 0;

  // If the rem set imposes any alignment restrictions on boundaries
  // within the heap, this function tells whether they are met.
  virtual bool is_aligned(HeapWord* addr) = 0;

  // If the RS (or BS) imposes an aligment constraint on maximum heap size.
  // (This must be static, and dispatch on "nm", because it is called
  // before an RS is created.)
  static uintx max_alignment_constraint(Name nm);

#ifndef PRODUCT
  virtual void verify() = 0;

  // Verify that the remembered set has no entries for
  // the heap interval denoted by mr.
  virtual void verify_empty(MemRegion mr) = 0;
#endif
  
  // If appropriate, print some information about the remset on "tty".
  virtual void print() {}

  // Informs the RS that the given memregion contains no references to
  // younger generations.
  virtual void clear(MemRegion mr) = 0;

  // Informs the RS that there are no references to generations
  // younger than gen from generations gen and older.
  // The parameter clear_perm indicates if the perm_gen's
  // remembered set should also be processed/cleared.
  virtual void clear_into_younger(Generation* gen, bool clear_perm) = 0;

  // Informs the RS that refs in the given "mr" may have changed
  // arbitrarily, and therefore may contain old-to-young pointers.
  virtual void invalidate(MemRegion mr) = 0;
  
  // Informs the RS that refs in this generation
  // may have changed arbitrarily, and therefore may contain
  // old-to-young pointers in arbitrary locations. The parameter
  // younger indicates if the same should be done for younger generations
  // as well. The parameter perm indicates if the same should be done for
  // perm gen as well.
  virtual void invalidate_or_clear(Generation* gen, bool younger, bool perm) = 0;
};
