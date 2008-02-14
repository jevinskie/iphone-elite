//
//  ZiPhoneWindowController.m
//  ZiPhoneOSX
//

#import "ZiPhoneWindowController.h"

#define is_digit(c) ((unsigned)((c) - '0') <= 9)

#define PMT_NORMAL 1
#define PMT_ERROR 2
#define PMT_SUCCESS 3

@implementation ZiPhoneWindowController

/**
 * Handle initilization once the GUI is loaded.
 */
- (void)awakeFromNib {
  [self checkboxClicked:self];
  [m_btnStop setEnabled:NO];
}

- (void)dealloc {
  [m_processTask dealloc];
  m_processTask = nil;
  [super dealloc];
}

/**
 * Check if process is running and confirm before exiting.
 */
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
  if([m_processTask isRunning]) {
    [self stopProcess:self];
    return NSTerminateLater;
  } else {
    return NSTerminateNow;
  }
}

/**
 * Close the app when the window closes.
 */
- (void)windowWillClose:(NSNotification *)notification {
  if([notification object] == [self window]) {
    [NSApp terminate:self];
  }
}

/**
 * Check that a string is a valid IMEI - 15 digits.
 */
- (BOOL)checkImei:(NSString*)p_imei {
  if([p_imei length] != 15) {
    return NO;
  } else {
    const char *ch = [p_imei cString];
    int i;
    for(i=0; i<strlen(ch); i++) {
      if(!is_digit(ch[i])) {
         return NO;
      }
    }
  }
  
  return YES;
}

/**
 * Writes progress to the screen.
 */
- (void)writeProgress:(NSString*)p_str messageType:(int)p_msgType {
  NSDictionary *attributes = nil;
  switch(p_msgType) {
    case PMT_NORMAL:
      attributes = [NSDictionary dictionaryWithObjectsAndKeys:@"", @"NORMALTEXT", nil];
      break;
    case PMT_ERROR:
      attributes = [NSDictionary dictionaryWithObjectsAndKeys:[NSColor redColor], NSForegroundColorAttributeName, nil];
      break;
    case PMT_SUCCESS:
      attributes = [NSDictionary dictionaryWithObjectsAndKeys:[NSColor blueColor], NSForegroundColorAttributeName, nil];
      break;
  }
  
  NSAttributedString *attText = [[NSAttributedString alloc] initWithString:p_str attributes:attributes];
  NSTextStorage *ts = [m_txtProgress textStorage];
  [ts appendAttributedString:attText];
  [attText release];
  [m_txtProgress scrollRangeToVisible:NSMakeRange([ts length], 0)];
}

/**
 * Validate parameters, then start the unlocking process.
 */
