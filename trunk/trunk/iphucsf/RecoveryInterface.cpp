#include "RecoveryInterface.h"

static am_recovery_device *g_recoveryDevice;

void recovery_progress_callback(unsigned int progress_number, unsigned int opcode)
{
	fprintf(stderr, "Recovery progress callback...\n");

	if (opcode == 9) {
		//sendCommandToDevice(g_recoveryDevice, CFSTR("setenv boot-args rd=md0 -progress"));

		// alternative boot args for restore mode boot spew
		sendCommandToDevice(g_recoveryDevice, CFSTR("setenv boot-args rd=md0 -v -progress"));
	}

}

int recovery_restore(string *args, struct shell_state *sh)
{
	
	mach_error_t ret;
	
	if (args[1] == "")
	{
		ifNotQuiet cout << "args[1] must be RestoreBundlePath" << endl;
		return SHELL_CONTINUE;
	}
	
	D("Building CFMutableDictionary");
	CFMutableDictionaryRef opts;
	D("Getting AMRestoreCreateDefaultOptions");
	opts = AMRestoreCreateDefaultOptions(kCFAllocatorDefault);
	CFStringRef value = CFStringCreateWithCString(kCFAllocatorDefault, args[1].c_str(),
												  kCFStringEncodingMacRoman);
	CFDictionarySetValue(opts, CFSTR("RestoreBundlePath"), value );
	
	//    describe255(opts);

	g_recoveryDevice = sh->recovery_dev;
	ret = AMRestorePerformRecoveryModeRestore(	sh->recovery_dev, opts,
							(void *)recovery_progress_callback, NULL );

	CFRelease(value);
	ifVerbose cout 	<< "AMRestorePerformRecoveryModeRestore: " << ret << endl;
	
	return SHELL_CONTINUE;
}

int recovery_grestore(string *args, struct shell_state *sh)
{
	int ret;
	
	if (args[1] == "")
	{
		ifNotQuiet cout << "args[1] must be RestoreBundlePath" << endl;
		return SHELL_CONTINUE;
	}
	
	// check for trailing /
	if ( args[1].at(args[1].length() - 1) != '/' )
		args[1] = args[1] + '/';
	
	ifNotQuiet cout << "Entering G-Restore ... " << endl;

	ifVerbose cout << "setting auto-boot to false" << endl;
	ret = sendCommandToDevice(sh->recovery_dev, CFSTR("setenv auto-boot false"));
	ifVerbose cout << "sendCommandToDevice: " << ret << endl;	
	if (ret != 0) {
		ifNotQuiet cout << "failed (" << ret << ") Aborting." << endl;
		return SHELL_CONTINUE;
	}
	sleep(1);

	ifVerbose cout << "saving environment" << endl;
	ret = sendCommandToDevice(sh->recovery_dev, CFSTR("saveenv"));
	ifVerbose cout << "sendCommandToDevice: " << ret << endl;	
	if (ret != 0) {
		ifNotQuiet cout << "failed (" << ret << ") Aborting." << endl;
		return SHELL_CONTINUE;
	}
	sleep(1);


	string temp = args[1] + "Firmware/all_flash/all_flash.m68ap.production/applelogo.img2";
	
	ifVerbose cout << "Sending apple logo img2 '"<< temp << "'" << endl;
	CFStringRef img2 = CFStringCreateWithCString(kCFAllocatorDefault, temp.c_str(), kCFStringEncodingMacRoman);
	ret = sendFileToDevice(sh->recovery_dev, img2);
	CFRelease(img2);
	ifVerbose cout << "sendFileToDevice: " << ret << endl;
	if (ret != 0) {
		ifNotQuiet cout << "Send apple logo img failed.  Aborting." << endl;
		return SHELL_CONTINUE;
	}
	sleep(1);
	
	ifVerbose cout << "setting picture 0" << endl;
	ret = sendCommandToDevice(sh->recovery_dev, CFSTR("setpicture 0"));
	ifVerbose cout << "sendCommandToDevice: " << ret << endl;	
	if (ret != 0) {
		ifNotQuiet cout << "failed (" << ret << ") Aborting." << endl;
		return SHELL_CONTINUE;
	}
	sleep(1);

	ifVerbose cout << "setting bgcolor 0 0 0" << endl;
	ret = sendCommandToDevice(sh->recovery_dev, CFSTR("bgcolor 0 0 0"));
	ifVerbose cout << "sendCommandToDevice: " << ret << endl;	
	if (ret != 0) {
		ifNotQuiet cout << "failed (" << ret << ") Aborting." << endl;
		return SHELL_CONTINUE;
	}
	sleep(1);


	temp = args[1] + "694-5259-38.dmg";

	// Send ramdisk to the phone
	ifVerbose cout << "Sending ramdisk '"<< temp << "'" << endl;
	CFStringRef ramdisk = CFStringCreateWithCString(kCFAllocatorDefault, temp.c_str(), kCFStringEncodingMacRoman);
	ret = sendFileToDevice(sh->recovery_dev, ramdisk);
	CFRelease(ramdisk);
	ifVerbose cout << "sendFileToDevice: " << ret << endl;
	if (ret != 0) {
		ifNotQuiet cout << "Send ramdisk failed.  Aborting." << endl;
		return SHELL_CONTINUE;
	}
	sleep(1);
	
	// load ramdisk on the phone
	ifVerbose cout << "Loading ramdisk." << endl;
	ret = sendCommandToDevice(sh->recovery_dev, CFSTR("ramdisk"));
	ifVerbose cout << "sendCommandToDevice 'ramdisk': " << ret << endl;
	if (ret != 0) {
		ifNotQuiet cout << "Load ramdisk failed.  Aborting." << endl;
		return SHELL_CONTINUE;
	}
	sleep(1);
	

	temp = args[1] + "Firmware/all_flash/all_flash.m68ap.production/DeviceTree.m68ap.img2";

	// Send ramdisk to the phone
	ifVerbose cout << "Sending Device Tree '"<< temp << "'" << endl;
	CFStringRef deviceTree = CFStringCreateWithCString(kCFAllocatorDefault, temp.c_str(), kCFStringEncodingMacRoman);
	ret = sendFileToDevice(sh->recovery_dev, deviceTree);
	CFRelease(deviceTree);
	ifVerbose cout << "sendFileToDevice: " << ret << endl;
	if (ret != 0) {
		ifNotQuiet cout << "Send ramdisk failed.  Aborting." << endl;
		return SHELL_CONTINUE;
	}
	sleep(1);
	
	// load ramdisk on the phone
	ifVerbose cout << "Loading Device Tree." << endl;
	ret = sendCommandToDevice(sh->recovery_dev, CFSTR("devicetree"));
	ifVerbose cout << "sendCommandToDevice 'devicetree': " << ret << endl;
	if (ret != 0) {
		ifNotQuiet cout << "Load ramdisk failed.  Aborting." << endl;
		return SHELL_CONTINUE;
	}
	sleep(1);


	temp = args[1]+ "kernelcache.restore.release.s5l8900xrb";
	
	// Send the kernelcache
	ifVerbose cout << "Sending kernelcache '"<< temp << "'" << endl;
	CFStringRef kerncache = CFStringCreateWithCString(kCFAllocatorDefault, temp.c_str(), kCFStringEncodingMacRoman);
	ret = sendFileToDevice(sh->recovery_dev, kerncache);
	CFRelease(kerncache);
	ifVerbose cout << "sendFileToDevice: " << ret << endl;
	if (ret != 0) {
		ifNotQuiet cout << "Send kernelcache failed.  Aborting." << endl;
		return SHELL_CONTINUE;
	}
	sleep(1);
	
	//check for custom bootargs
	if( args[2] != "" )
		D("grestore does not yet except custom bootargs.");
	
	ifVerbose cout << "Setting bootargs: setenv boot-args rd=md0 -progress" << endl;
	//ret = sendCommandToDevice(sh->recovery_dev, CFSTR("setenv boot-args rd=md0 -progress debug=0x14e"));
	ret = sendCommandToDevice(sh->recovery_dev, CFSTR("setenv boot-args rd=md0 -progress debug=0x14e"));
	ifVerbose cout << "sendCommandToDevice: " << ret << endl;
	if (ret != 0) {
		ifNotQuiet cout << "Setevn command failed.  Aborting." << endl;
		return SHELL_CONTINUE;
	}
	sleep(1);
	
	ifVerbose cout << "Loading kernelcache (command: bootx)" << endl;
	ret = sendCommandToDevice(sh->recovery_dev, CFSTR("bootx"));
	ifVerbose cout << "sendCommandToDevice: " << ret << endl;	
	if (ret != 0) {
		ifNotQuiet cout << "failed (" << ret << ") Aborting." << endl;
		return SHELL_CONTINUE;
	}
	
	ifNotQuiet cout << "G-Restore Completed.  \nExit iPHUC or reconnect in order to continue in restore mode." << endl;
	
	return SHELL_CONTINUE;
}

