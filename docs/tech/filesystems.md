---
layout: page
title: Filesystems
nav_order: 5
permalink: /tech/filesystems
parent: Technical Guide
---

# Filesystems

This document focuses on the different filesystems used by Mac OS over the years and how filenames work on these filesystems.

## MFS

Macintosh file system (MFS) is Apple’s filesystem for the first Macintosh. MFS does not support directories and has a maximum filename length of 63 characters. According to Wikipedia, the final OS versions that supported MFS were 7.6 for read-write access and 8.0 for read-only access.

It is unusual to see this filesystem in practice, since it was replaced by HFS shortly after it appeared.

## HFS

Hierarchical file system (HFS) was introduced shortly after MFS and replaced MFS. It first appeared alongside Apple’s first Macintosh hard disk, the “Hard Disk 20” in 1985, and afterwards appeared in the 128K ROM on the Mac Plus. HFS introduces directories and a new set of file APIs.

The final versions of Mac OS which support HFS are Mac OS X 10.5 Leopard for read-write access and macOS 10.14 Mojave for read-only access.

HFS has a maximum filename length to 31 characters. The script used for the filename is recorded, but filenames are compared as if they are were encoded using the Macintosh Roman encoding. Filenames are case insensitive, and the sort order is described on page A-20 of _Inside Macintosh: Text_ (1993).

## HFS Plus

HFS Plus was introduced with Mac OS 8.1 and added support for filesystem journaling, Unicode filenames, up to 255 characters per filename, and case-sensitive filenames if enabled. Mac OS 8.1 was released in January 1998.

To provide backwards compatibility with older APIs, HFS Plus records the encoding that filenames should be encoded with when listing files in the older APIs. The volume header tracks a list of all encodings used for all filenames on the volume, so the appropriate conversion tables can be loaded when the volume is mounted. When an application using an older API lists the files in a directory, it will see backwards-compatible filenames substituted for filenames that use unsupported characters or filenames that are too long.

Filenames are stored in UTF-16, decomposed using the rules from Unicode 2.1 (up to Mac OS X 10.2) or Unicode 3.2 (for Mac OS X 10.3 and later).

See [Apple Technical Note TN1150: HFS Plus Volume Format][tn1150].

[tn1150]: https://developer.apple.com/library/archive/technotes/tn/tn1150.html

There is a variant of HFS Plus called HFSX. The major difference between normal HFS Plus and HFSX is that HFSX does not carry an HFS wrapper for backwards compatibility with systems that do not support HFS Plus.

## APFS

APFS is introduced in macOS 10.12.4. Filenames are encoded using UTF-8. Only code points assigned in Unicode 9.0 are permitted in filenames. APFS does not normalize filenames, but does store files by using the hash of the normalized version of the filename.

## UFS

UFS stands for _Unix file system_. It is a case-sensitive filesystem which is only supported by Mac OS X versions 10.0 through 10.5.

UFS is not seen often.

## Disk Images

There are various formats for disk images: Disc Copy 4.2 images, NDIF images, and UDIF images.