- (IBAction)startProcess:(id)sender {
  if([m_btnChangeImei state] == NSOnState) {
    [m_txtImei validateEditing];
  }
  
  // Figure out the options:
  NSMutableArray *opts = [NSMutableArray arrayWithCapacity:7];
  
  // First get the unlock stuff taken care of
  if([m_btnDowngrade state] == NSOnState) {
    [opts addObject:@"-b"];
  } else {
    if([m_btnUnlock state] == NSOnState) {
      [opts addObject:@"-u"];
      
      if([m_btnChangeImei state] == NSOnState) {
        NSString *strImei = [m_txtImei stringValue];
        if(![self checkImei:strImei]) {
          NSAlert *lert = [NSAlert alertWithMessageText:@"Invalid IMEI" 
                                          defaultButton:@"OK" 
                                        alternateButton:nil 
                                            otherButton:nil 
                              informativeTextWithFormat:@"The new IMEI number must be exactly 15 digits"];
          [lert runModal];
          return;
        }
        [opts addObject:@"-i"];
        [opts addObject:strImei];
      }
    } else {
      if([m_btnErase state] == NSOnState) {
        [opts addObject:@"-e"];
      }
    }
    
    // Activate?
    if([m_btnActivate state]) {
      [opts addObject:@"-a"];
    }
    
    // Jailbreak?
    if([m_btnJailbreak state]) {
      [opts addObject:@"-j"];
    }
  }
  
  // Verbose?
  if([m_btnVerbose state]) {
    [opts addObject:@"-v"];
  }
  
  if([opts count] == 0) {
    NSAlert *lert = [NSAlert alertWithMessageText:@"No parameters selected" 
                                    defaultButton:@"OK" 
                                  alternateButton:nil 
                                      otherButton:nil 
                        informativeTextWithFormat:@"You must pick at least one action parameter from the checkboxes on the left."];
    [lert runModal];
    return;
  }
  
  // Prevent any changes while we run.
  [m_btnUnlock setEnabled:NO];
  [m_btnActivate setEnabled:NO];
  [m_btnJailbreak setEnabled:NO];
  [m_btnChangeImei setEnabled:NO];
  [m_btnVerbose setEnabled:NO];
  [m_btnErase setEnabled:NO];
  [m_txtImei setEnabled:NO];
  [m_lblImei setEnabled:NO];
  [m_btnDowngrade setEnabled:NO];
  
  
  // Toggle buttons
  [m_btnStart setEnabled:NO];
  [m_btnStop setEnabled:YES];
  
  
  NSString *toolPath = [[NSBundle mainBundle] pathForResource:@"ziphone" ofType:@""];
  
  // The ziphone binary could easily be lacking its X bit.
  [NSTask launchedTaskWithLaunchPath:@"/bin/chmod" arguments:[NSArray arrayWithObjects:@"+x", toolPath, nil]];  
  
  NSNotificationCenter *defaultCenter = [NSNotificationCenter defaultCenter];
    
  NSDictionary *defaultEnvironment = [[NSProcessInfo processInfo] environment];
  NSMutableDictionary *environment = [[NSMutableDictionary alloc] initWithDictionary:defaultEnvironment];
  m_processTask = [[NSTask alloc] init];
  [defaultCenter addObserver:self selector:@selector(taskCompleted:) name:NSTaskDidTerminateNotification object:m_processTask];
  [m_processTask setLaunchPath:toolPath];
  [m_processTask setArguments:opts];
  [m_processTask setCurrentDirectoryPath:[[NSBundle mainBundle] resourcePath]];
  [environment setObject:@"YES" forKey:@"NSUnbufferedIO"];
  //[m_processTask setEnvironment:environment];
  NSPipe *outputPipe = [NSPipe pipe];
  NSFileHandle *taskOutput = [outputPipe fileHandleForReading];
  [defaultCenter addObserver:self selector:@selector(taskDataAvailable:) name:NSFileHandleReadCompletionNotification object:taskOutput];
  [m_processTask setStandardOutput:outputPipe];
  [m_processTask setStandardError:outputPipe];
  
//  NSPipe *inputPipe = [NSPipe pipe];
//  NSFileHandle *taskInput = [inputPipe fileHandleForWriting];
//  [m_processTask setStandardInput:inputPipe];

  [m_processTask launch];
  [taskOutput readInBackgroundAndNotify];
  
  [environment release];
}

/**
 * Callback when task is completed.
 */
- (void)taskCompleted:(NSNotification *)notif {
  NSTask *theTask = [notif object];
  int exitCode = [theTask terminationStatus];
 
  if(exitCode != 0) {
    [self writeProgress:[NSString stringWithFormat:@"\n\nERROR: ziphone returned %d", exitCode] messageType:PMT_ERROR];
  } else {
    [self writeProgress:@"\n\nZiPhone completed successfully!" messageType:PMT_SUCCESS];
  }
  
  [[NSNotificationCenter defaultCenter] removeObserver:self];

  // Call this to clean up the GUI.
  [self stopProcess:self];
}

/**
 * Take data from the running task and display it to the screen.
 */
- (void)taskDataAvailable:(NSNotification *)notif {
  NSData *incomingData = [[notif userInfo] objectForKey:NSFileHandleNotificationDataItem];
  if (incomingData && [incomingData length]) {
    NSString *incomingText = [[NSString alloc] initWithData:incomingData encoding:NSASCIIStringEncoding];
    [self writeProgress:incomingText messageType:PMT_NORMAL];
    
    [[notif object] readInBackgroundAndNotify];
    [incomingText release];
  }
}

/**
 * Warn the user, then kill -9 ziphone.
 */
- (IBAction)stopProcess:(id)sender {
  if([m_processTask isRunning]) {
    NSAlert *lert = [NSAlert alertWithMessageText:@"Are you sure you want to kill ZiPhone?" 
                                    defaultButton:@"Keep Running" 
                                  alternateButton:@"Kill It"
                                      otherButton:nil 
                        informativeTextWithFormat:@"Killing ZiPhone now may cause damage to your phone and/or baseband!"];
    [lert setAlertStyle:NSCriticalAlertStyle];
    
    [lert beginSheetModalForWindow:[self window] modalDelegate:self didEndSelector:@selector(killAlertDidEnd:returnCode:contextInfo:) contextInfo:nil];
  } else {
    // It's not really running, so just cut to the chase...
    [self killAlertDidEnd:nil returnCode:NSAlertAlternateReturn contextInfo:NULL];
  }
}

/**
 * Handle the user's confirmation selection on kill.
 */
