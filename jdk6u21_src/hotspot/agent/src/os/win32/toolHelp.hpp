/*
 * Copyright (c) 2001, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _TOOLHELP_H_
#define _TOOLHELP_H_

#include <windows.h>
#include <tlhelp32.h>

namespace ToolHelp {
extern "C" {

  ///////////////
  // Snapshots //
  ///////////////
  typedef HANDLE WINAPI
  CreateToolhelp32SnapshotFunc(DWORD dwFlags, DWORD th32ProcessID);

  //////////////////
  // Process List //
  //////////////////
  typedef BOOL WINAPI Process32FirstFunc(HANDLE hSnapshot,
                                         LPPROCESSENTRY32 lppe);

  typedef BOOL WINAPI Process32NextFunc(HANDLE hSnapshot,
                                        LPPROCESSENTRY32 lppe);

  // NOTE: although these routines are defined in TLHELP32.H, they
  // seem to always return false (maybe only under US locales)
  typedef BOOL WINAPI Process32FirstWFunc(HANDLE hSnapshot,
                                          LPPROCESSENTRY32W lppe);

  typedef BOOL WINAPI Process32NextWFunc(HANDLE hSnapshot,
                                         LPPROCESSENTRY32W lppe);

  /////////////////
  // Module List //
  /////////////////
  typedef BOOL WINAPI
  Module32FirstFunc(HANDLE hSnapshot, LPMODULEENTRY32 lpme);

  typedef BOOL WINAPI
  Module32NextFunc (HANDLE hSnapshot, LPMODULEENTRY32 lpme);


  // Routines to load and unload KERNEL32.DLL.
  HMODULE loadDLL();
  // Safe to call even if has not been loaded
  void    unloadDLL();

} // extern "C"
} // namespace "ToolHelp"

#endif // #defined _TOOLHELP_H_
