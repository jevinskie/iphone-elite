     :::::::::   ::::::::::: :::::::::  :::    :::  ::::::::  ::::    ::: :::::::::: 
           :+:      :+:     :+:    :+: :+:    :+: :+:    :+: :+:+:   :+: :+:         
         +:+       +:+     +:+    +:+ +:+    +:+ +:+    +:+ :+:+:+  +:+ +:+          
       +#+        +#+     +#++:++#+  +#++:++#++ +#+    +:+ +#+ +:+ +#+ +#++:++#      
     +#+         +#+     +#+        +#+    +#+ +#+    +#+ +#+  +#+#+# +#+            
   #+#          #+#     #+#        #+#    #+# #+#    #+# #+#   #+#+# #+#             
 ######### ########### ###        ###    ###  ########  ###    #### ##########       

Ziphone version 2.3 (16th Feb 2008)

For a full restore do:

ziphone -D

then run itunes and restore to 1.1.3 (or 1.1.2 if you like)

then at the end of restore run ziphone again:

ziphone -b -a -j

if you already used ziphone on your phone, you can skip the above command and issue only:

ziphone -a -j  (after the restore)

If you find any missing icons.. or the iphone is "strange" it's because of a WRONG backup in iTunes.
So, do not RESTORE a backup and set the iPhone as a NEW PHONE.

(This problem happens if you backed up things from a 'soft updated' iphone with 'dev team method'.)

Updated:

No need to set Verbose mode (its default)

To use installer you must firstly install the "BSD Subsystem" package

Usage

Ziphone -j  = Jailbreak
Ziphone -a = Activate
Ziphone -u = Unlock (Works on both BL3.9 and BL4.6)
Ziphone -e = Erase Baseband (BL 3.9 only)
Ziphone -b = Downgrade Bootloader from 4.6 to 3.9, update baseband to 4.03.13 and patch the unlock.

Ziphone  -D: Enter DFU Mode.
Ziphone  -R: Enter Recovery Mode.
Ziphone  -N: Exit Recovery Mode (normal boot).
Ziphone  -C: Make coffee.

*** only use -b if your iphone is 1.1.2/1.1.3 OOB ***

It is also possible to combine the functions e.g ziphone -a -j -u

*Added Windows batch files for n00bs*

ClickHereX3.9.bat Will call Ziphone -u -j -a (use with iphones that were < 1.1.1 OOB)

ClickHereX4.6.bat will call Ziphone -b -j -a (Downgrades bootloader be warned, use for iphone > 1.1.2)

Jailbreak will work on any OS version.

Activation will work on any OS version, except for youtube on 1.0.X (I am lazy i know)

Details on how it works and WHY it works are available on http://www.iphone-elite.org

No 'dev-team' was directly involved with this work.
This program is based on the work of everyone who
believed in free software. (Thanks for the toolchain!)

Thanks to geohot for his latest work.
Thanks to Tissy for moral support.
Thanks to iphone-elite.org for support.
Thanks to Viper and Tifel and ortner for being so nice always.
Thanks to PENDOR for the osx gui
Thanks to mzaouara for the ascii art (he didn't use some generator he's an ascii artist !)
Thanks to PsxGunDown to review this readme.

Namaste,
Zibri.