- (void)killAlertDidEnd:(NSAlert*) p_lert returnCode:(int) p_ret contextInfo:(void*)p_ctx {
  [[p_lert window] orderOut:self];
  
  if(p_ret == NSAlertAlternateReturn) {
    // She chose down....
        
    // Kill the task:
    if([m_processTask isRunning]) {
      [m_processTask terminate];
      [m_processTask dealloc];
      m_processTask = nil;
      [self writeProgress:@"\nZiPhone killed!" messageType:PMT_ERROR];
    }
    
    // Toggle buttons
    [m_btnStart setEnabled:YES];
    [m_btnStop setEnabled:NO];
    
    // Fix the checkboxes
    [m_btnUnlock setEnabled:YES];
    [m_btnActivate setEnabled:YES];
    [m_btnJailbreak setEnabled:YES];
    [m_btnChangeImei setEnabled:YES];
    [m_btnVerbose setEnabled:YES];
    [m_btnErase setEnabled:YES];    
    [m_btnDowngrade setEnabled:YES];
    [self checkboxClicked:self];
    
    // Let NSApp know it's time to go
    [NSApp replyToApplicationShouldTerminate:YES];
  } else {
    // Otherwise, keep going.
    [NSApp replyToApplicationShouldTerminate:NO];
  }
}

/**
 * Kill ZiPhone if necessary, then quit.
 */
- (IBAction)quitApplication:(id)sender {
  [NSApp terminate:self];
}

/**
 * Show the help/about box.
 */
- (IBAction)showAbout:(id)sender {
  if([m_btnChangeImei state] == NSOnState) {
    [m_txtImei validateEditing];
  }
  
  static BOOL bLoaded = NO;
  
  if(!bLoaded) {
    NSString *strReadMe = [[NSBundle mainBundle] pathForResource:@"README.txt" ofType:nil];
    NSString *strReadMeContents = [NSString stringWithContentsOfFile:strReadMe];
    
    
    [m_txtAbout insertText:strReadMeContents];
    
    bLoaded = YES;
  }
  
  [m_wndAbout makeKeyAndOrderFront:self];
}

/**
 * React to checkbox changes by validating parameters.
 *
 * This method will enable/disable checkboxes according to what's
 * possible based on other selections.
 *
 * ChangeIMEI requires Unlock.
 * Unlock disables iErase since they're for different firmwares.
 */
- (IBAction)checkboxClicked:(id)sender {
  if([m_btnDowngrade state] == NSOnState) {
    [m_btnErase setState:NSOffState];
    [m_btnErase setEnabled:NO];
    [m_btnChangeImei setState:NSOffState];
    [m_btnChangeImei setEnabled:NO];
    [m_txtImei setEnabled:NO];
    [m_lblImei setEnabled:NO];
    [m_btnUnlock setEnabled:NO];
    [m_btnUnlock setState:NSOffState];
    [m_btnJailbreak setEnabled:NO];
    [m_btnJailbreak setState:NSOffState];
    [m_btnActivate setEnabled:NO];
    [m_btnActivate setState:NSOffState];
  } else {
    [m_btnErase setEnabled:YES];
    [m_btnChangeImei setEnabled:YES];
    [m_btnUnlock setEnabled:YES];
    [m_btnJailbreak setEnabled:YES];
    [m_btnActivate setEnabled:YES];
  
    if([m_btnUnlock state] == NSOnState || [m_btnActivate state] == NSOnState ||
       [m_btnJailbreak state] == NSOnState || [m_btnErase state] == NSOnState) {
      [m_btnDowngrade setState:NSOffState];
      [m_btnDowngrade setEnabled:NO];
    } else {
      [m_btnDowngrade setEnabled:YES];
    }
    
    if([m_btnUnlock state] == NSOnState) {
      [m_btnErase setState:NSOffState];
      [m_btnErase setEnabled:NO];
      [m_btnChangeImei setEnabled:YES];
    } else {
      [m_btnErase setEnabled:YES];
      [m_btnChangeImei setState:NSOffState];
      [m_btnChangeImei setEnabled:NO];
    }
    
    if([m_btnChangeImei state] == NSOnState) {
      [m_txtImei setEnabled:YES];
      [m_txtImei setEditable:YES];
      [m_lblImei setEnabled:YES];
    } else {
      [m_txtImei setEnabled:NO];
      [m_lblImei setEnabled:NO];
    }
    
    if([m_btnErase state] == NSOnState) {
      [m_btnChangeImei setEnabled:NO];
      [m_btnChangeImei setState:NSOffState];
      [m_btnUnlock setEnabled:NO];
      [m_btnUnlock setState:NSOffState];
      [m_txtImei setEnabled:NO];
      [m_lblImei setEnabled:NO];
    } else {
      [m_btnUnlock setEnabled:YES];
    }
  }
  [m_txtImei validateEditing];
}



@end
