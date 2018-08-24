#!/bin/sh

autoreconf -ivf && ./configure;
if [[ $? == 0 ]]; then
	echo;
	echo "Now type 'make' to build the project.";
	echo;
fi
