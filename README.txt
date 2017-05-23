Building OTE

If you're running windows first download the OSAL, OTG and OTE repositories from https://github.com/oddthread/

After you have the three repositories in the same directory, open a command prompt in windows, cd to the OTE directory and run the rebuild batch file. You have to have SDL2 installed as well from libsdl.org and the microsoft C compiler that you get with Visual Studio.

If you have an issue where cl is unable to find the proper directories type vcvars32 in the command prompt.

If you're running linux then I'm sure you will be able to figure it out.

If you're running OSX then switch to linux.