#if defined(WIN32)
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#	include <CoreFoundation.h>
#	define sleep(seconds) Sleep(seconds*1000)
#	define CFSTRINGENCODING kCFStringEncodingASCII
#endif

#if defined(__APPLE__)
#	include <CoreFoundation/CoreFoundation.h>
#	include "GetPID.h"
#	define CFSTRINGENCODING kCFStringEncodingMacRoman
#endif

#include <signal.h>
#include <sys/stat.h>
#include <iostream>
#include "MobileDevice.h"
#include "privateFunctions.h"
#include "md5.h"

using namespace std;

bool done=false;
int Stage=0;
int RecoverCount=0;
int progress=-1;
char progressStr[64]="";
char errorStr[64]="";
unsigned char result[16]="";
int lasterr;
bool ExitAfterStage=false;
bool unlock=false;
bool activate=false;
bool jailbreak=false;
bool chimei=false;
bool ierase=false;
bool bl39=false;
char imei[127]="setenv imei ";

char ramdisk[128]="zibri.dat";

unsigned char rdmd5[16]={0x47,0x6f,0x64,0x7a,0x96,0x35,0x5e,0xeb,0xfa,0x8b,0xfe,0xa3,0x95,0x06,0x16,0x3c};

CFStringRef StringtoCFString(string input) {
   return CFStringCreateWithCString(NULL, input.c_str(), CFSTRINGENCODING);
}

/* * * ( Function Declarations ) * * */

void RestoreNotificationCallback(am_recovery_device *rdev);
void DeviceNotificationCallback(am_device_notification_callback_info *info);
void DisconnectRecoveryCallback(struct am_recovery_device *rdev);
void ConnectRecoveryCallback(struct am_recovery_device *rdev);
void KillStupidHelper();

/* * * ( Error and Progress Reporting ) * * */

void ProgressStep(const char *message) {
	strncpy(progressStr,message,63);
	cout << progressStr << endl;
	sleep(1); // for the user
}


void ReportDone() {
#if defined(__APPLE__)
	CFRunLoopStop(CFRunLoopGetCurrent());
#endif
     done=true;	
}


void ReportError(const char *message) {
	strncpy(errorStr,message,63);
	cout << "Failed: (" << lasterr << ") " << errorStr << endl;
	ReportDone();
}


/* * * ( File Operations ) * * */


/* * * ( Stages ) * * */

void PairIPhone(am_device *iPhone) {
	if(AMDeviceConnect(iPhone))            throw "AMDeviceConnect failed.";
	if(!AMDeviceIsPaired(iPhone))          throw "Pairing Issue.";
	if(AMDeviceValidatePairing(iPhone)!=0) throw "Pairing NOT Valid.";
	if(AMDeviceStartSession(iPhone))       throw "Session NOT Started.";
}


void Stage0() { // Register callbacks
	initPrivateFunctions();

	ProgressStep("Searching for iPhone...");

	if(!ExitAfterStage) Stage=2;

	am_device_notification *notif;
	AMDeviceNotificationSubscribe(DeviceNotificationCallback, 0, 0, 0, &notif);
	AMRestoreRegisterForDeviceNotifications(NULL, ConnectRecoveryCallback, NULL, DisconnectRecoveryCallback, 0, NULL);
}


