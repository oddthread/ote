Building OTE

If you're running windows first download the OSAL, OTG and OTE repositories from https://github.com/oddthread/

After you have the three repositories in the same directory, open a command prompt in windows, cd to the OTE directory and run the rebuild batch file. You have to have SDL2 installed as well from libsdl.org and the microsoft C compiler that you get with Visual Studio.

If you have an issue where cl is unable to find the proper directories type vcvars32 in the command prompt. It's a Visual Studio batch file that sets up the include path variables.

If you're running linux you'll have to write the makefiles yourself until I work on porting it (it may not compile on GCC at the moment).

If you're running OSX then you'll have to install SDL2 and create an Xcode project yourself until I port it to OSX. Note: this is a nightmare but there are plenty of tutorials you can find.