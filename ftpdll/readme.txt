add as a plugin to dash launch
connect to your normal xbox IP on port 7564
it check for device change events every 1s or so, removed devices will still appear in the ftp list, new devices will be added.

if it doesn't seem like a good idea, don't do it (like write to flash)


it's not the greatest ftp, you don't get great speed uploading to the xbox but you can do it (slowly) while you play a game.
based on the xexmenu ftp (slimftpd) with some bugfixes and extra devices added.

cOz

v4
- add corona 4G paths
- add SHDN command to ftp command list, shuts down xbox
- added UTF8 to FEAT, some clients like filezilla should be able to show proper names now where glyphs are envolved

v3
- change load address, no longer overlaps anything else
- changed mount names slightly, easier to tell when using FTP or FTPdll
- fixed EXEC command (xex only)
