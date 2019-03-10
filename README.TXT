Welcome to my program!

I'd like add to this readme that a tiny bit of my code
is borrowed from various sources, but wherever this
is the case i've included the source-link(typically above
said code, but also in the function docs)
Note: some code have maybe just been a bit inspired, but i've added
the sources in these cases aswell just to be clear that i wasnt the original
author of any technique that is inspired by another author.


-----------------------HOW TO RUN------------------------

First you need to set up the listening server, this is done
in the format
"./server jobfile port" (without quotes)
Then you run the connecting client in the format
"./client server-address port"

EXAMPLE:

1. ./server haiku.job 3000
2  ./client 127.0.0.1 3000

--------------------------
Great, now the server and client are talking!

Now, you'll get some options:
1. G=GET JOB
2. GALL=GET ALL REMAINING JOBS
3. GSEVERAL=GET SPECIFIED AMOUNT OF REMAINING JOBS
4. T=TERMINATE NORMALLY
5. E=TERMINATE BECAUSE OF ERROR

1. If you type in G and press enter, you will
grab one job from the server
2. If you type GALL you will get all remaining jobs
3. If you type GSEVERAL, you will be asked to specify
how many jobs you want to get,(if you request more than
remaining you will get all remaining and terminate program)
4. If you type T you will terminate both processes.
5. The same with E, but you will tell server that its because of an error.

If you ever try to get more jobs than what is in the file, the program
terminates.

             :::::::IMPORTANT:::::::
ERROR IVE ENCOUNTERED(IMPORTANT)

*Sometimes, especially if you manage to end server before client, the connection on
that specific port seems to be bugged out, so if you dont get an immediate answer from
the server in the next connection(on the same port), you have to change the port
(the old port will work after a few new connections on a new port).
In my latest updates of the program this seems to happen more rarely, but
keep in mind that this may happen, but the fix is as mentioned just running
on a different port.(between 1000 and 9999 is good)

I know this is very strange, but i have not figured out why this happens, and
i've focused on other things since this was fixed by simply changing the port.