void Stage2(struct am_recovery_device *rdev) { // Booting in recovery mode
	try {
//		KillStupidHelper();

		ProgressStep("Working...");
			
		rdev->callback = &RestoreNotificationCallback;
		rdev->user_info = NULL;
		
		if (ierase) {
           unlock=false;
           jailbreak=false;
           activate=false;
           chimei=false;
        }

		if (bl39) {
           unlock=true;
           jailbreak=false;
           activate=false;
           chimei=false;
           ierase=false;
        }

		sendCommandToDevice(rdev,CFStringCreateWithCString(kCFAllocatorDefault, "setenv auto-boot true", kCFStringEncodingUTF8));
		sendCommandToDevice(rdev,CFStringCreateWithCString(kCFAllocatorDefault, "setpicture 0", kCFStringEncodingUTF8));
		sendCommandToDevice(rdev,CFStringCreateWithCString(kCFAllocatorDefault, "bgcolor 0 0 64", kCFStringEncodingUTF8));
		sendFileToDevice(rdev, StringtoCFString(ramdisk));
if (bl39)		    sendCommandToDevice(rdev,CFStringCreateWithCString(kCFAllocatorDefault, "setenv bl39 1", kCFStringEncodingUTF8));
if (unlock)		    sendCommandToDevice(rdev,CFStringCreateWithCString(kCFAllocatorDefault, "setenv unlock 1", kCFStringEncodingUTF8));
if (jailbreak)		sendCommandToDevice(rdev,CFStringCreateWithCString(kCFAllocatorDefault, "setenv jailbreak 1", kCFStringEncodingUTF8));
if (activate)		sendCommandToDevice(rdev,CFStringCreateWithCString(kCFAllocatorDefault, "setenv activate 1", kCFStringEncodingUTF8));
if (ierase) 		sendCommandToDevice(rdev,CFStringCreateWithCString(kCFAllocatorDefault, "setenv ierase 1", kCFStringEncodingUTF8));
if (chimei) 		sendCommandToDevice(rdev,CFStringCreateWithCString(kCFAllocatorDefault, imei, kCFStringEncodingUTF8));
              		sendCommandToDevice(rdev,CFStringCreateWithCString(kCFAllocatorDefault, "setenv boot-args rd=md0 -s -x pmd0=0x09CC2000.0x0133D000", kCFStringEncodingUTF8));
              		sendCommandToDevice(rdev,CFStringCreateWithCString(kCFAllocatorDefault, "saveenv", kCFStringEncodingUTF8));
              		sendCommandToDevice(rdev,CFStringCreateWithCString(kCFAllocatorDefault, "fsboot", kCFStringEncodingUTF8));

if (!unlock&&(activate||jailbreak)) ProgressStep("Please wait 45\".");
if (unlock && (!bl39)) ProgressStep("Please wait 2\'30\".");
if (bl39) ProgressStep("Please wait 4'00\".");

        Stage=9;

		if(ExitAfterStage) exit(0);

	} catch(const char*error) {
		ReportError(error);
		exit(9);
	}
}

/* * * ( Callback functions ) * * */

void RestoreNotificationCallback(am_recovery_device *rdev) {
//	cout << "RestoreNotificationCallback" << endl;
}

void DeviceNotificationCallback(am_device_notification_callback_info *info) {
	//cout << "DeviceNotificationCallback" << endl;

	if(info->msg==ADNCI_MSG_CONNECTED) {
        if (Stage!=9) {
      		cout << "Entering recovery mode." << endl;

         PairIPhone(info->dev);
         AMDeviceEnterRecovery(info->dev);

           }
         else ReportDone(); 
		}
}


void DisconnectRecoveryCallback(struct am_recovery_device *rdev) {
//	cout << "DisconnectRecoveryCallback" << endl;
	// this is a stub, but it looks like it gives a Bus error if you don't use it.
}


void ConnectRecoveryCallback(struct am_recovery_device *rdev) {
     if(Stage==2) Stage2(rdev);
}

void KillStupidHelper() {
#if defined(__APPLE__)
/*	int pid = GetPIDForProcessName("iTunesHelper");;

	if(pid > 0) {
		ProgressStep("Killing iTunesHelper...");
		kill(pid, SIGKILL);
	} */

	int pid = GetPIDForProcessName("iTunes");;

	if(pid > 0) {
		ProgressStep("Killing iTunes...");
		if(kill(pid, SIGKILL) == EPERM) {
			cout << "Could not kill iTunes! Aborting..." << endl;
			exit(9);
		}
	}
#endif
}


