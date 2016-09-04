#!/bin/sh
#Simple package extractor by Richard Thrippleton, public domain licensed
#
#If you're wondering how to extract this just (as root) type at the command line;
# sh <name of this file>

scriptlength=22
if (tail +$scriptlength $0 | gzip -cd | tar xvf - -C /)
then
	echo ""
	echo "		Star Voyager has been installed; type 'starvoyager' to play it"
	echo "		If you get an error in loading shared libraries, be"
	echo "		sure to install both SDL_net and SDL, available from"
	echo "		http://www.libsdl.org/projects/SDL_net/index.htm and"
	echo "		http://www.libsdl.org/ respectively"
else
	echo ""
	echo "		There was an error installing Star Voyager. Be sure"
	echo "		that you are logged in as root to install this package."
fi
exit
