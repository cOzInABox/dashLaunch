Dash Launch 3.21
------------------

Known Issues:
----------------

- *** WARNING ***
    One of the testers observed a console reaching out to live despite liveblock only
    when fakelive or autofake was enabled.  If you intend a keyvault to not get banned,
    do not use it on a glitch/jtag machine!
- Autologin pop-up blob does not display properly... live with it.

Currently the project is missing the following supported translations:
Japanese, Korean, Chinese Simplified, Chinese traditional

Currently supplied translations:
English, French, Portuguese, Russian, Spanish, German, Italian, Polish

The skin pack includes the sources used to skin dash launch as well as the
string files if anyone wishes to create a translation to one of the above
languages (including English, as I know my explanations are not always easy to
understand.) Also included is a c# based editor for the string files to assist
in translation.

External fonts, background image and skins may be used by this,
simply place skin.xzp and/or font.ttf and/or background.png (1020x720) beside
default.xex. If neither location has a font file supplied the system font
on flash will be used.


================================================================================
    Overview - what it does
================================================================================
- It will launch a XeX or CON file from the path you specified in launch.ini
    as long as it's valid
- Depending on the button you hold when the xbox is trying to load the NXE, it
    will divert to the xex/con tied to that button or return to default as
    defined in launch.ini
- At boot time it is possible to subvert default item and/or NXE loading, but
    you must wait until the controller syncs to do so
- Depending which button is held when closing miniblade in NXE (use Y button to
    close, release then hold a QL button) it will quick launch a CON or XEX
    from your ini file
- allows one to patch kernel/xam at bootup with a freeBOOT patch style bin file
    from usb or hdd (in that order) must be in root of the device and be named
    "kxam.patch" and be no larger than 0x4000 bytes. Again, kxam.patch binary
    format is the same as a compiled freeboot patch bin, but uses real virtual
    addresses rather than offsets - as of 2.22 the first 32bit value must instead
    be the version of the kernel the patches apply to
- with the included patch set, launch.xex acts as a helper to detect when
    xbox1 emulator loads, allowing memory unlock patch and xbox1 emulator
    to function together

================================================================================
    Installation
================================================================================
- have the required hacked kernel version installed on the console

- get the installer to a place where you can run it, and do so. Follow onscreen
    instructions if any. The installer will prompt you if it needs to update the 
    kernel/hv patches and will give you an opportunity to configure stuff.

- edit the options, and dont forget to save them somewhere if you want them to
    be applied next boot. Don't forget to set configapp to the installer, so
    you can go to it any time via miniblades' system settings (hold RB to go to
    real system settings)

- the back button is your friend if you are wondering what button to push

================================================================================
    Updaters and Avatars
================================================================================
- this version of dash launch contains an update blocker that is enabled by
    default. There are two ways around this if you wish to install the bits and
    pieces used by the dash for kinect and avatars
    1 - place the updater that matches this version on removable media, and
        rename the folder from $SystemUpdate to $$ystemUpdate
        ----OR----
    2 - place an ini where dash launch can find it and set the noupdater option
        to false - noupdater = false
    Updates seem to work best if memory stick is inserted while in official dash
****
NOTE that some games WILL prompt you to update the console if avatars are
    not installed, this doesn't mean they have an update to actually put in, it
    just means it needs avatar/kinect bins/resources to run
****

================================================================================
    LIVEblocker
