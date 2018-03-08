# nanoinstall
Tiny installer system

This project is the very beginning of an installer system.

The download_demo demonstrates two basic needed abilities:
1. Downloading some data from a URL via Windows libraries. 2. Reading data appeneded onto the end of the file.
The intent here is that a script could be appended to the installer that determines what it does.
The type of script meant to be used is liteml. See the liteml project for more information on that.

The ui_demo demonstrates the ability to draw images, text, and buttons from configuration. Once again, this
functionality is meant to be tied into liteml so that liteml can be used to specify the commands to be run.
The purpose of this demo is to show that the basic elements needed for an installer can be built into a very small
executable.

