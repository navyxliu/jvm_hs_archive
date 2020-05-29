#ifdef USE_PRAGMA_IDENT_SRC
#pragma ident "@(#)filemap.hpp	1.5 04/04/20 13:33:51 JVM"
#endif
/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL.  Use is subject to license terms.
 */

// Layout of the file:
//  header: dump of archive instance plus versioning info, datestamp, etc.
//   [magic # = 0xF00BABA2]
//  ... padding to align on page-boundary
//  read-write space from CompactingPermGenGen
//  read-only space from CompactingPermGenGen
//  misc data (block offset table, string table, symbols, dictionary, etc.)
//  tag(666)

static const int JVM_SHARED_JARS_MAX = 8;
static const int JVM_SPACENAME_MAX = 128;
static const int JVM_IDENT_MAX = 256;
static const int JVM_ARCH_MAX = 12;



class FileMapInfo : public CHeapObj {
private:
  enum {
    _invalid_version = -1,
    _current_version = 1
  };

  bool  _file_open;
  int   _fd;
  long  _file_offset;

  // FileMapHeader describes the shared space data in the file to be
  // mapped.  This structure gets written to a file.  It is not a class, so
  // that the compilers don't add any compiler-private data to it.

  struct FileMapHeader {
    int   _magic;                    // identify file type.
    int   _version;                  // (from enum, above.)

    struct space_info {
      int    _file_offset;   // sizeof(this) rounded to vm page size
      char*  _base;          // copy-on-write base address
      size_t _capacity;      // for validity checking
      size_t _used;          // for setting space top on read
      bool   _read_only;     // read only space?
      bool   _allow_exec;    // executable code in space?
    } _space[CompactingPermGenGen::n_regions];

    // The following fields are all sanity checks for whether this archive
    // will function correctly with this JVM and the bootclasspath it's
    // invoked with.
    char  _arch[JVM_ARCH_MAX];            // architecture
    char  _jvm_ident[JVM_IDENT_MAX];      // identifier for jvm
    int   _num_jars;              // Number of jars in bootclasspath

    // Per jar file data:  timestamp, size.

    struct {
      time_t _timestamp;          // jar timestamp.
      long   _filesize;           // jar file size.
    } _jar[JVM_SHARED_JARS_MAX];
  } _header;
  const char* _full_path;

  static FileMapInfo* _current_info;

  bool  init_from_file(int fd);
  void  align_file_position();

public:
  FileMapInfo() {
    _file_offset = 0;
    _file_open = false;
    _header._version = _invalid_version;
  }

  static int current_version()        { return _current_version; }
  void  populate_header();
  bool  validate();
  void  invalidate();
  int   version()                     { return _header._version; }
  size_t space_capacity(int i)         { return _header._space[i]._capacity; }
  char* region_base(int i)            { return _header._space[i]._base; }
  struct FileMapHeader* header()      { return &_header; }

  static void set_current_info(FileMapInfo* info)  { _current_info = info; }
  static FileMapInfo* current_info()  { return _current_info; }
  static void assert_mark(bool check);

  // File manipulation.
  bool  initialize();
  void  open_for_write();
  void  write_header();
  void  write_space(int i, CompactibleSpace* space, bool read_only);
  void  write_region(int region, char* base, size_t size,
                     size_t capacity, bool read_only, bool allow_exec);
  void  write_bytes(const void* buffer, int count);
  void  write_bytes_aligned(const void* buffer, int count);
  bool  map_space(int i, ReservedSpace rs, ContiguousSpace *space);
  char* map_region(int i, ReservedSpace rs);
  char* map_region(int i, bool address_must_match);
  void  unmap_region(int i);
  void  close();
  bool  is_open() { return _file_open; }

  // Errors.
  static void fail_stop(const char *msg, ...);
  void fail_continue(const char *msg, ...);
};
