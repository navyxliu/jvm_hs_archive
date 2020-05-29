/*
 * Copyright (c) 2005, Oracle and/or its affiliates. All rights reserved.
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

package sun.jvm.hotspot.tools;

import sun.jvm.hotspot.utilities.HeapHprofBinWriter;
import java.io.IOException;

/*
 * This tool is used by the JDK jmap utility to dump the heap of the target
 * process/core as a HPROF binary file. It can also be used as a standalone
 * tool if required.
 */
public class HeapDumper extends Tool {

    private static String DEFAULT_DUMP_FILE = "heap.bin";

    private String dumpFile;

    public HeapDumper(String dumpFile) {
        this.dumpFile = dumpFile;
    }

    protected void printFlagsUsage() {
        System.out.println("    <no option>\tto dump heap to " +
            DEFAULT_DUMP_FILE);
        System.out.println("    -f <file>\tto dump heap to <file>");
        super.printFlagsUsage();
    }

    // use HeapHprofBinWriter to write the heap dump
    public void run() {
        System.out.println("Dumping heap to " + dumpFile + " ...");
        try {
            new HeapHprofBinWriter().write(dumpFile);
            System.out.println("Heap dump file created");
        } catch (IOException ioe) {
            System.err.println(ioe.getMessage());
        }
    }

    // JDK jmap utility will always invoke this tool as:
    //   HeapDumper -f <file> <args...>
    public static void main(String args[]) {
        String file = DEFAULT_DUMP_FILE;
        if (args.length > 2) {
            if (args[0].equals("-f")) {
                file = args[1];
                String[] newargs = new String[args.length-2];
                System.arraycopy(args, 2, newargs, 0, args.length-2);
                args = newargs;
            }
        }

        HeapDumper dumper = new HeapDumper(file);
        dumper.start(args);
        dumper.stop();
    }

}
