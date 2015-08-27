# NanoPi-fuser-win32

This tool is designed to write a disk image, bootloader, u-boot environment,
and/or kernel to a microSD card for booting the FriendlyARM NanoPi under
Windows.

Please see the [releases page](https://github.com/ARMWorks/NanoPi-fuser-win32/releases)
to download a precompiled version of this tool.  Windows XP or later is
required to run this tool.

Compiling
---------

The solution was made with Visual Studio 2013.  To reproduce a release build,
you will need [Visual Studio 2013](https://www.visualstudio.com/news/vs2013-community-vs) and
[WTL 9.0 build 4140 Final](http://sourceforge.net/projects/wtl/files/WTL%209.0/WTL%209.0.4140%20Final/).

Make sure that the WTL include directory is in your include path and build as
usual with Visual Studio.
