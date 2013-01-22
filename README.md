acp
===

A program which simulates how the compressed caching is better than the usual
paging and swapping process. So far only UI is ready which is a ncurses based
UI. :)

External Package depedencies:
----------------------------
1. Curses Library:
	ACP ui is a curses-based UI system for linux. On a ubuntu machine install
	it using:
	
	'''bash
	$sudo apt-get install ncurses5-dev
	'''

	Also install if any related package dependency is suggested by packager.

2. Curses development Kit (CDK)
	CDK is a high level library which enables a developer to use ready-to-use
	curses based widgets. For complex designs, plain curses becomes a pain. 
	There comes CDK.

	However CDK available in standard repositories has a few issues. So get 
	it from here. 

		http://invisible-island.net/datafiles/release/cdk.tar.gz

	Extract the archive,configure,build and install it.

3. libconfig
	A library for parsing text based configuration files. Get the source 
	from :

		http://www.hyperrealm.com/libconfig/libconfig-1.4.9.tar.gz
	
	Again extract the archive,configure,build and install it.
