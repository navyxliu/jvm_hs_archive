@echo off
REM
REM Copyright (c) 2003, 2004, Oracle and/or its affiliates. All rights reserved.
REM ORACLE PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
REM
REM
REM
REM
REM
REM
REM
REM
REM
REM
REM
REM
REM
REM
REM
REM
REM
REM
REM  
REM

call saenv64.bat

%SA_JAVA_CMD% sun.jvm.hotspot.tools.PermStat %1 %2
