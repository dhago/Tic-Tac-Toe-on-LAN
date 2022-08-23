/////////////TICTACTOE//////////////
Commands:
	Compile the codes
	a) gcc -pthread server.c -o server
	b) gcc client.c -o client
	To run,
	-> ./server
	-> ./client
Multiple games can be held.(Multi-threaded approach)
Change max number of players in the server code.(LINE 11)
PORT number has to be changed in the code.(line 10 in server.c, line 14 in client.c)
Timeout has been set to 15 seconds for the clients to provide any inputs.
Inputs:
	Server is non interactive.
	In the clients, (ROW,COL) input is given as 1,2 (for example)
	Yes is given by y
	No is given by n
Log file:
	Contains info of the games that ran in the server.
	Contains series of moves performed.
	In a game, on each rematches, starting turn is exchanged.
	
/////////////YAPP/////////////
Yet another ping program.
To set timeout thresholds, change the #defines in yapp.c
To run -> gcc yapp.c -o yapp
       -> sudo ./yapp X.X.X.X
Returns RTT if ping was succesful.
(Checksum and basic structure borrowed from ping's implementation)
