#ifdef USE_PRAGMA_IDENT_SRC
#pragma ident "@(#)icache.cpp	1.15 04/03/18 12:13:23 JVM"
#endif
/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL.  Use is subject to license terms.
 */

#include "incls/_precompiled.incl"
#include "incls/_icache.cpp.incl"

// The flush stub function address
AbstractICache::flush_icache_stub_t AbstractICache::_flush_icache_stub = NULL;

void AbstractICache::initialize() {
  // Making this stub must be FIRST use of assembler 
  ResourceMark rm;

  BufferBlob* b = BufferBlob::create("flush_icache_stub", ICache::stub_size);
  CodeBuffer* c = new CodeBuffer(b->instructions_begin(), b->instructions_size());

  ICacheStubGenerator g(c);
  g.generate_icache_flush(&_flush_icache_stub);

  // The first use of flush_icache_stub must apply it to itself.
  // The StubCodeMark destructor in generate_icache_flush will
  // call Assembler::flush, which in turn will call invalidate_range,
  // which will in turn call the flush stub.  Thus we don't need an
  // explicit call to invalidate_range here.  This assumption is
  // checked in invalidate_range.
}

void AbstractICache::call_flush_stub(address start, int lines) {
  // The business with the magic number is just a little security.
  // We cannot call the flush stub when generating the flush stub
  // because it isn't there yet.  So, the stub also returns its third
  // parameter.  This is a cheap check that the stub was really executed. 
  static int magic = 0xbaadbabe;

  int auto_magic = magic; // Make a local copy to avoid race condition
  int r = (*_flush_icache_stub)(start, lines, auto_magic);
  guarantee(r == auto_magic, "flush stub routine did not execute");
  ++magic;
}

void AbstractICache::invalidate_word(address addr) {
  // Because this is called for instruction patching on the fly,
  // long after bootstrapping, we execute the stub directly.
  // Account for a 4-byte word spanning two cache lines.
  intptr_t start_line = ((intptr_t)addr + 0) & ~(ICache::line_size - 1);
  intptr_t end_line   = ((intptr_t)addr + 4) & ~(ICache::line_size - 1);
  (*_flush_icache_stub)((address)start_line, start_line == end_line ? 1 : 2, 0);
}

void AbstractICache::invalidate_range(address start, int nbytes) {
  static bool firstTime = true;
  if (firstTime) {
    guarantee(start == CAST_FROM_FN_PTR(address, _flush_icache_stub),
	      "first flush should be for flush stub");
    firstTime = false;
    return;
  }
  if (nbytes == 0) {
    return;
  }
  // Align start address to an icache line boundary and transform
  // nbytes to an icache line count.
  const uint line_offset = mask_address_bits(start, ICache::line_size-1);
  if (line_offset != 0) {
    start -= line_offset;
    nbytes += line_offset;
  }
  call_flush_stub(start, round_to(nbytes, ICache::line_size) >>
		         ICache::log2_line_size);
}

// For init.cpp
void icache_init() {
  ICache::initialize();
}