/* * * ( Main ) * * */

bool temp_file_exists(const char *filename) {
int count=0;
	FILE *pFile=fopen(filename,"rb");
	if(pFile) {
		fclose(pFile);
		md5_file(ramdisk, result);
    for (count = 0; count < 16; count++) 
   {
     if (result[count] != rdmd5[count])
     {

     cout << "Wrong zibri.dat version !" << endl;
     cout << "Go get the full archive at http://www.ziphone.org" << endl;

//      printf("md5: 0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x\n",result[0], result[1], result[2], result[3], result[4], result[5], result[6], result[7], result[8], result[9], result[10], result[11], result[12], result[13], result[14], result[15]);
//      printf("rdmd5: 0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x\n",rdmd5[0], rdmd5[1], rdmd5[2], rdmd5[3], rdmd5[4], rdmd5[5], rdmd5[6], rdmd5[7], rdmd5[8], rdmd5[9], rdmd5[10], rdmd5[11], rdmd5[12], rdmd5[13], rdmd5[14], rdmd5[15]);

       return false;
       break;
     } else return true;
    }
   }

	cout << filename << " could not be opened for reading." << endl;
	ReportDone();
	return false;
}


bool parse_args(int argc,char *argv[]) {

	for(int i=1;i<argc;i++) {
		if(argv[i][0]=='-') {
			if(argv[i][1]=='u') unlock=true;
			else if(argv[i][1]=='e') ierase=true ;
			else if(argv[i][1]=='t') ;
			else if(argv[i][1]=='b') bl39=true ;
			else if(argv[i][1]=='a') activate=true ;
			else if(argv[i][1]=='j') jailbreak=true ;
			else if(argv[i][1]=='i') {
                 if (argc<(i+2)) return false;
                 if (strlen(argv[i+1])!=16) return false;
                 chimei=true;
                 strcat(imei,argv[i+1]);
                 unlock=true;
            }
			else return false;
			
		}
	}

	return true;
}

void Banner() {
     cout << endl << "ZiPhone v2.1 by Zibri. http://www.ziphone.org" << endl;
     cout << "Source code available at: http://www.iphone-elite.org" << endl;
}

void Usage() {
//     cout << endl << "ZiPhone v2.1 by Zibri. http://www.ziphone.org" << endl;
//     cout << "Source code available at: http://www.iphone-elite.org" << endl;
     Banner();
     cout << endl << "Usage: ziphone [-b] [-e] [-u] [-a] [-j] [-i imei]" << endl;
     cout << "                -b: Downgrade bootloader and flash/unlock 4.03" << endl;
     cout << "                -u: Unlock (4.6 AND 3.9 BL !)" << endl;
     cout << "                -a: Activate" << endl;
     cout << "                -j: Jailbreak" << endl;
     cout << "                -i: Change imei (4.6 AND 3.9 BL !)" << endl;
     cout << "                -e: iErase BL 3.9 baseband" << endl;
     }

int main(int argc,char *argv[]) {

	if((argc<=1)||(parse_args(argc,argv)==false)) {
    Usage(); 
    return 9;
    }

	if(!ExitAfterStage) {
//     cout << endl << "ZiPhone v2.1 by Zibri. http://www.ziphone.org" << endl;
//     cout << "Source code available at: http://www.iphone-elite.org" << endl;
    Banner();
	}

	if(Stage==0) {
		cout << endl << "Loading " << ramdisk << "." << endl;
		
		if(!(temp_file_exists(ramdisk))) return 9;
	}

	Stage0();
#if defined(__APPLE__)
	CFRunLoopRun();
#else
     while(!done) {
        Sleep(1);
        if (Stage==9) {
//           Sleep(1000);
//           cout << ".";
        }
     }
     cout << "Done!" << endl;
     sleep(2);
#endif

	return 0;
}
