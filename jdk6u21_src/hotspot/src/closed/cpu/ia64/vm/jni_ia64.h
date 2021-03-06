/*
 * Copyright (c) 2007, Oracle and/or its affiliates. All rights reserved.
 * ORACLE PROPRIETARY/CONFIDENTIAL.  Use is subject to license terms.
 */

#ifndef _JAVASOFT_JNI_MD_H_
#define _JAVASOFT_JNI_MD_H_

#if defined(SOLARIS) || defined(LINUX)
  #define JNIEXPORT
  #define JNIIMPORT
  #define JNICALL

  typedef int jint;
  typedef long long jlong;
#else
  #define JNIEXPORT __declspec(dllexport)
  #define JNIIMPORT __declspec(dllimport)
  #define JNICALL __stdcall

  typedef int jint;
  typedef __int64 jlong;
#endif

typedef signed char jbyte;

#endif /* !_JAVASOFT_JNI_MD_H_ */
