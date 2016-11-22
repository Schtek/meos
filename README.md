# meos
MeOS is a full featured orienteering event organizing program (http://www.melin.nu/meos/en/).

This repository is dedicated to improvements and more specifically compatibility with French Federation of Orienteering formats.
It is based on development version 3.4.616 snapshot 2016-08-15 and the strategy is to contribute back these developments to the official project.

## Caution
* __This is experimental software. Backup your event data before executing this version of meos.__
* This version is based on meos 3.4 and data once migrated are not compatible anymore with meos 3.3. 

## How to use it?
meos 3.4 has been ported to Microsoft Visual Studio 2015.
To build it, it is therefore required to install this environment of development (available at https://www.microsoft.com/france/visual-studio/produits/community/Default.aspx).
Then:
* Open with it the solution file _meos_vc15.sln_
* Check that the configuration is _Debug_ / _x86_
* Press _Local Windows Debugger_. You can safely ignore build warnings.
* Accept to launch the built executable.
* Enjoy :)

