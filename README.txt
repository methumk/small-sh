Program description:
    This program simulates a shell
    You can run commands in either background or foreground mode
    SIGINT is handled such that only the foreground processes terminate
    SIGTSTP is handled such that foreground and background processes terminate, if hit once, it goes into foreground mode (this ignores background command)
        if you hit it again, it will exit foregroun mode
    There are 3 built in functions: status, exit, and cd
        These cannot be run in the background
        status: shows previous terminating or exit status, 0 if nothing has ran before when you call status
        exit: exits the program
        cd: changes directory

Executable name:
    ./smallsh
Compiled with:
    GNU99
To compile program type:
    make
    ./smallsh

Test Executable:
    ./p3testscript
