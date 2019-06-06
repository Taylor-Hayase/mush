# mush
minimally usable shell - written in collaboration with Ryan Premi

This program can be compiled with make

Can be run with no command line arguments and then stdin,
or it can be given a file which hold commands in it
  ie. ./mush or ./mush cmd_file
 
To end the mush, send an EOF signal to stdin (control D)
  
The mush will be able to support
- redirections (>, <)
- piping (|)
- cd
- SIGINT support, which will handle any running child processes without killing the shell.

Limits
- The maximum command line length is 512 bytes
- The maximun amount of pipes is 10
- The maximum amount of arguments in a command is 10
