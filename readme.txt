State of the project: usable, but performance is very poor and only the basic features are in.

        Building ode:

If you're running windows first download the opl, osg, and ode repositories from https://github.com/oddthread/

After you have the three repositories in the same directory, open a command prompt in windows, cd to the ode/script/build directory and run the rebuild batch file. You have to have SDL2 installed as well from libsdl.org and the microsoft C compiler that you get with Visual Studio.

If you have an issue where cl is unable to find the proper directories type vcvars32 in the command prompt. It's a Visual Studio batch file that sets up the include path variables.

If you're running linux use the make_gcc.bash file.

If you're running OSX then you'll have to install SDL2 and create an Xcode project yourself until I port it to OSX. Note: this is a nightmare but there are plenty of tutorials you can find.

At any point in time build files for any given platform may be out of date but the linux files usually are up-to-date.

    Some goals of this editor:

Extremely responsive and performant - a frame should never pass where input isn't recieved and handled, fast startup/close time, etc. After the basic features are done most time will be spent on optimization.

Parameterized macros (things like generating for loops), I have used Auto Hotkey in the past for this but it would be nice for the editor to do it.

Having a super simple and minimal UI, most/all of the UI should actually just be text files (like configuration files, similar to Visual Code).

Decent autocomplete (will work on this after writing a parser).

Remote text editing for collaboration. This isn't a crucial feature but there aren't really any editors that do it well.
