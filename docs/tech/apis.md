---
layout: page
title: File APIs
nav_order: 3
permalink: /tech/apis
parent: Technical Guide
---

# File APIs

Mac OS filesystem APIs evolved as the underlying filesystem semantics changed.

## Pascal Strings

Prior to Mac OS X, Macintosh APIs used _Pascal strings_ to represent strings. Pascal strings are passed by pointer. The first byte pointed to stores the string length, and the string contents follows. Pascal strings are not null-terminated and not compatible with C-style strings. Note that since the string length is stored in a single byte, these strings cannot be longer than 255 bytes.

Compilers for Mac OS support Pascal strings by putting the `\p` sequence at the beginning of a string. For example,

```c
const unsigned char kFilename[] = "\pMy File";
```

This is equivalent to:

```c
const unsigned char kFilename[8] = {
  7, 'M', 'y', ' ', 'F', 'i', 'l', 'e'
};
```

Pascal strings are encoded using one of the old Macintosh character encodings, such as Mac OS Roman. In some cases, the encoding is assumed to be the system’s encoding, whatever that is. In other cases, the encoding is explicitly specified using a `ScriptCode` (although this value is somewhat ambiguous). In other cases still, the actual encoding is ignored and the string is treated as if it were encoded using the Mac OS Roman encoding.

## Early Mac OS

The filesystem on the very first Macintosh, later called the Mac 128K, did not support folders or directories. Each file was identified by volume ID and name. For example, `OpenDF` opens the data fork of a file (presumably, `DF` stands for “data fork”—there is a corresponding `OpenRF` for the resource fork), and `Create` creates a new file.

```c
OSErr OpenDF(
  const unsigned char *  fileName,
  short                  vRefNum,
  short *                refNum);

OSErr Create(
  const unsigned char *  fileName,
  short                  vRefNum,
  OSType                 creator,
  OSType                 fileType);
```

Filenames have a maximum length of 63 characters and are case insensitive. Systems from this era did not support multiple character encodings, so the encoding did not need to be specified. Note that the 63-character limit is an API limit, and common filesystems have a lower, 31-character limit.

You should not be using this API unless you are targeting _extremely_ old Macintosh systems, like the Mac 128K. This API became obsolete with the introduction of 128K ROMs with the Mac Plus in 1986.

This API is not part of Carbon and cannot be used on Mac OS X.

### Swapping Floppy Disks

During this era, it was common to swap floppy disks while a program was running. You can run a program from one disk and save files on another disk. When a program tries to access a file that’s on a different disk, the operating system ejects the current disk and prompts the user to insert the correct disk. This happens automatically; programs do not need to include any code to make this possible.

### Compatibility with HFS

Old applications written to use this API continue to work after files after the introduction of the hierarchical filesystem. The way this happens is through something called _working directories_.

A working directory is the combination of a volume ID and a directory ID, and it can be used in place of a volume ID in the filesystem API. The intent is that old code which only uses volume IDs can be used to save files in different locations on the filesystem. For example, if an old application creates a “save file” dialog box, instead of a volume ID, the dialog box returns a working directory pointing to the directory where the user chose to save the file.

## Hierarchical API

Alongside Apple’s first hard disk for the Macintosh, the operating system introduced a new filesystem (HFS) which supported directories, and this required a new API. Instead of `OpenDF`, programs now call `HOpenDF` to the data fork of a file, and call `HCreate` instead of `Create`. Presumably, “H” stands for “hierarchical”.

```c
OSErr HOpenDF(
  short                  vRefNum,
  long                   dirID,
  const unsigned char *  fileName,
  SInt8                  permission,
  short *                refNum);

OSErr HCreate(
  short                  vRefNum,
  long                   dirID,
  const unsigned char *  fileName,
  OSType                 creator,
  OSType                 fileType);
```

In this API, files are identified by volume ID, the directory ID within that volume, and the filename within that directory. The encoding is not specified, and presumably, files will be created using the system’s default character encoding.

## FSSpec API

Mac System 7 introduces the FSSPec API. It does not change semantics, but provides a simple data structure which is used to store the volume ID, directory ID, and filename. This structure is called `FSSpec`.

```c
struct FSSpec {
  short          vRefNum;
  long           parID;
  unsigned char  name[64];
};
```

Functions which use an `FSSpec` are named with the `FSp` prefix. These functions are preferred for over the previous versions for their simplicity. For example, `FSpOpenDF`, which opens a file’s data fork, and `FSpCreate`, which creates a new file:

```c
OSErr FSpOpenDF(
  const FSSpec *  spec,
  SInt8           permission,
  short *         refNum);

OSErr FSpCreate(
  const FSSpec *  spec,
  OSType          creator,
  OSType          fileType,
  ScriptCode      scriptTag);
```

Note that the character encoding is specified when creating a file, using the `scriptTag` parameter (although technically, this does not completely specify an encoding). The character encoding is not specified when opening a file, instead, the encoding is ignored and treated as if it were the Mac OS Roman encoding.

This API _preserves_ the encoding used for filenames, but you only need the correct bytestring to refer to existing files.

Starting in Mac OS X 10.4, this API and other APIs before it are marked as deprecated.

## FSRef API

Mac OS 9 introduces a new opaque alternative to `FSSpec` called `FSRef`. You can use an `FSRef` to refer to an existing file, but there is no way to get any information from an `FSRef` without invoking the filesystem API. The `FSRef` is not a drop-in replacement for `FSSpec`, beceause a `FSRef` must refer to an existing file, and therefore, cannot be used to create a new file.

```c
struct FSRef {
  UInt8  hidden[80];
};
```

The `FSRef` structure is private to an application and cannot be assumed to be valid if the file is moved/renamed, if the volume is unmounted and remounted, or if the structure is passed to another process. We can speculate that the `FSRef` structure contains information about a file like its filesystem inode, which can be used to look up the file name.

You can convert an `FSSpec` to an `FSRef`, although this will fail if the file does not exist:

```c
OSErr FSpMakeFSRef(
  const FSSpec *  source,
  FSRef *         newRef);
```

The `FSRef` API has some other differences. For example, when you open a file, you specify the name of the fork you want to open. Previous APIs used different functions for opening the data fork and the resource fork. File names are now specified as Unicode strings encoded with UTF-16.

```c
OSErr FSOpenFork(
  const FSRef *    ref,
  UniCharCount     forkNameLength,
  const UniChar *  forkName,
  SInt8            permissions,
  SInt16 *         forkRefNum);

OSErr FSCreateFileUnicode(
  const FSRef *          parentRef,
  UniCharCount           nameLength,
  const UniChar *        name,
  FSCatalogInfoBitmap    whichInfo,
  const FSCatalogInfo *  catalogInfo,
  FSRef *                newRef,
  FSSpec *               newSpec);
```

This API is a part of Carbon. Carbon was never ported to 64-bit architectures, and was deprecated in Mac OS X 10.8. The last version that supports this API is macOS 10.14, which is the last version of macOS that supports 32-bit programs.

## Unix API

Mac OS X brought us the Unix APIs:

```c
int open(
  const char *  pathname,
  int           flags,
  ...);

int openat(
  int           fd,
  const char *  pathname,
  int           flags,
  ...);
```

The `openat` call is available from Mac OS 10.10 onwards.

Strings are now null-terminated C strings, which are interpreted as UTF-8. Mac OS filesystems do not support arbitrary bytestrings as filenames. If you try to create a file with `open` with a filename that is not supported by the filesystem, the system call will fail and set `errno` to `EILSEQ` (92). This is not documented in the man page for `open`.

### Forks on Unix

The resource fork can be accessed as if it were a separate file. To access the resource fork, append `/rsrc` or `/..namedfork/rsrc` to the file path. This allows you to view resource fork data through ordinary Unix APIs or with Unix command-line utilities like `hexdump` and `ls`.

Linux also allows you to access the resource fork by appending `/rsrc` to the file path, for filesystems that support resource forks.

### Extended Attributes

Starting in version 10.4, Mac OS X provides an interface for accessing extended attributes on a file.

```c
ssize_t getxattr(
  const char *  path,
  const char *  name,
  void *        value,
  size_t        size,
  u_int32_t     position,
  int           options);
```

The resource fork is presented as an extended attribute with the name `com.apple.ResourceFork`. Since resource forks can be as much as 16 MiB in size, the `getxattr` function provides a way to read portions of the resource fork without having to read the entire fork.

Finder info is contained in an attribute named `com.apple.FinderInfo`.

## Aliases and Bookmarks

Mac OS also provides facilities to store a reference to a file. These references are designed to be durable, and work even if the file is moved and renamed. These references work by containing various pieces of metadata about the file. If the file is moved, the metadata can be used to find it again.

On older Mac OS systems, these records are called _aliases_ and you can create them using the alias manager APIs, which are available starting in System 7.

In Cocoa, these records are called _bookmarks_.

The main use of aliases and bookmarks is to store references to files in the “recently used files” menu option in applications.