================================================================================
- if you are like me, and keep your consoles off the net then this option is
    for you. It's capable of blocking the resolution of the LIVE specific
    servers and does so by default, with an additional option in the ini file
    it will attempt to block access to all MS servers. The default option is
    set up to block only LIVE servers, which still allows programs like FSD to
    access covers and such. The blocks lists are:

    weak:
        ^xemacs.xboxlive.com$
        ^xeas.xboxlive.com$
        ^xetgs.xboxlive.com$
        ^xexds.xboxlive.com$
        ^piflc.xboxlive.com$
        ^siflc.xboxlive.com$
        ^msac.xboxlive.com$
        ^xlink.xboxlive.com$
        ^xuacs.xboxlive.com$
        ^sts.xboxlive.com$
        ^xam.xboxlive.com$
        ^notice.xbox.com$
        ^macs.xbox.com$
        ^rad.msn.com$
        passport.net$

    strong:
        xboxlive.com$
        xbox.com$
        nsatc.net$
        microsoft.com$
        passport.net$
        bing.net$
        msn.com$
    
    where:
        somedomain.com$ = ends with somedomain.com
        ^somesub.somedomain = starts with somesub.somedomain
        ^somesub.somedomain.com$ = is exactly somesub.somedomain.com

================================================================================
    Important - going to NXE
================================================================================
- if you need to go back to NXE and have default item set in ini, HOLD RB while
    exiting game via miniblade or exit using one of the miniblade options like 
    family settings

================================================================================
    INI notes
================================================================================
-it's possible to have multiple ini files, priority is as they appear in the list
    (** it is NOT recommended to launch USB con/xex from hdd ini **)
    the first one found on the devices in that order will be the one used at boot.
    -see http://code.jellycan.com/simpleini/ for more info on the ini parser

================================================================================
    autoswap option functionality
================================================================================
    GOD ie:
        disk1 = Hdd:\Content\0000000000000000\01234567\00004000\01234567890123456789; 
            will have 01234567890123456789.data folder beside it
        disk2 = Hdd:\Content\0000000000000000\01234567\00004000\98765432109876543210;
            will have 98765432109876543210.data folder beside it
    EXTRACTED ie:
        disk1 = Hdd:\games\somegame\disk1\default.xex;
        disk2 = Hdd:\games\somegame\disk2\default.xex;
- GOD/NXE disk rips on the same media in the same folder will automatically
    be found with no special naming convention
- EXTRACTED games with the naming above for each disk with the disk# folders all in
    the same folder on the same media will be found without an ini file
- swapping between disks contained on different media is not supported

================================================================================
    Caveats
================================================================================
The work herein is presented as-is, any risk is solely the end users
    responsibility. While all of us are sorry when unforeseen things happen, not
    every situation or mistake can be accounted for before they have been
    spotted. Please use responsibly.

================================================================================
    Support (report bugs/request features)
================================================================================
    english:        http://www.realmodscene.com/index.php?/forum/14-dashlaunch/
    french/english: http://homebrew-connection.org/forum/index.php?board=7.0

================================================================================
    Thanks
================================================================================
-Big thanks to those who opened the way and those who made it even more usable.
-Thanks to Tux, Arbiter, stk, the2000, Corrupted, tk_saturn and Toddler for all
    the bugs you caught trying to sneak by
-Thanks AmyGrrl for passing along the glitch and new ideas
-Thanks to Tux, Ironman, JPizzle and Dionis Fernandez for helping procure a
    Jasper big block console to extend testing and fix NAND MU corruption bug
    Dionis - you went above and beyond
-Thanks to vgcrepairs for providing the cygnos, dash launch likely wouldn't 
    exist without one
-Thanks to the FSD team, without your cheering this rewrite would have never been completed
-Thanks to Nate and Anthony for constantly reminding me that no, I'm not alone
-Thanks to FBDev and mojobojo for the data used for the patch options
-Thanks to sm32
-Thanks to unknown, you know why
-Extra Special thanks to SpkLeader, Boflc, and LordXBig
-Big thanks to Swizzy, the least bit for debugging readmes
-Thanks to XeBuild, keeping us on our toes and up to date
-Greetz to XeDev and RgLoader
-Thanks to Team Xecuter for thinking towards the future
-Thanks to vladstudio.com for "night launch"
-Thanks to Razkar for always spotting the hard to spot bugs
-Thanks to Danny Lane for doing a bunch of testing on Corona 16m
-Thanks Juvenal for being the best sarcastic a**hole there ever was
-shouts out to E Nellie and D33per, thanks to you this is still a sourceless release

