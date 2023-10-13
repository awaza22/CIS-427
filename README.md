# CIS-427
Group Project for CIS 427 Computer Networks

Programming Assignment 1:
Online Pokémon Cards Store
LINK TO VIDEO: https://youtu.be/1lEEdd9ryRY


--Update: We uploaded our latest version of the server file -----

We weren't able to upload our latest version of the code the group mate that has the file has no power so we had to submit an older one. We also added a screenshot of the DTE messages for the power outage that happened within her city as well. We can re-upload our latest version of the code when her power goes back on, but here's a copy of an older version of the code. Although the video we recorded shows all the latest updates we implemented.


Connect first to global protectant
Then you can run Bitvise SSH Client

After connected to UMICH Server-
Make sure you have uploaded both server and client files and the sqlite files (3 Sqlite files total) to the umich server: Bitvise SSH.

You will need a New Terminal console x 2 for client and server windows

Server:

g++ server.cpp -std=c++11 -ldl -pthread sqlite3.o -o server
./server

./server


Client:

g++ client.cpp -o client

./client 127.0.0.1

*Must include IP address to compile


LOGIN: There are several options to login with the login command-

LOGIN j_doe Passwrd4

LOGIN j_smith pass456

LOGIN c_brown Snoopy

LOGIN l_van Football

LOGIN l_blanket security23



