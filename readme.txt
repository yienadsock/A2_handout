# University of Leeds 2023-24
# COMP 5823M Assignment 1 Flight Simulator

UNIVERSITY LINUX:
=================

The university machines have QT 5.15.2 installed.

To compile on the University Linux machines, you will need to do the following:

[userid@machine A1_handout]$ module add legacy-eng
[userid@machine A1_handout]$ module add qt/5.15.2
[userid@machine A1_handout]$ qmake -project QT+=opengl LIBS+=-lGLU
[userid@machine A1_handout]$ qmake
[userid@machine A1_handout]$ make

You should see a compiler warning about an unused parameter, which can be ignored.

To execute the programe:

[userid@machine A1_handout]$ ./A1_handout 

RASPBIAN (RASPBERRY PI):
========================

If you have a raspberry pi, you can install QT 5 and the necessary tools with:
	sudo apt install gcc
	sudo apt install qt5-base5-dev qt5-qmake qtbase-5-dev-tools
	
This will install QT 5.15.2, and should work as described for the University machines.

UBUNTU 22.04.01:
================

Under Ubuntu, it is possible to install either QT 5 or 6.  This involves (at a minimum):
	sudo apt install gcc
and
	sudo apt install qt5-base5-dev qt5-qmake qtbase-5-dev-tools
or
	sudo apt install qt6-base-dev qt6-base-dev-tools qmake6

It should then work in much the same way as Raspbian or the university machines.

MAC OS X SONOMA:
================

To compile on your Mac running OS X Sonoma (earlier versions are similar):

[userid@machine A1_handout]$ qmake -project QT+=opengl QT+=openglwidgets QMAKE_CXXFLAGS+=-DGL_SILENCE_DEPRECATION
[userid@machine A1_handout]$ qmake
[userid@machine A1_handout]$ make

You will see some compiler warnings about deprecated use of x() and y().  These are due to changes in QT 6, which is what homebrew installs on OSX.  It will still compile.

On OSX, QT applications are stored in a "bundle" - a directory containing all of the resources the program needs in a UI environment.  To execute the application, use:

[userid@machine A1_handout]$ ./A1_handout.app/Contents/MacOS/A1_handout

You can also create a symbolic link after your compile:

[userid@machine A1_handout]$ ln -s ./A1_handout.app/Contents/MacOS/A1_handout A1_handout

after which you can invoke the executable as follows:

[userid@machine A1_handout]$ ./A1_handout

QT CREATOR:
===========

This code is intended to work at the command-line, since it makes it much easier to script for assessment purposes.  QT Creator makes this more difficult.  So if you run it under QT Creator, make sure you allow time for porting to the university command-line environment when you are done.

WINDOWS:
========
I am working on Windows instructions, and will make them available when I can.