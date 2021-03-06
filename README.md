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

	```bash
	$sudo apt-get install ncurses5-dev
	```

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
	
	Again extract the archive,configure,build and install it. If 
	trying to run  gives error as "./acp: error while loading shared
	libraries: libconfig.so.9: cannot open shared object file:
	No such file or directory " then look for "libconfig.a" in the
	compilation directory (of libconfig). It should be in 
	"libconfig-1.4.9/lib/.libs" note that ".libs" is hidden directory.
	Now copy "libconfig.a" where Makefile of acp is located and edit 
	Makefile by replacing the word "-lconfig" with "libconfig.a". Now 
	compile again with make. Everything should be fine. :)

4. Increase terminal color display for better character display
	First get package ncurses-term followed by putting a line in your
	bashrc to make this full color suport available to all applications.
	In short run these commands.
	```bash
	$sudo apt-get install ncuress-term
	$echo "export TERM=\"xterm-256color\"" >> ~/.bashrc
	```
	That should solve the case.
	
Generating documentation
-------------------------
1. This project uses doxygen to generate documentation. Therefore install 
	following packages to be able to generate documentation. 

	```bash
	$sudo apt-get install doxygen,doxygen-latex,doxygen-doc
	```
	Note: doxygen-latex package is needed only if documentation is 
			required in pdf format. If pdf output is not required 
			then it can be explicitly disabled by editing file 
			"acp.doxyfile". Open "acp.doxyfile" with a text editor
			and search for "GENERATE_LATEX" and infront of it make 
			sure that "NO" is specified.
		
2. For to be able to generate graphs (call graphs, dependency graph etc)
	as part of the documentation get graphviz.

	```bash
	$sudo apt-get install graphviz,graphviz-dev
	```
3. Now generate documentation using following make command. The documentation
	is generated in a directory named "doxygen-output" under project root
	directory.

	```bash
	$make docs
	```