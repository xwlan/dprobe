dprobe
======

D Probe is a dynamic trace toolkit for user mode Windows application, support custom filters, dynamic loading/unloading
of tracing runtime, large volume trace records, conditional filtering in UI, low overhead even in high work load.

structure
=========

+ bin      Build target folder
+ btr      The core runtime injected into target process address space, support both x86/x64 builds
+ dprobe   UI console, record view, management, trace control, symbol parse etc
+ flt      Several common custom filters, include fs/mm/registry/ps/net/wininet/
+ inc      Common headers
+ lib      Common libs
+ kbtr     Empty kernel mode driver, not implement yet
+ sqlite   SQLITE as metadata for configuration purpose

build
=====

Build dprobe.sln in VS 2008+

contact
=======

lan.john at gmail dot com
