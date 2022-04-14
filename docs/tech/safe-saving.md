---
layout: page
title: Safe Saving
nav_order: 6
permalink: /tech/safe-saving
parent: Technical Guide
---

# Safe Saving

There are a number of different goals for when your program saves a file:

- I/O errors should be reported. If the data does not make it to disk, then tell the user that the operation failed.

- Saves are atomic. After saving, you either get the old version of the file or the complete new version of the file. If your program crashes, it’s okay if the old file is untouched, but it’s not okay if it’s been partially overwritten.

- Saves preserve file references. Any references to a document (aliases or bookmarks) remain valid after modifying the document.

- Saves do not change the creation date, or other metadata associated with the file.

If your first thought is, “that sounds like it could be complicated”, then you’re in good company. Theodore Ts’o wrote an article in 2009, [Don’t fear the fsync!][tso-fsync] which covers some of these issues on Linux in detail.

[tso-fsync]: https://thunk.org/tytso/blog/2009/03/15/dont-fear-the-fsync/

## Classic Mac OS

HFS and HFS+ support an operation which exchanges the contents of files. The high-level API call looks like this:

```c
OSErr FSpExchangeFiles(
  const FSSpec *  source,
  const FSSpec *  dest);
```

This function exchanges the _contents_ of the two files (both forks), and exchanges the modification dates, but leaves the other metadata alone.

The recipe for safe saving on HFS volumes is:

1. Save the document to a temporary file on the same volume.

1. Exchange the contents of the original file and the temporary file with `FSpExchangeFiles`.

You can test that `FSpExchangeFiles` is supported by a volume by getting the volume parameters. Not all filesystems support this operation.

## Mac OS X

Mac OS X provides a Unix system call that provides the same functionality as `FSpExchangeFiles`, but with a Unix API.

```c
int exchangedata(
  const char *  path1
  const char *  path2,
  unsigned int  options);
```

However, this function does not work on APFS.

## Mac OS X 10.6+

Starting on Mac OS X 10.6, the Foundation framework provides a method for safely replacing an item on the filesystem with a new item. This method is present on `NSFileManager`:

```objc
- (BOOL)replaceItemAtURL:(NSURL *)originalItemURL
           withItemAtURL:(NSURL *)newItemURL
          backupItemName:(NSString *)backupItemName
                 options:(NSFileManagerItemReplacementOptions)options
        resultingItemURL:(NSURL * _Nullable *)resultingURL
                   error:(NSError * _Nullable *)error;
```

This method should be preferred for Mac OS X 10.6 and newer. Unlike `exchangedata()`, this function works on APFS.
