/*
 * Copyright (c) 2002, 2007, Oracle and/or its affiliates. All rights reserved.
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

// Forward declarations of classes defined here.
class GCTaskThread;
class GCTaskTimeStamp;

// Declarations of classes referenced in this file via pointer.
class GCTaskManager;

class GCTaskThread : public WorkerThread {
private:
  // Instance state.
  GCTaskManager* _manager;              // Manager for worker.
  const uint     _processor_id;         // Which processor the worker is on.

  GCTaskTimeStamp* _time_stamps;
  uint _time_stamp_index;

  GCTaskTimeStamp* time_stamp_at(uint index);

 public:
  // Factory create and destroy methods.
  static GCTaskThread* create(GCTaskManager* manager,
                              uint           which,
                              uint           processor_id) {
    return new GCTaskThread(manager, which, processor_id);
  }
  static void destroy(GCTaskThread* manager) {
    if (manager != NULL) {
      delete manager;
    }
  }
  // Methods from Thread.
  bool is_GC_task_thread() const {
    return true;
  }
  virtual void run();
  // Methods.
  void start();

  void print_task_time_stamps();
  void print_on(outputStream* st) const;
  void print() const                                { print_on(tty); }

protected:
  // Constructor.  Clients use factory, but there could be subclasses.
  GCTaskThread(GCTaskManager* manager, uint which, uint processor_id);
  // Destructor: virtual destructor because of virtual methods.
  virtual ~GCTaskThread();
  // Accessors.
  GCTaskManager* manager() const {
    return _manager;
  }
  uint which() const {
    return id();
  }
  uint processor_id() const {
    return _processor_id;
  }
};

class GCTaskTimeStamp : public CHeapObj
{
 private:
  jlong  _entry_time;
  jlong  _exit_time;
  char*  _name;

 public:
  jlong entry_time()              { return _entry_time; }
  jlong exit_time()               { return _exit_time; }
  const char* name() const        { return (const char*)_name; }

  void set_entry_time(jlong time) { _entry_time = time; }
  void set_exit_time(jlong time)  { _exit_time = time; }
  void set_name(char* name)       { _name = name; }
};