int recovery_filecopytophone(string *args, struct shell_state *sh)
{
	CFStringRef file = CFStringCreateWithCString(NULL, args[1].c_str(), kCFStringEncodingMacRoman);
	mach_error_t retval = sendFileToDevice(	sh->recovery_dev, file );
	CFRelease(file);
	ifNotQuiet cout	<< "filecopytophone: " << retval << endl;
	//cout << "filecopytophone: " << filecopytophone(sh->recovery_dev, cline.c_str(), 0x09000000) << " bytes copied" << endl;
	
	return SHELL_CONTINUE;
}

int recovery_serial(string *args, struct shell_state *sh)
{
	ifNotQuiet cout << "Restarting in serial mode." << endl;
        
	D("bgcolor 255 255 0");
	sendCommandToDevice(sh->recovery_dev, CFSTR("bgcolor 255 255 0"));
	D("setenv debug-uarts 1");
	sendCommandToDevice(sh->recovery_dev, CFSTR("setenv debug-uarts 1"));
	D("saveenv");
	sendCommandToDevice(sh->recovery_dev, CFSTR("saveenv"));
	D("reboot");
	sendCommandToDevice(sh->recovery_dev, CFSTR("reboot"));
	
	//im ASSUMING we dont want to kill the app here, but that we want to
	// at least exit recovery mode (?)
	
	ifNotQuiet cout << "Please restart iPHUC or reconnect in order to issue serial commands." << endl;
	
	return SHELL_CONTINUE;
}

int recovery_cmd(string *args, struct shell_state *sh)
{
	
	if ( args[1] == "" )
	{
		ifNotQuiet cout << "Please provide a command to send to the phone." << endl;
		return SHELL_CONTINUE;
	}
	
	mach_error_t retval = sendCommandToDevice(	sh->recovery_dev,
							CFStringCreateWithCString(NULL, args[1].c_str(), kCFStringEncodingASCII) );
	
	ifVerbose cout	<< "cmd: " << args[1] << ": " << retval << endl; 
	
	return SHELL_CONTINUE;
}

int recovery_exit(string *args, struct shell_state *sh)
{
	D("exiting iphuc");
	exit(0);
}

int recovery_disconnect(string *args, struct shell_state *sh)
{
	return SHELL_WAIT;
}