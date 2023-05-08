# SyncFiles

SyncFiles is being written, it is currently unusable.

SyncFiles is a tool which synchronizes files between classic Mac OS systems and modern systems. You can use it to copy files between a classic Macintosh development environment (e.g. MPW or CodeWarrior running on Mac System 7) and a modern Unix environment (e.g. a Basilisk II host system or AppleShare volume). SyncFiles will convert line endings, convert character encodings, preserve resource forks, and set file type and creator codes.

## Sharing Files

You still have to figure out how to transfer files.

- Networked Macs running Mac OS 9 or earlier: Use [Netatalk][netatalk] to run a AFP file server on a different computer on the network. Connect to the file server using the Chooser. Use SyncFiles to transfer files between the local hard drive and the file server. Note that Netatalk may require special configuration in order to work with old Macs.

- Networked Macs running Mac OS X: In addition to using AFP, you can also use a Samba file server. This will be easier to set up than Netatalk. Use SyncFiles to transfer files between the local hard drive and the file server. There is a command-line version of SyncFiles for Mac OS X.

- [Basilisk II][basiliskii] or [SheepShaver][sheepshaver] virtual machines: Use SyncFiles to transfer files between the VM and the host using the “host directory tree”. This requires either running Mac OS 7.6 or an older version of Mac OS with the “File System Manager 1.2” extension.

- Other virtual machines: [Mini vMac][minivmac], [QEMU][qemu]: Unknown.

[netatalk]: https://netatalk.sourceforge.io/
[basiliskii]: https://basilisk.cebix.net/
[sheepshaver]: https://sheepshaver.cebix.net/
[minivmac]: https://www.gryphel.com/c/minivmac/
[qemu]: https://www.qemu.org/

## License

SyncFiles is distributed under the terms of the Mozilla Public License, version 2.0. See LICENSE.txt for details.
