/*
 * Copyright (c) 2006, 2008, Oracle and/or its affiliates. All rights reserved.
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

import java.util.*;

// Copyright 1996, Animorphic Systems
// gri 28 Aug 92 / 15 Jan 93 / 8 Dec 95

class Queens {

  static void try_i(boolean a[], boolean b[], boolean c[], int x[], int i) {
    int adj = 7;

    for (int j = 1; j <= 8; j++) {
      if (b[j] && a[i+j] && c[adj+i-j]) {
        x[i] = j;
        b[j] = false;
        a[i+j] = false;
        c[adj+i-j] = false;
        if (i < 8) try_i(a, b, c, x, i+1);
        else print(x);
        b[j] = true;
        a[i+j] = true;
        c[adj+i-j] = true;
      }
    }
  }

  public static void main(String s[]) {
    boolean a[] = new boolean[16+1];
    boolean b[] = new boolean[ 8+1];
    boolean c[] = new boolean[14+1];
    int     x[] = new int[8+1];
    int adj = 7;

    for (int i = -7; i <= 16; i++) {
      if (i >= 1 && i <= 8) b[i]     = true;
      if (i >= 2)           a[i]     = true;
      if (i <= 7)           c[adj+i] = true;
    }

    x[0] = 0; // solution counter

    try_i(a, b, c, x, 1);
  }

  static void print(int x[]) {
    // first correct solution: A1 B5 C8 D6 E3 F7 G2 H4

    char LF = (char)0xA;
    char CR = (char)0xD;

    x[0]++;
    if (x[0] < 10)
        System.out.print(" ");
    System.out.print(x[0] + ". ");
    for (int i = 1; i <= 8; i++) {
      char p = (char)('A' + i - 1);
      System.out.print(p);
      System.out.print (x[i] + " ");
    }
    System.out.println();
  }

};
