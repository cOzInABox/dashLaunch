# this is a simple python script to capture the UDP packets
# that are broadcast by dash launch, display that info
# in human readable form and log the data to a csv file
# note, there is also another sample for using this data on a PC in launch_sysdll_exports.c

# thanks juvenal for the struct unpack :)
import socket, sys, time, struct

# this is the structure that is broadcast over the UDP socket from dash launch, last updated as of DL3.02
#	#pragma pack(push, 1)
#	typedef struct _TEMP_MESSAGE {
#		BYTE smcMsg[MESSAGE_TEMPDATA_SZ]; // 0x0 sz 0x10
#		char peName[MESSAGE_PE_MAX_SZ]; // 0x10 sz 0x50
#		char imagePath[MESSAGE_COMMAND_SZ]; // 0x60 sz 0x100
#		DWORD Tid; // 0x160 sz 0x4
#		DWORD Mid; // 0x164 sz 0x4
#		BYTE PwrRsn; // 0x168 sz 0x1
#		BYTE padding[0x97]; // pad to 0x200
#	}TEMP_MESSAGE, *PTEMP_MESSAGE;
#	C_ASSERT(sizeof(TEMP_MESSAGE) == 0x200);
#	#pragma pack(pop)

port = 7030

dgr = unichr(176)
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
s.bind(('', port))
divisor = float(256)
header = 'CPU,GPU,EDRAM,MOBO,UNK,Time,Date,PEName,Path,TID,MID,PWRsn\n'

PWRrsn = 	[[0x11,"Console Power Button pushed"],
			[ 0x12,"Eject Button pushed"],
			[ 0x15,"?? Wake alarm caused power on ??"],
			[ 0x20,"Power Button pressed on IR Remote"],
			[ 0x22,"Guide Button pressed on Media Remote"],
			[ 0x24,"Windows Button pressed on Media Remote"],
			[ 0x30,"SMC caused console reset"],
			[ 0x31,"?? Power button pushed from PNC ??"],
			[ 0x41,"Kiosk remote power on signal"],
			[ 0x55,"Wireless Controller Guide button pressed"],
			[ 0x56,"Guitar/Accessory turned console on"],
			[ 0x57,"BigButton controller Guide button pressed"],
			[ 0x5A,"Wired Controller Guide button pressed"]]

def getReason(pwrrsn):
	found = False
	for it in PWRrsn:
		if pwrrsn == it[0]:
			retval = "%s." % (it[1])
			found  = True

	if not found:
		retval = "Unknown poweron cause 0x%02x!" % (pwrrsn)
	return retval

def get_len(var, max):
	for i in range(max):
		if ord(var[i]) == 0x0:
			return (i)
	return 0

def tempPoll(lf):
	message = s.recv(0x200)
	# uncomment this bit if you want to view the raw data too
	# out_str = ""
	# for x in range(0x200):
		# out_str += "%02X " % (ord(message[x]))
	#print out_str
	(cmd, cpu, gpu, edr, mob, unk, pename, image, tid, mid, rsn) = struct.unpack("<B4HB6x80s256s2LB151x", message)
	if cmd != 0x7:
		print "ERRROR!!! 0x%x is not a temp reply!!!" % (cmd)
		return

	pename = pename.split("\x00")[0]
	image = image.split("\x00")[0]
	
	cpu = float(cpu/divisor)
	gpu = float(gpu/divisor)
	edr = float(edr/divisor)
	mob = float(mob/divisor)
	rsn = getReason(rsn)
	st = "%0.1f,%0.1f,%0.1f,%0.1f,%d,%s,%s,%s,%08x,%08x,%s\n" % (cpu, gpu, edr, mob, unk, time.strftime("%H:%M:%S,%m/%d/%Y"), pename, image, tid, mid, rsn)
	lf.write(st)
	print "CPU: %0.1f%sC GPU: %0.1f%sC EDRAM: %0.1f%sC MOBO: %0.1f%sC Unk: %d %s image: %s @ %s T: 0x%08x M: 0x%08x REAS: %s" % (cpu, dgr, gpu, dgr, edr, dgr, mob, dgr, unk, time.strftime("%I:%M:%S%p"), pename, image, tid, mid, rsn)
	
if __name__ == '__main__':
	logfile = 'temps.csv'
	if (len(sys.argv) == 1):
		logfile = "temps_%s.csv" % (time.strftime("%I.%M.%S%p_%m.%d.%Y"))
		lfile = open(logfile, 'w')
	else:
		logfile = sys.argv[0]
	print "Opening log: %s" % (logfile)
	lfile = open(logfile, 'w')
	lfile.write(header)

	while True:
		try:
			tempPoll(lfile)
		except KeyboardInterrupt:
			lfile.close()
			sys.exit()
			