~brought to you by cOz~
//2019


================================================================================
    To Do
================================================================================
- fix hud loading of nxe rips
- everything else
- spoof privilege for disk eject when game is not running from a disk (thanks lopertyur!)

================================================================================
    Known Bugs
================================================================================
- some well used NAND images with earlier versions of DL already installed seem
    incapable of being updated with larger sized files, it is recommended for
    the time being to make a clean NAND image with the most recent/up to date
    image builder if you run into this issue
- nxe disk rips when launched from 16197+ metro still work, if you get an err
    dismiss it and launch again (it's a resource busy issue in official dash)
- some situations are causing a black screen when starting installer, it somehow seems to
    be related to USB devices and/or signed in profiles. If you run into this issue, try the debug
    version of installer - it's slower because it's logging to disk but apparently works fine.
- I'm sure you'll find some bugs, please see the links earlier in this doc
    for a place to post them where they will be seen

================================================================================
    Supported Versions
================================================================================
    at time of this writing, this is ONLY compatible with RETAIL kernel versions:
	9199, 12611, 12625, 13146, 13599, 13604, 14699, 14717, 14719, 15574, 16197, 16202, 16203,
    16537, 16547, 16747, 16756, 16767, 17148, 17150, 17349, 17489, 17502, 17511, 17526, 17544, 17559
	13599 is the first glitch version supported (embedded patches)
	14717 is the first glitch2 version supported (embedded patches)

================================================================================
    ChangeLog
================================================================================
V3.21
- add 17559

V3.20
- add 17544

V3.19
- add 17526

V3.18.1
- critical bug fix

V3.18
- add 17511

V3.17
- add 17502
- minor bugfixes

V3.16
- updated spanish translation
- add 17489

V3.15
- update embedded patches 15574+ to allow dash tile to show data dvd game icon/name
- switch to Rtl kernel functions for *printf, safer for system threads
- add 17349

V3.14
- add transfer cable to list of possible devices as xfer:
- added back button logo to installer to illustrate how to show/hide help without using the readme
- add 17148, 17150
- update 16767+ with the new security sector skip patch for transfer cable

V3.13
- add 16756, 16767
- add UTF8 to the ftp FEAT list, improves non-ascii name support
- can now place up to 10 title IDs in ini for autofake to enable fakelive at title startup
- fix dvd game/video loading from official dash tile
- added autocont option (yeah it's not a network option really, but relies on autofake so its right near it)
- changed how contpatch works, added/separated into xblapatch and licpatch (the Lets Try Find The Problem Blindly edition)
- made number value entry in installer a little more consistent
- added a check to launch.xex for lhelper.xex in flash to prevent E71 error screen
- updated built in update server to V3

V3.12
- default behaviour of live block is now to use strong block rules (at least until ini is loaded)
- fix compatibility issues with dashes created before AP25 was deployed (spoof the AP functions on older versions)
- limit fakelive/autofake to 14717+ kernels
- add export so plugins can find out where they were loaded from during dllMain() (it's volatile, copy it in Main() if you need it!)
- update 16547 patches to delay network bringup in xam until launch.xex loads
- add trinity internal usb to hddkeepalive (for those that have a usb hdd hooked up there)
- add 16747

V3.11
- more thoroughly check display names in xcontent header if english name is not present (TODO: check if this applies on launch items too)
- yet another correction to the dev kernel checks (thanks tydye and XDK!)
- add 16547
- fix autoswap for going from disk 2 to 1 (DS3) (thanks c.... and Swizzy for the report!)
- made launcher mode useful if dl is not running

V3.10
- fix for 13599-14699, dash launch patch sets were missing trinity patches
- fix bug that was misidentifying trinity as a fat glitch1 when updating patches (sorry everyone!)
- prevent too frequent polling for network address (should fix black screen on some consoles when ethernet disconnect helped)

V3.09
- fix in update server for corona 16M consoles (thanks Danny Lane!)
- added exception logging to installer
- fix some minor bugs
- *known issue* some situations are causing a black screen when starting installer, it somehow seems to
    be related to USB devices and/or signed in profiles. If you run into this issue, try the debug
    version of installer - it's slower because it's logging to disk but apparently works fine. (thanks again Danny!)

V3.08
- tweak xelllaunch, see it's readme for how it's changed
- all patch sets updated to support xebuild update server full use
- added xebuild update server and related options
- fixed a bug with signnotice on 13604 (and probably older)
- changed farenheit to fahrenheit everywhere it wasn't before
- added 16537

V3.07
- added 16203
- hopefully all cpu/dvd keys will display fully in installer now
- fix description spelling error (F/C)

V3.06
- fix power/guide boot time paths when fakeanim is not used (thanks mass3n!)
- fix hddalive task being scheduled as a title task and not surviving title changes (thanks moulder!)

V3.05
- add 16202
- update spanish translation (thanks gromber!)
- fix remotenxe and windows button on remote not booting to dash (thanks spkleader!)

V3.04
- kinect health message block fixed for 16197
- updated polish translation (thanks Pelcu!)
- fix CIV hook issue, may break some titles that use CIV (a gamy Call of Decay: Body Odor 2 now works)
- lump updater version limit patch into noupdater option so it can be disabled
- improved installers ability to prevent install on unsupported kernels (including devkit and unsupported retail versions)
- installer will now only offer to update, if the embedded version is newer than currently running one

V3.03
- some commented code made contpatch non-functional on untouched demo containers
- added polish translation
- add nohealth option, disables kinect health pseudo video at game launch
- add autofake option, when enabled fakelive functionality is enabled during dash and indie games only (thanks BioHazard!)
- added some failsafe code to lhelper and launch to ensure auto profile signins occur
- moved boot time quick launch button check to lhelper, it now occurs at the point where bootanim freezes (approx)
- removed bootdelay option, it should no longer be required
- add corona 4G memory unit path
- add fakeanim path
- fix bugs related to Guide/Power paths
- add PIRS type content to installer launch item parsing
- add nooobe option, disables setup screens when settings already exist
- dash launch now patches xam to prevent flash updates from appearing when updaters newer than current are on devices
- wired controller poweron causes should now be recognized from all ports for Guide path
- added new option 'fahrenheit'
- add 16197
- removed button debouncing, A and Y are more useful exiting from miniblades but will be touchy on older dashes
- add corona bl detection to xelllaunch
- added a few more domains to liveblock

V3.02
- add italian translation - thanks Gnappo!
- correct mobo/edram order in shuttemps and installer
- update Spanish translation
- correct version number
- add RThumb and LThumb for paths (analog controller button when you press them down)
- add autoswap option for multidisk games (see notes above)
- remove beta tag
- add 15574
- correct bug in hv patches in 14717/14719
- add poweron reason to tembcast data, document struct sent in the supplied .py script
- fixed problem with loading on older (<13xxx) firmwares, thanks KneelB4ZD for the donor image!

V3.01
- add Russian translation
- add Spanish translation - thanks Gromber!
- add German translation - thanks Tuxuser!
- altered DNS blocker to fail dns requests on block instead of succeed to loopback address (speed improvement)
- fakelive now forces DNS blocker to be on (thanks uzi for the heads up! IG 4tw!)
- installer: launch button now can launch indie games, they MUST be in their proper content path to detect/work
    autosets fakelive (and dns blocker) on when launching indie games via installer
- going to system settings from installer now goes to official system settings (if nosysexit is not true)
- added new option "shuttemps" which displays temperature data on the shutdown scene (hold guide down)
    thanks to Dwack for the idea, sorry it took so long
- added basic ftp (based on ftpdll)
- reduced default bootdelay to 0x1E
- new option 'devprof' allows devkit profiles to work on retail firmware
    note any changes such as saving games or getting achievements will resign the profile with the current/retail keyvault
    this seems not to affect glitch/jtag dev crossflash, but could affect true devkits
- new option 'devlink' to allow system link with devkits, ping limit is still separate (thanks Anthony for devlink!)
- updated patches to remove CON sig checks, remove restrictions on xekeys (thanks Redline!) and add hvpeek api to keyed syscall
- add glitch2 to xelllaunch, force file sizes to be 4 byte aligned (thanks Juvenal!)
- nxe disk installs can now be started like GOD containers
- blacklist devkit firmware during installation checks
- changed dlaunchGetOptInfo to give a more useful category instead of the internal bitmask
- changed filters to be inline and use the new categories
- add external options to the ini file (ftpserv, ftpport)
- can launch elf via embedded xell stage1 (thanks libxenon devs!)
- added info button in misc page
    - show CPU key, DVD key, console ID, console serial number, MAC address and decrypted XVal (0 is no violations)
    - allows adjusting fan settings and smc_config target temps and optionally saves them to flash
- added external option calaunch for config app, so it will start in the launch option instead of normal options
- load external skin/background/font to memory so the files are no longer held open
- prevent dash launch from taking over signin, create profile (was waiting infintely) and skip in metro startup/login screen
- change trap hook method so nate's awesome xbdm does not break across load/unload of dash launch

v3.00
- as with V1->V2 this is a nearly complete restructure and rewrite, expect bugs
- rewrote all hooks and tasks to be unhookable/stopable
- installer can now unload any existing v3 xex and/or start dash launch without rebooting console
   - installing over v2 or installing patch updates still requires reboot
- setup exports for managing all options from external programs
- stop exception recovery from firing a new launch/bubble message more than once in a ~4s window
- add 'configapp' path, if it exists going to miniblades -> system settings will start this program
- rewrote installer a little to be marginally better
- ini category [quicklaunchbuttons] is now simply [paths]
- add 'nonetstore' option (hides network storage in disk dialogs)
- hook XexpVerifyXexHeaders and XexpLoadImage to detect retail encrypted xex with bad signature
    and fix the image key (thanks Anthony!)
- safereboot is no longer tied to fatalfreeze, reboot requests when this is set to false will be
    redirected to jtag friendly (but hard on hardware) methods
- added in glitch2 patches, restructured embedded patch sets to be a munged file instead of individual
- fixed fakelive to get past app gold check (still does not work, can't connect to server) and no longer
    try to reply to profile info requests with a hardcoded online xuid
- added french translation - thanks to Razkar!
- added portugese translation - thanks to SpkLeader!
- added translation c# GUI - thanks to Swizzy!

v2.32
- fixed glitch jasper big block patch installer
- reworked contpatch yet again, should perform equal to xm360 now (thanks node21!)
   - new patch only operates on containers of type 000D0000 (XBLA) and 00000002 (ADDONS) of LIVE or PIRS types

v2.31
- revert contpatch to older form
- signnotice now defaults to FALSE/disabled
- signnotice option should no longer wind up in network troubleshooter on 14717+
- add 14719

v2.30
- fixed uart debug output 0xD 0xA instead of 0xA 0xD
- STOP code 0x2B can now output stack info
- xex header revoke check (requires live to download revoke list) flag now ignored (hv and xam patch)
- add 14717

v2.29
- expanded temp broadcaster to include PE Name and path of current title
- added titleid and mediaid output to temp logger
- fixed a bug in unhandled-exception handler (could cause freeze/multiple consecutive exceptions)
- add title module PE name and path to exception log
- contpatch completely rewritten, now takes over checking license bits entirely for xam when enabled
    (may break... things, or allow some to work that shouldn't/crash)

v2.28
- added rad.msn.com to weak blocklist
- added *bing.net, *msn.com to strong blocklist
- added glitch machine detection for xellLaunch to launch on flash xell-1f
- added xhttp auth patch for 14699 (thanks Anthony!)
- added signin notice dismiss (optionally disabled, only affects 'ok' type dialogs)
- added intMu: to installer ini updater
- added autoselect shutdown and auto off option for the "hold guide to shutdown"
    NOTE: that both these options can affect other things that use this type of dialog!
- added optional temp broadcaster
- added quick python script to cap temp broadcasts to a .csv file

v2.27
- removed FCRT patch (was not compatible with 1175 drives)
- installer: revise patch checks to only check base patches
- installer: conform to xebuild's base+patch extension method, copies addon patches to
    base as needed
- add 14699

v2.26
- correct jtag/glitch wording in installer patch updater
- fix compare with glitch machine, now accurate when it checks patches for update
- add fcrt removal patch (and correction)
- rebuild to hopefully improve stability (less optimization)
- changed contpatch to only patch ID bytes to 0xFF
- fix sonic, more thoroughly hooked disk verification (thanks again Nate!)
- add current launch.xex version display to installer, add versioning
- add note about encryption on modified retail xex
- added '$' to permitted chars in launch.xex ini parser

v2.25
- added Trinity arcade memory unit to dash launch as IntMu:
- can update ggBuild type patch sets
- add 13604
- patches updated to remove E66 (dvd code exec) errors in kernel

v2.24
- hddalive was defaulted to true instead of false, fixed
- xell launch now shuts off usb device (fixes issues with xell reloaded)
- xell launch simplified to allow for variable sized xell bins
- added jtag debug support to patches and export var (thanks Nate!)
- reworked devkit signed xex loading. Faster, dll dev xex working again (thanks Anthony!)
- fix forza 4 (and maybe others - thanks Nate!)

v2.23
- added hddalive (2.23b fixes this)
- added 13599
- relocate external files so old files will no longer be used accidentally
    new path is \default.xex dir\VERSION\files
    ie: GAME:\13599\patches_jasper.bin

v2.22
- resolving 'localhost' when the router forwards it to the internet or there is
    no network at all... bad idea game devs... fixed (Yars', maybe others)
- button handler now more reliably removes Y and A mishandling when held on
    miniblade exits
- added new note to readme regarding update prompts and avatar data missing
- hopefully extracted new games are now working fine, instead of GOD only
- potential bug corrected in 12611/12625 patch sets
- add 13146 compatibility
- all patch sets updated to fully remove xex bound checks (ie: default.xex
    on root of USB causing E71)
- "remotenxe" option added to ini(thx adihash!)
- windows button on remote always boots to NXE/media center now(thx adihash!)
- added "guide" and "power" path options to set boot time default override
- changed kxam.patch, first .long must be the kernel version the patches match
- added a check to kxam.patch data to abort on invalid address
- added "nohud" option
- added installer check to verify at least the 1BL segment of patches before
    installing, will re-ask and warn of possible corruption/brick

v2.21
- fixed noupdater option, readme is accurate with regards to updates now (sorry)
- fixed dvdexitdash option, no longer conflicts with using miniblade to exit NXE
    should only affect DVD launched from NXE (note this affects DVD games too)
- fixed a glitch with unhandled exception logging when occurs in kernel
- revert to original fileExist() method

v2.20
- export option info along with the rest of dash launch info struct export
- add multi version compatibility to installer and plugin
- add 12625 patch set and offsets
- new LIVE content hook patching, does auto yaris swap as well as extracted XBLA 
    should work more consistently (hopefully)
- added unhandled exception handler, dumps except info to UART/file and exits
    to dash/default item when apps don't have their own exception handler
    (instead of crash), disable by setting 'exchandler = false' in ini file.
- added ini path setting (dumpfile) for capturing crash logs to a file,
    capture device must be connected at console boot time
- added 'safereboot' option for those who have JTAG that have applied
    blackaddr's smc reboot fix, instead of 'hard' reboot
- added option to enable debug strings to print to UART
- adjusted patches to remove default UART hooking (less chance of string
    collision/overlap using DbgPrint via debug out option)
- corrected a bug in the flasher ini update settings in regard to noupdater,
    it was setting nosysexit instead of noupdater value
- added live "blocker", reroutes requests to resolve DNS names to loopback
- added "livestrong" option to use an alternate list of DNS to block
- added ini option to set how long buttons are watched for at boot time
- embedding current versions external files into installer, no more messy
    directory; original paths still work and take priority over embedded files 

v2.11
- fixed xbox1 launches (thanks folks at x-s and fsd for reporting)

v2.10
- reworked hooking to be a bit more dynamic and simpler to update
- ini parser fixed, glitch when comment line last line with no blank line after
    (thanks Toddler!)
- disables updaters (DA2 and other disks; safety)
- minor tweaks to boot time delays, further improves on previous autologin issue

v2.09 (beta)
- moved strictly to C, much smaller DLL
- correct bug with busy CON/sometimes ignoring ini for boot time default item
- made boot time quick launch buttons more reliable
- added 1s delay to resolve autologin at boot issue and slower USB hdd issues
- patches updated to remove min version check (DA2)

v2.08
- correcting for a glitch where launchdata should be cleared between titles but
   isn't; fixes launching some games twice in a row (thanks stk and FSD!)

v2.07
- fixed media center extender (thanks jester and antman)
    ~hopefully this is the last whitelist option needed
- added option "nosysexit" (thanks rhai)

v2.06
- update to fbbuild 0.11 patches
- fixed bug with fatal freeze options
- changed installer to use zeropair CB version to determine patch set
- added new options dvdexitdash and xblaexitdash (thanks AmyGrrl)
- added regionspoof, dvdexitdash and xblaexitdash to ini updater
- added instructions to this readme regarding boot time buttons and diagnosing
    non-ASCII ini files

v2.05
- added AP25 xex priveledge filter (fix AC:B GOD/xex, maybe others)

v2.04
- fixed a glitch with launching kinect games when a default item is set

v2.03
- updated for 12611

v2.02
- added version info to data struct exported at ordinal 1
- made number of times button holds are scanned variable, longer window
    at boot time to sync controller and hold a button
- added region spoofing for XGetGameRegion
- DVD video play from NXE now plays DVD regardless of default setting
    (thanks krizalid!)

v2.01
- corrected flash mu mount point (thanks Antho02 at l-s)
- added kernel version check to installer as some xbr using folks seem incapable
    of reading the first line of this file

v2.0
- plugins now use logical paths just like quick launch buttons
- added common (9199) content and ping limit patches as options
- mostly runs in system threads, startup completes while bootanim runs
- completely subverted dash.xex, no more CD issues or NXE split seconds
- removed insistance on 0/1/2 paths and reliance on CaPs to detect
- return to NXE via miniblade for system settings and others works w/o using RB
- added big block NAND mu as possible device
- optionally subvert Y to exit miniblade while in NXE to load button/default
- added xell loader to patch set and included a xex to load xell
- removed reboot on fatal error from patches
- included reboot/shutdown on fatal error as settings
- installer onscreen output cleaned up, now shuts down console at end of install
- added ini file updater to installer
- added patch updater to installer

v1.0
- added other devices for launch targets
- added flash for location to load launch.ini
- added flash installer supports flashing launch.xex and launch.ini
- with help of freeboot patches, fixes issues with xbox1 emulator on memory
    unlocked patchset
- overhaul ini parser with simpleIni, support for most buttons and a default
- added dll/plugin loading support
- added hooking/return to launch app instead of NXE (hold RB to bypass)
- added kernel/xam boot time/one time patch engine
- added export to allow the loading of other system modules

v0.02
- added a small delay to allow XBR users to launch CON
- implemented a simple ini file parser and fileExist
- auto detects LIVE and XEX2 to use the appropriate launch method
- fails to dash reliably now

v0.01
-initial release
