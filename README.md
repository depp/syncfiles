# SyncFiles

SyncFiles is a tool for MPW (Macintosh Programmer’s Workshop) which synchronizes files between a Macintosh and Unix system. It is used to copy files between a classic Macintosh development environment (e.g. MPW running on Mac System 7) and a modern Unix environment (e.g. a Basilisk II host system or an AppleShare volume).

## What SyncFiles Does

- By default, only copies files which are _newer_ than the destination file (unless `-force` is specified). This means that your classic Macintosh’s system should have the clock set correctly!

- Sets the modification timestamp of the destination file to match the timestamp of the source file.

- Only synchronizes files which match hard-coded patterns.

- Converts text files to UTF-8 and LF line endings for Unix systems; converts to Mac OS Roman and CR line endings for Macintosh systems. XML files are not re-encoded.

- For resource files, converts by copying the Macintosh resource fork to the data fork.

- Sets the file type and creator code, creating MPW Shell text files and ResEdit resource files.

## File Patterns

Copies files named Makefile, and files with the following extensions:

- C: `.c` `.h`

- C++: `.cc` `.cp` `.cpp` `.cxx` `.hh` `.hpp` `.hxx`

- Plain text: `.txt`

- Resource: `.rsrc`

- XML: `.xml` (CR-LF conversion only, no encoding conversion)

## Usage

Operates in push or pull mode. The tool runs from inside the classic Macintosh environment, so the “push” mode copies from Macintosh to Unix, and the “pull” mode copies from Unix to Macintosh. It is assumed that the Macintosh directory is on a normal disk volume.

### Basic Usage

To push files from the current directory,

```
SyncFiles <DestPath> -push
```

To pull files from the current directory,

```
SyncFiles <DestPath> -pull
```

### Other Flags

- `-verbose`: Print lots of boring messages.

- `-quiet`: Print only errors and warnings.

- `-force`: Ignore timestamps, copy all source files to destination.

- `-dry-run`: Perform no actions, just print out what would be done.

- `-dir`: Specify an alternat Macintosh directory to push from or pull to. By default, pushes from and pulls to the current directory.

- `-delete`: Delete files in destination which are missing from source.

## Testing

Run `sh test.sh` to test the text conversion code.

## License

SyncFiles is distributed under the terms of the MIT license. See LICENSE.txt for details.
