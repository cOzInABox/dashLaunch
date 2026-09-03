disk swapper v1.3
=================
Multidisk game switcher

this plugin is not hard coded to require any specific version of
    dash (fixed in this release) or for any specific game.

!!! DO NOT use this plugin if you have autoswap enabled in Dash Launch 3.02
    or higher or if you use freestyle's plugin !!!


What it does:
=============
If you have games with more than one disk installed on your machine,
this plugin takes care of switching from one disk to the other ingame
(once you set up the multi.ini file if needed, the plugin, and dash
launch of course).
    

install
=======
-add to dash launch plugins
    ie:
    [Plugins]
    plugin1 = Usb:\swap.xex

-place swap.xex on the path you specified above
-edit and place multi.ini on the root of a usb/hdd/memory unit
    content partition (just like launch.ini) if needed
-in some cases it seems the plugins were only enabled after running
    the dash launch installer again, you may want to give this a try

Auto switching:
===============
	GOD ie1:
		disk1 = Hdd:\Content\0000000000000000\01234567\00004000\01234567890123456789; // will have 01234567890123456789.data folder beside it
		disk2 = Hdd:\Content\0000000000000000\01234567\00004000\98765432109876543210; // will have 98765432109876543210.data folder beside it
	GOD ie2:
		disk1 = Hdd:\Games\SomeGame\yourselfnameddisk1; // will have yourselfnameddisk1.data folder beside it
		disk2 = Hdd:\Games\SomeGame\yourselfnameddisk2; // will have yourselfnameddisk2.data folder beside it
	EXTRACTED ie:
		disk1 = Hdd:\games\somegame\disk1\default.xex;
		disk2 = Hdd:\games\somegame\disk2\default.xex;
- GOD/NXE disk rips on the same disk drive in the same folder will automatically
    be found without an ini file and no special naming convention
- EXTRACTED games with the naming above for each disk with the disk# folders all in
    the same folder on the same drive will be found without an ini file
- ini file takes precedence and will override these autos, for operations like
    GOD to extracted or disk to GOD switching (and similar)


thanks everyone in FSD, including/especially the FSD testers!
thanks krk for iso2god
thanks xedev for nxe2god

v1.3
- update drive list to include trinity mu for multi.ini
- allow GOD/NXE/EXTRACTED disk rips for auto disk switching
     *note: the auto functionality here is replicated in dash launch, without the ability to use ini files*
v1.2
- cleanup GOD devices from remount on game exit
v1.1
- fixes export table lookup to be versionless
v1.0
- first release

cOz


