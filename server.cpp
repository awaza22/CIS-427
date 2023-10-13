#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "sqlite3.h"
using namespace std;

#define SERVER_PORT  1163
#define MAX_PENDING  5
#define MAX_LINE     256

struct sockaddr_in srv;
char buf[MAX_LINE];
socklen_t buf_len, addr_len;
int nRet;
int nClient[10] = { 0, };
int nSocket;
string clientInfo[3];

sqlite3* db;
char* zErrMsg = 0;
const char* sql;
int rc;
string result;
string* ptr = &result;


typedef struct
{
    int socket;
    int id;
    string user;
    string password;
}userInfo;

typedef struct
{
    string ip;
    string user;
    int socket;
    pthread_t myThread;
}loggedUser;

void* temp = malloc(sizeof(userInfo));
userInfo u;
vector<loggedUser> list;
fd_set fr;
fd_set fw;
fd_set fe;
int nMaxFd;
pthread_t thread_handles;
long thread;

void NewClientConnection();
void DataFromClient();

string buildCommand(char*);
void* databaseCommands(void*);
static int callback(void*, int, char**, char**);

string getData(char*, string);
bool getData(char*, string*, string);

string clientPassword(char line[], int n) {
    int spaceLocation = n + 2;
    int i = spaceLocation;
    string info = "";
    while (line[i] != '\n') {
        if (line[i] == 0)
            return "";
        if (line[i] == ' ')
            return info;
        info += line[i];
        i++;
    }
    return info;
}



int main(int argc, char* argv[]) {
#pragma region Database Setup
// Open Database and Connect to Database
    rc = sqlite3_open("cis427PokeCard200023.sqlite", &db);
// Check that Database was opened successfully
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return(0);
    }
    else {
        fprintf(stderr, "Opened database successfully\n");
    }
// Create sql Users table creation command
    sql = "create table if not exists Users\
    (\
        ID INTEGER PRIMARY KEY AUTOINCREMENT,\
        first_name TEXT,\
        last_name TEXT,\
        user_name TEXT NOT NULL,\
        password TEXT,\
        email TEXT NOT NULL,\
        usd_balance DOUBLE NOT NULL\
    );";

// Execute Users table creation
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);

// Create sql Pokemon_cards table creation command
    sql = "create table if not exists Pokemon_cards (\
        ID INTEGER PRIMARY KEY AUTOINCREMENT,\
        Card_name varchar(10) NOT NULL,\
        Card_type TEXT,\
        rarity TEXT,\
        quantity INTEGER,\
        Card_price DOUBLE,\
        owner_id TEXT,\
        FOREIGN KEY(owner_id) REFERENCES Users(ID)\
    );";

// Create Users Table Values
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
// First Check that there is no root user, then add.
    sql = "SELECT IIF(EXISTS(SELECT 1 FROM Users WHERE  Users.user_name='root'), 'USER_PRESENT', 'USER_NOT_PRESENT') result;";
    rc = sqlite3_exec(db, sql, callback, ptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else if (result == "USER_NOT_PRESENT") {
// Create the root user:
        fprintf(stdout, "Root user is not present. Attempting to add the user.\n");
// Add a root user
        sql = "INSERT INTO Users VALUES (1, 'Root', 'User', 'root', 'root01', 'cis427PokemonCards@abc.com', 100);";
        rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else {
            fprintf(stdout, "\tA root user was added successfully.\n");
        }
    }
    else if (result == "USER_PRESENT") {
        cout << "\tThe root user already exists in the Users table.\n";
    }
    else {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        cout << "Error returned Result = " << result << endl;
    }
// Check that john exists. No? Create entry
    sql = "SELECT IIF(EXISTS(SELECT 1 FROM Users WHERE  Users.user_name='j_doe'), 'USER_PRESENT', 'USER_NOT_PRESENT') result;";
    rc = sqlite3_exec(db, sql, callback, ptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else if (result == "USER_NOT_PRESENT") {
        fprintf(stdout, "john is not present. Attempting to add the user.\n");
// Add john to Pokemon Database
        sql = "INSERT INTO Users VALUES (2, 'john', 'doe', 'j_doe', 'Passwrd4', 'j.doe@abc.com', 100);";
        rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else {
            fprintf(stdout, "\tA new user (john) was added successfully.\n");
        }
    }
    else if (result == "USER_PRESENT") {
        cout << "\tjohn already exists in the Users table.\n";
    }
    else {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        cout << "Error returned Result = " << result << endl;
    }
// Check that jane exists. No? Create entry
    sql = "SELECT IIF(EXISTS(SELECT 1 FROM Users WHERE  Users.user_name='j_smith'), 'USER_PRESENT', 'USER_NOT_PRESENT') result;";
    rc = sqlite3_exec(db, sql, callback, ptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else if (result == "USER_NOT_PRESENT") {
        fprintf(stdout, "jane is not present. Attempting to add the user.\n");
// Add jane
        sql = "INSERT INTO Users VALUES (3, 'jane', 'smith', 'j_smith', 'pass456', 'j.smith@abc.com', 90);";
        rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else {
            fprintf(stdout, "\tA new user (jane) was added successfully.\n");
        }
    }
    else if (result == "USER_PRESENT") {
        cout << "\tjane already exists in the Users table.\n";
    }
    else {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        cout << "Error returned Result = " << result << endl;
    }   
// Check that charlie exists. No? Create entry
    sql = "SELECT IIF(EXISTS(SELECT 1 FROM Users WHERE  Users.user_name='c_brown'), 'USER_PRESENT', 'USER_NOT_PRESENT') result;";
    rc = sqlite3_exec(db, sql, callback, ptr, &zErrMsg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else if (result == "USER_NOT_PRESENT") {
        fprintf(stdout, "charlie is not present. Attempting to add the user.\n");
// Add charlie to Pokemon Database
        sql = "INSERT INTO Users VALUES (4, 'charlie', 'brown', 'c_brown', 'Snoopy', 'c.brown@abc.com', 90);";
        rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else {
            fprintf(stdout, "\tA new user (charlie) was added successfully.\n");
        }
    }
    else if (result == "USER_PRESENT") {
        cout << "\tcharlie already exists in the Users table.\n";
    }
    else {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        cout << "Error returned Result = " << result << endl;
    }
// Check that lucy exists. No? Create entry
    sql = "SELECT IIF(EXISTS(SELECT 1 FROM Users WHERE  Users.user_name='l_van'), 'USER_PRESENT', 'USER_NOT_PRESENT') result;";
    rc = sqlite3_exec(db, sql, callback, ptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else if (result == "USER_NOT_PRESENT") {
        fprintf(stdout, "lucy is not present. Attempting to add the user.\n");
// Add lucy to Pokemon Database
        sql = "INSERT INTO Users VALUES (5, 'lucy', 'van', 'l_van', 'Football', 'l.van@abc.com', 70);";
        rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else {
            fprintf(stdout, "\tA new user (lucy) was added successfully.\n");
        }
    }
    else if (result == "USER_PRESENT") {
        cout << "\tlucy already exists in the Users table.\n";
    }
    else {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        cout << "Error returned Result = " << result << endl;
    }
// Check that linus exists. No? Create entry
    sql = "SELECT IIF(EXISTS(SELECT 1 FROM Users WHERE  Users.user_name='l_blanket'), 'USER_PRESENT', 'USER_NOT_PRESENT') result;";
    rc = sqlite3_exec(db, sql, callback, ptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else if (result == "USER_NOT_PRESENT") {
        fprintf(stdout, "linus is not present. Attempting to add the user.\n");
// Add linus to Pokemon Database
        sql = "INSERT INTO Users VALUES (6, 'linus', 'blanket', 'l_blanket', 'security23', 'l.blanket@abc.com', 90);";
        rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else {
            fprintf(stdout, "\tA new user (linus) was added successfully.\n");
        }
    }
    else if (result == "USER_PRESENT") {
        cout << "\tlinus already exists in the Users table.\n";
    }
    else {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        cout << "Error returned Result = " << result << endl;
    }

// Check that Pikachu exists. No? Create entry
    sql = "SELECT IIF(EXISTS(SELECT 1 FROM Pokemon_cards WHERE  Pokemon_cards.POKEMON_CARD_name='Pikachu'), 'POKEMON_CARD_PRESENT', 'POKEMON_CARD_NOT_PRESENT') result;";
    rc = sqlite3_exec(db, sql, callback, ptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else if (result == "POKEMON_CARD_NOT_PRESENT") {
        fprintf(stdout, "Pikachu is not present. Attempting to add the card.\n");
// Add Pikachu to Pokemon Database
        sql = "INSERT INTO Pokemon_cards VALUES (1, 'Pikachu', 'Electric', 'Common', 2, 19.99, 2);";
// sql = "INSERT INTO Pokemon_cards VALUES (1, 'Pikachu', 19.99, 'Electric', 'Common', 2);";
        rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else {
            fprintf(stdout, "\tA new card (Pikachu) was added successfully.\n");
        }
    }
    else if (result == "POKEMON_CARD_PRESENT") {
        cout << "\tcard already exists in the Pokemon_cards table.\n";
    }
    else {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        cout << "Error returned Result = " << result << endl;
    }
// Check that Charizard exists. No? Create entry
    sql = "SELECT IIF(EXISTS(SELECT 1 FROM Pokemon_cards WHERE  Pokemon_cards.POKEMON_CARD_name='Charizard'), 'POKEMON_CARD_PRESENT', 'POKEMON_CARD_NOT_PRESENT') result;";
    rc = sqlite3_exec(db, sql, callback, ptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else if (result == "POKEMON_CARD_NOT_PRESENT") {
        fprintf(stdout, "Charizard is not present. Attempting to add the card.\n");
// Add Charizard to Pokemon Database
        sql = "INSERT INTO Pokemon_cards VALUES (2, 'Charizard', 'Fire', 'Rare', 1, 15.49, 2);";
        rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else {
            fprintf(stdout, "\tA new card (Charizard) was added successfully.\n");
        }
    }
    else if (result == "POKEMON_CARD_PRESENT") {
        cout << "\tCharizard already exists in the Pokemon_cards table.\n";
    }
    else {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        cout << "Error returned Result = " << result << endl;
    }
// Check that Bulbasaur exists. No? Create entry
    sql = "SELECT IIF(EXISTS(SELECT 1 FROM Pokemon_cards WHERE  Pokemon_cards.POKEMON_CARD_name='Bulbasaur'), 'POKEMON_CARD_PRESENT', 'POKEMON_CARD_NOT_PRESENT') result;";
    rc = sqlite3_exec(db, sql, callback, ptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else if (result == "POKEMON_CARD_NOT_PRESENT") {
        fprintf(stdout, "Bulbasaur is not present. Attempting to add the card.\n");
// Add Bulbasaur to Pokemon Database
        sql = "INSERT INTO Pokemon_cards VALUES (3, 'Bulbasaur',  'Grass', 'Common', 50, 11.9, 4);";
        rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else {
            fprintf(stdout, "\tA new card (Bulbasaur) was added successfully.\n");
        }
    }
    else if (result == "POKEMON_CARD_PRESENT") {
        cout << "\tBulbasaur already exists in the Pokemon_cards table.\n";
    }
    else {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        cout << "Error returned Result = " << result << endl;
    }
// Check that Squirtle exists. No? Create entry
    sql = "SELECT IIF(EXISTS(SELECT 1 FROM Pokemon_cards WHERE  Pokemon_cards.POKEMON_CARD_name='Squirtle'), 'POKEMON_CARD_PRESENT', 'POKEMON_CARD_NOT_PRESENT') result;";
    rc = sqlite3_exec(db, sql, callback, ptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else if (result == "POKEMON_CARD_NOT_PRESENT") {
        fprintf(stdout, "Squirtle is not present. Attempting to add the card.\n");
// Add Squirtle to Pokemon Database
        sql = "INSERT INTO Pokemon_cards VALUES (4, 'Squirtle', 'Water', 'Uncommon', 30, 18.99, 5);";
        rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else {
            fprintf(stdout, "\tA new card (Squirtle) was added successfully.\n");
        }
    }
    else if (result == "POKEMON_CARD_PRESENT") {
        cout << "\tSquirtle already exists in the Pokemon_cards table.\n";
    }
    else {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        cout << "Error returned Result = " << result << endl;
    }
// Check that Jigglypuff exists. No? Create entry
    sql = "SELECT IIF(EXISTS(SELECT 1 FROM Pokemon_cards WHERE  Pokemon_cards.POKEMON_CARD_name='Jigglypuff'), 'POKEMON_CARD_PRESENT', 'POKEMON_CARD_NOT_PRESENT') result;";
    rc = sqlite3_exec(db, sql, callback, ptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else if (result == "POKEMON_CARD_NOT_PRESENT") {
        fprintf(stdout, "Jigglypuff is not present. Attempting to add the card.\n");
// Add Jigglypuff to Pokemon Database
        sql = "INSERT INTO Pokemon_cards VALUES (5, 'Jigglypuff', 'Normal', 'Common', 3, 24.99, 6);";
        rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else {
            fprintf(stdout, "\tA new card (Jigglypuff) was added successfully.\n");
        }
    }
    else if (result == "POKEMON_CARD_PRESENT") {
        cout << "\tJigglypuff already exists in the Pokemon_cards table.\n";
    }
    else {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        cout << "Error returned Result = " << result << endl;
    }


#pragma endregion

// Setup passive open & Initialize the socket
    nSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (nSocket < 0) {
        cout << "Socket not Opened\n";
        sqlite3_close(db);
        cout << "Closed DB" << endl;
        exit(EXIT_FAILURE);
    }
    else {
        cout << "Socket Opened: " << nSocket << endl;
    }
// Set Socket Options
    int nOptVal = 1;
    int nOptLen = sizeof(nOptVal);
    nRet = setsockopt(nSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&nOptVal, nOptLen);
    if (!nRet) {
        cout << "The setsockopt call successful\n";
    }
    else {
        cout << "Failed setsockopt call\n";
        sqlite3_close(db);
        cout << "Closed Database" << endl;
        close(nSocket);
        cout << "Closed socket: " << nSocket << endl;
        exit(EXIT_FAILURE);
    }
// Build address data structure
    srv.sin_family = AF_INET;
    srv.sin_port = htons(SERVER_PORT);
    srv.sin_addr.s_addr = INADDR_ANY;
    memset(&(srv.sin_zero), 0, 8);
//Bind the socket to the local port
    nRet = (bind(nSocket, (struct sockaddr*)&srv, sizeof(srv)));
    if (nRet < 0) {
        cout << "Failed to bind to local port\n";
        sqlite3_close(db);
        cout << "Closed Database " << endl;
        close(nSocket);
        cout << "Closed socket: " << nSocket << endl;
        exit(EXIT_FAILURE);
    }
    else {
        cout << "Successfully bound to local port\n";
    }
// Listen for Client Requests
    nRet = listen(nSocket, MAX_PENDING);
    if (nRet < 0) {
        cout << "Failed to start listen to local port\n";
        sqlite3_close(db);
        cout << "Closed Database " << endl;
        close(nSocket);
        cout << "Closed socket: " << nSocket << endl;
        exit(EXIT_FAILURE);
    }
    else {
        cout << "Started listening to local port\n";
    }
    struct timeval tv;
    cout << "\nWaiting for connections ...\n";
    while (1)
    {
        tv.tv_sec = 1;
        tv.tv_usec = 0;
// Set the FD_SET
        FD_ZERO(&fr);
        FD_SET(nSocket, &fr);
        nMaxFd = nSocket;
        for (int nIndex = 0; nIndex < 5; nIndex++)
        {
            if (nClient[nIndex] > 0)
            {
                FD_SET(nClient[nIndex], &fr);
            }
            if (nClient[nIndex] > nMaxFd)
                nMaxFd = nClient[nIndex];
        }
        nRet = select(nMaxFd + 1, &fr, NULL, NULL, &tv);
// Reset Bits
        if (nRet < 0)
        {
            cout << endl << "select api call failed. Will exit";
            return (EXIT_FAILURE);
        }
        else
        {
// There is some client waiting either to connect or some new data came from existing client.
            if (FD_ISSET(nSocket, &fr))
            {
// Handle New connection
                NewClientConnection();
            }
            else
            {
// Check what existing client got the new data
                DataFromClient();
            }
        }
    }



    for (int l = 0; l < 10; l++) {
        close(nClient[l]);
    }


    sqlite3_close(db);
    cout << "Closed DB" << endl;
    close(nSocket);
    cout << "Closed socket: " << nSocket << endl;
    exit(EXIT_SUCCESS);
}

// Parses command from buffer sent from client 
string buildCommand(char line[]) {
    string command = "";
    size_t len = strlen(line);
    for (size_t i = 0; i < len; i++) {
        if (line[i] == '\n')
            continue;
        if (line[i] == ' ')
            break;
        command += line[i];
    }
    return command;
}



//Get information from the User and place into clientData array to be extracted in commands
string getData(char line[], string command) {
    int l = command.length();
    int spaceLocation = l + 1;
    int i = spaceLocation;
    string info = "";
//For correct output and lines
    while (line[i] != '\n') {
        if (line[i] == 0)
            return "";
        if (line[i] == ' ')
            return info;
        info += line[i];
        i++;
    }
    return info;
}

void* databaseCommands(void* userData) {
    cout << "Username: " << ((userInfo*)userData)->user << endl;;
    int clientIndex = ((userInfo*)userData)->socket;
    int clientID = nClient[((userInfo*)userData)->socket];
    nClient[clientIndex] = -1;
    cout << clientID << endl;
    int buf_len;
    string u = ((userInfo*)userData)->user;
    int idINT = ((userInfo*)userData)->id;
    string id = to_string(idINT);
    string command;
    bool rootUsr;
    if (idINT == 1) {
        rootUsr = true;
    }
    else {
        rootUsr = false;}
    while (1){
        char Buff[256] = { 0, };
        while ((buf_len = (recv(clientID, Buff, sizeof(Buff), 0)))) {
// Print out received message
            cout << "SERVER: Received: " << Buff << endl;
// Parse message for initial command
            command = buildCommand(Buff);
            if (command == "LOGIN") {
                send(clientID, "You are already logged in!", 27, 0);}
            else if (command == "BUY") {
// ERROR Handling: Client Command Input 
                if (!getData(Buff, clientInfo, command)) {
                    send(clientID, "400 Invalid Command: Missing information\n EX. Command: BUY POKEMON_CARD_name #_to_buy price userID", sizeof(Buff), 0);
                    cout << "extraction Error" << endl;}
                else {
// Check that selected user exists in Users table 
                    string sql = "SELECT IIF(EXISTS(SELECT 1 FROM Users WHERE Users.ID=" + (string)id + "), 'PRESENT', 'NOT_PRESENT') result;";
                    rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                    cout << "RC is equal to: " << rc << endl;
// ERROR Handling: Check SQL Execution
                    if (rc != SQLITE_OK) {
                        fprintf(stderr, "SQL error: %s\n", zErrMsg);
                        sqlite3_free(zErrMsg);}
                    else if (result == "PRESENT") {
// USER EXISTS
                        fprintf(stdout, "User Exists in Users Table.\n");
// Calculate Card price
                        double cardPrice = stod(clientInfo[1]) * stod(clientInfo[2]);
                        cout << "Card Price: " << cardPrice << endl;
// Get the usd balance of the user
                        sql = "SELECT usd_balance FROM Users WHERE Users.ID=" + (string)id;
                        rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                        string usd_balance = result;
                        cout << "Current User Balance: " << usd_balance << endl;
// ERROR Handling: Check SQL Execution
                        if (rc != SQLITE_OK) {
                            fprintf(stderr, "SQL error: %s\n", zErrMsg);
                            sqlite3_free(zErrMsg);}
                        else if (stod(usd_balance) >= cardPrice) {
                            double difference = stod(usd_balance) - cardPrice;
                            string sql = "UPDATE Users SET usd_balance=" + to_string(difference) + " WHERE ID =" + id + ";";
                            rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
                            cout << "User Balance Updated: " << difference << endl;
// ERROR Handling: Check SQL Execution
                            if (rc != SQLITE_OK) {
                                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                sqlite3_free(zErrMsg);  }
// Add new record or update record to Card table, Check that record already exists in Pokemon_cards
                            sql = "SELECT IIF(EXISTS(SELECT 1 FROM Pokemon_cards WHERE Pokemon_cards.POKEMON_CARD_name='" + clientInfo[0] + "' AND Pokemon_cards.owner_id='" + id + "'), 'RECORD_PRESENT', 'RECORD_NOT_PRESENT') result;";
                            rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                            if (rc != SQLITE_OK) {
                                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                sqlite3_free(zErrMsg);}
                            else if (result == "RECORD_PRESENT") {
// A record exists, so update the record
                                sql = "UPDATE Pokemon_cards SET POKEMON_CARD_price= POKEMON_CARD_price +" + clientInfo[1] + " WHERE Pokemon_cards.POKEMON_CARD_name='" + clientInfo[0] + "' AND Pokemon_cards.ID='" + id + "' AND Pokemon_cards.owner_id='" + id + "';";
                                rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, &zErrMsg);
                                cout << "Added " << clientInfo[1] << " Card to " << clientInfo[0] << " for " << id << endl;
// ERROR Handling: Check SQL Execution
                                if (rc != SQLITE_OK) {
                                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                    sqlite3_free(zErrMsg);}}
                            else {
 //  A record does not exist, so add a record
                                sql = "INSERT INTO Pokemon_cards(card_name, card_type, rarity, card_price, owner_id) VALUES ('" + clientData[0] + "', '" + clientData[1] + "', '" + clientData[2] + "', '" + clientData[3] + "', '" + clientData[4] + "', '" + id + "');";
                                rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, &zErrMsg);
                                cout << "New record created:\n\tCard Name: " << clientData[0] << "\n\tCard Balance: " << clientData[3] << "\n\tUserID: " << id << endl;
// ERROR Handling: Check SQL Execution
                                if (rc != SQLITE_OK) {
                                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                    sqlite3_free(zErrMsg);}}
//  Get Updated usd_balance for Client
                            sql = "SELECT usd_balance FROM Users WHERE Users.ID=" + id;
                            rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                            usd_balance = result;
// ERROR Handling: Check SQL Execution
                            if (rc != SQLITE_OK) {
                                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                sqlite3_free(zErrMsg);    }

// Get the new POKEMON_CARD_price
                            sql = "SELECT POKEMON_CARD_price FROM Pokemon_cards WHERE Pokemon_cards.POKEMON_CARD_name='" + clientInfo[0] + "' AND Pokemon_cards.owner_id='" + id + "';";
                            rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
// ERROR Handling: Check SQL Execution
                            if (rc != SQLITE_OK) {
                                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                sqlite3_free(zErrMsg);    }
                            string POKEMON_CARD_price = result;
// The command completed successfully, return 200 OK, the new usd_balance and new POKEMON_CARD_price
                            string tempStr = "200 OK\n   BOUGHT: New balance: " + POKEMON_CARD_price + " " + clientInfo[0] + ". USD balance $" + usd_balance;
                            send(clientID, tempStr.c_str(), sizeof(buf), 0);}
                        else {
                            cout << "SERVER: Not enough balance. Purchase Aborted." << endl;
                            send(clientID, "400 Invalid Command: not enough USD", sizeof(Buff), 0);}}
                    else {
// USER DOES NOT EXIST
                        fprintf(stdout, "SERVER: User Does Not Exist in Users Table. Aborting Buy\n");
                        string tempStr = "400 Invalid Command: user " + id + " does not exist";
                        send(clientID, tempStr.c_str(), sizeof(Buff), 0);}
                }

                cout << "SERVER: Successfully executed BUY command\n\n";}
            else if (command == "SELL") {
// ERROR Handling: Client Command Input 
                if (!getData(Buff, clientInfo, command)) {
                    cout << "Invalid Command: Missing information" << endl;
                    send(clientID, "400 Invalid Command: Missing information\n EX. Command: SELL card_name card_type rarity quantity card_price userID", sizeof(Buff), 0);
                }
                                else {
// Check that the selected user exists in Users table 
                    string sql = "SELECT IIF(EXISTS(SELECT 1 FROM Users WHERE Users.ID=" + id + "), 'PRESENT', 'NOT_PRESENT') result;";
                    rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);

                    if (rc != SQLITE_OK) {
                        fprintf(stderr, "SQL error: %s\n", zErrMsg);
                        sqlite3_free(zErrMsg);

                    }
                    else if (result == "PRESENT") {
 //  Check if Client owns specific card
                        sql = "SELECT IIF(EXISTS(SELECT 1 FROM Pokemon_cards WHERE Pokemon_cards.POKEMON_CARD_name='" + clientInfo[0] + "' AND Pokemon_cards.owner_id='" + id + "'), 'RECORD_PRESENT', 'RECORD_NOT_PRESENT') result;";
                        rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);

                        if (rc != SQLITE_OK) {
                            fprintf(stderr, "SQL error: %s\n", zErrMsg);
                            sqlite3_free(zErrMsg);}
                        else if (result == "RECORD_NOT_PRESENT") {
                            cout << "SERVER: Invalid Card Selection. Aborting Sell\n";
                            send(clientID, "400 Invalid Command: Invalid Card Selection, This User does not Own that Pokemon Card.", sizeof(Buff), 0);}
                        else {
// Check that the user has enough of the selected card to sell

// int numCardsToSell = stod(clientInfo[1]);

//trying 2 instead
                            int numCardsToSell = stod(clientInfo[2]);

// Checking to see if it is the correct amount here. NOT THE RIGHT AMOUNT!
                            cout << numCardsToSell << endl;
// Get the number of cards the user owns of the selected card
                            sql = "SELECT POKEMON_CARD_price FROM Pokemon_cards WHERE Pokemon_cards.POKEMON_CARD_name='" + clientInfo[0] + "' AND Pokemon_cards.owner_id='" + id + "';";
                            rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                            if (rc != SQLITE_OK) {
                                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                sqlite3_free(zErrMsg);}
                            double POKEMON_CARD_price = stod(result);
// Not enough cards in balance to sell
                            if (POKEMON_CARD_price < numCardsToSell) {
                                cout << "SERVER: Invalid Quantity. User does not own that many cards. Aborting sell.\n";
                                send(clientID, "400 Invalid Command: User does not have that quantity.", sizeof(Buff), 0);}
                            else {
// Get dollar amount to sell
                                double cardPrice = stod(clientInfo[4]) * stod(clientInfo[2]);
// Add amount to user's balance
                                sql = "UPDATE Users SET usd_balance= usd_balance +" + to_string(cardPrice) + " WHERE Users.ID='" + id + "';";
                                rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, &zErrMsg);
                                if (rc != SQLITE_OK) {
                                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                    sqlite3_free(zErrMsg);}
// Remove sold cards from Pokemon_cards
                                sql = "UPDATE Pokemon_cards SET quantity= quantity -" + to_string(numCardsToSell) + " WHERE Pokemon_cards.POKEMON_CARD_name='" + clientInfo[0] + "' AND Pokemon_cards.owner_id='" + id + "';";
                                rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, &zErrMsg);
                                if (rc != SQLITE_OK) {
                                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                    sqlite3_free(zErrMsg);}

                     else {
 //  A record does not exist, so add a record
                                sql = "INSERT INTO Pokemon_cards(card_name, card_type, rarity, quantity, card_price, owner_id) VALUES ('" + clientData[0] + "', '" + clientData[1] + "',  '" + clientData[2] + "',  '" + clientData[3] + "',  '" + clientData[4] + "', '" + id + "');";
                                rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, &zErrMsg);
                                cout << "New record created:\n\tCard Name: " << clientData[0] << "\n\tCard Balance: " << clientData[3] << "\n\tUserID: " << id << endl;
// Check if SQL executed correctly
                                if (rc != SQLITE_OK) {
                                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                    sqlite3_free(zErrMsg);
                                }
                            }                                    
// Get Client’s new balance
                                sql = "SELECT usd_balance FROM Users WHERE Users.ID=" + id;
                                rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                                string usd_balance = result;

// Get new SOLD

 
   // sql = "SELECT POKEMON_CARD_price FROM Pokemon_cards WHERE Pokemon_cards.POKEMON_CARD_name='" + clientInfo[0] + "' AND Pokemon_cards.owner_id='" + id + "';";



                               sql = "SELECT quantity FROM Pokemon_cards WHERE Pokemon_cards.POKEMON_CARD_name='" + clientInfo[0] + "' AND Pokemon_cards.owner_id='" + id + "';";
//Error handling to make sure info was cofrect
//cout << "array 0: " << clientInfo[0] << endl;
//cout <<  "array 1: " <<  clientInfo[1] << endl;
//cout <<  "array 2: " <<  clientInfo[2] << endl;

                                rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                                string quantity = result;

// Sell command completed successfully, changed here, numbercards sold
// POKEMON_CARD_price clientInfo[0]

// string tempStr = "200 OK\n   SOLD: New balance: " + quantity + " " + clientInfo[2] + ". USD $" + usd_balance;

                                string tempStr = "200 OK\n   SOLD: New balance: " + quantity + ". USD $" + usd_balance;




                                send(clientID, tempStr.c_str(), sizeof(Buff), 0);
    }
}
                    }
                    else {
                        fprintf(stdout, "SERVER: User Does Not Exist  in Users Table. Aborting Sell.\n");
                        send(clientID, "400 Invalid Command: user does not exist.", sizeof(Buff), 0);
                    }
                }
                cout << "Successfully executed SELL command for User\n\n";

            }
            else if (command == "LIST") {
                if (idINT == 1) {
                    result = "";
// List all records in Pokemon_cards table for owner_id = 1
                    string sql = "SELECT * FROM Pokemon_cards";

// SQL Execute
                    rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);

                    if (rc != SQLITE_OK) {
                        fprintf(stderr, "SQL error: %s\n", zErrMsg);
                        sqlite3_free(zErrMsg);
                    }

                    string sendStr;

                    if (result == "") {
                        sendStr = "200 OK\n   No records in the Pokemon Cards Database.";
                    }
                    else {
                        sendStr = "200 OK\n   The list of records in the Card database:\nCardID  POKEMON_CARD_name POKEMON_CARD_Amount  POKEMON_CARD_Type  POKEMON_CARD_Rarity  UserID\n   " + result;
                    }
                    send(clientID, sendStr.c_str(), sizeof(Buff), 0);
                }
                else {
                    result = "";
// List all records in Pokemon_cards table for owner_id = 1
                    string sql = "SELECT * FROM Pokemon_cards WHERE Pokemon_cards.owner_id=" + id;

// SQL Execute
                    rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);

                    if (rc != SQLITE_OK) {
                        fprintf(stderr, "SQL error: %s\n", zErrMsg);
                        sqlite3_free(zErrMsg);
                    }

                    string sendStr;

                    if (result == "") {
                        sendStr = "200 OK\n   No records in the Card Database.";
                    }
                    else {
// changed here LIST added output
                        sendStr = "200 OK\n   The list of records in the Card database:\nCardID  Card Name Card Type  Rarity Quantity Card Price UserID\n   " + result;
                    }
                    send(clientID, sendStr.c_str(), sizeof(Buff), 0);
                }
            }
            else if (command == "BALANCE") {
// cout << "Received: BALANCE" << endl;
                string sql = "SELECT IIF(EXISTS(SELECT 1 FROM Users WHERE Users.ID=" + id + "), 'PRESENT', 'NOT_PRESENT') result;";
// SQL Execute
                rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                if (rc != SQLITE_OK) {
                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                    sqlite3_free(zErrMsg);
                }
                else if (result == "PRESENT") {
//Balance for user with that ID
                    sql = "SELECT usd_balance FROM Users WHERE Users.ID=" + id;
                    rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                    string usd_balance = result;
//Client Name
                    sql = "SELECT first_name FROM Users WHERE Users.ID=" + id;
                    rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                    string user_name = result;
                    sql = "SELECT last_name FROM Users WHERE Users.ID=" + id;
                    rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                    user_name += " " + result;
                    string tempStr = "200 OK\n   Balance for user " + user_name + ": $" + usd_balance;
                    send(clientID, tempStr.c_str(), sizeof(Buff), 0);
                }
                else {
                    cout << "SERVER: User does not exist. Aborting Balance.\n";
                    send(clientID, "User does not exist.", sizeof(Buff), 0);
                }
            }
            else if (command == "QUIT") {
                send(clientID, "200 OK", 27, 0);
                for (int i = 0; i < list.size(); i++) {
                    if (list.at(i).user == u)
                        list.erase(list.begin() + i);
                }
                nClient[clientIndex] = 0;
                close(clientID);
                pthread_exit(userData);
                return userData;
            }
            else if (command == "SHUTDOWN" && rootUsr) {
                send(clientID, "200 OK", 7, 0);
                sqlite3_close(db);
                cout << "Closed Database" << endl;
                close(clientID);
                cout << "Client Terminated Connection:  " << clientID << endl;
                for (int i = 0; i < list.size(); i++) {
                    if (list.at(i).user == u)
                        list.erase(list.begin() + i);
                }
                for (int i = 0; i < list.size(); i++) {
                    close(nClient[list.at(i).socket]);
                    pthread_cancel((list.at(i)).myThread);
                }
                close(nSocket);
                cout << "Socket Closed: " << nSocket << endl;
                pthread_exit(userData);
                exit(EXIT_SUCCESS);
            }
            else if (command == "LOGOUT") {
                send(clientID, "200 OK", 7, 0);
                for (int i = 0; i < list.size(); i++) {
                    if (list.at(i).user == u)
                        list.erase(list.begin() + i);
                }
                nClient[clientIndex] = clientID;
                pthread_exit(userData);
                return userData;
            }
            else if (command == "DEPOSIT") {
                string sql = "SELECT IIF(EXISTS(SELECT 1 FROM Users WHERE Users.ID=" + id + "), 'PRESENT', 'NOT_PRESENT') result;";
// SQL Execute
                rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                if (rc != SQLITE_OK) {
                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                    sqlite3_free(zErrMsg);
                }
                else if (result == "PRESENT") {
// Output Balance For This User
                    string deposit = "";
                    for (int i = (command.length() + 1); i < strlen(Buff); i++) {
                        if (Buff[i] == '\n')
                            break;
                        deposit += Buff[i];
                    }
                    sql = "UPDATE Users SET usd_balance= usd_balance +" + deposit + " WHERE Users.ID='" + id + "';";
                    rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                    sql = "SELECT usd_balance FROM Users WHERE Users.ID=" + id;
                    rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                    string usd_balance = result;
// Get User Name
                    sql = "SELECT first_name FROM Users WHERE Users.ID=" + id;
                    rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                    string user_name = result;
                    sql = "SELECT last_name FROM Users WHERE Users.ID=" + id;
                    rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                    user_name += " " + result;
                    string tempStr = "200 OK\n   New balance for user " + user_name + ": $" + usd_balance;
                    send(clientID, tempStr.c_str(), sizeof(Buff), 0);
                }
                else {
                    cout << "SERVER> User does not exist. Aborting Balance.\n";
                    send(clientID, "User does not exist.", sizeof(Buff), 0);
                }
            }
            else if (command == "WHO" && rootUsr) {
                string result = "200 OK\nThe list of the active Users:\n";
                for (int i = 0; i < list.size(); i++) {
                    result += (list.at(i).user + " " + list.at(i).ip + "\n");
                }
                send(clientID, result.c_str(), sizeof(Buff), 0);
            }
//CHECK to determine if client is in database when have multiple clients
            else if (command == "CHECK") {
                string searchTerm = "";
                string sendStr;
                result = "";
                for (int i = (command.length() + 1); i < strlen(Buff); i++) {
                    if (Buff[i] == '\n')
                        break;
                    searchTerm += Buff[i];
                }
                string sql = "SELECT COUNT(POKEMON_CARD_name) FROM (SELECT * FROM Pokemon_cards WHERE owner_id = " + id + ") WHERE POKEMON_CARD_name LIKE '%" + searchTerm + "%';";
                rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                string count = result;
                result = "";
                sql = "SELECT POKEMON_CARD_name, POKEMON_CARD_price FROM (SELECT * FROM Pokemon_cards WHERE owner_id = " + id + ") WHERE POKEMON_CARD_name LIKE '%" + searchTerm + "%'";
                rc = sqlite3_exec(db, sql.c_str(), callback, ptr, &zErrMsg);
                if (result == "") {
                    sendStr = "404 Your search did not match any records";
                }
                else {
                    sendStr = "200 OK\n   Found:" + count + "\nPOKEMON_CARD_name POKEMON_CARD_Amount\n   " + result;
                }
                send(clientID, sendStr.c_str(), sizeof(Buff), 0);

            }
// All Else Fails, Invalid Command
            else {
                cout << "SERVER: INVALID COMMAND" << endl;
                send(clientID, "400 Invalid Command", 20, 0);
            }
        }

        for (int i = 0; i < list.size(); i++) {
            if (list.at(i).user == u)
                list.erase(list.begin() + i);
        }

        cout << endl << "Error at client socket\n";
        nClient[clientIndex] = 0;
        close(clientID);
        pthread_exit(userData);

        return userData;
    }
}

bool getData(char line[], string info[], string command) {
    int l = command.length();
    int spaceLocation = l + 1;

    for (int i = 0; i < 3; i++) {
        info[i] = "";

// Parses the information
        for (int j = spaceLocation; j < strlen(line); j++) {

            if (line[j] == 0)
                return false;
            if (line[j] == ' ')
                break;
            if (line[j] == '\n')
                break;
            info[i] += line[j];

//Valid Client Data input for commands
            if (i > 0) {
                if (((int)line[j] > 57 || (int)line[j] < 46) && (int)line[j] != 47)
                    return false;
            }
        }
        if (info[i] == "") {
            fill_n(info, 3, 0);
            return false;
        }

        spaceLocation += info[i].length() + 1;

    }

    return true;

}

//Function to deal with message format to screen to client
static int callback(void* ptr, int count, char** data, char** azColName) {

    if (count == 1) {
        result = data[0];
    }
    else if (count > 1) {
        for (int i = 0; i < count; i++) {

            if (result == "") {
                result = data[i];
            }
            else {
                result = result + " " + data[i];
            }
            if (i == 3)
            {
                result += "\n  ";
            }

        }
    }
    return 0;
}

void NewClientConnection(){
//Descriptor file per CIS 427 Lecture
    int nNewClient = accept(nSocket, (struct sockaddr*)&srv, &addr_len);

    //Issue with Connection since client tried to connect without IP
    if (nNewClient < 0) {
        perror("Error during accepting connection");
    }
    else {

        void* temp = &nNewClient;
        cout << nNewClient << endl;

        int nIndex;
        for (nIndex = 0; nIndex < 5; nIndex++)
        {
            if (nClient[nIndex] == 0)
            {
                nClient[nIndex] = nNewClient;
                if (nNewClient > nMaxFd)
                {
                    nMaxFd = nNewClient + 1;
                }
                break;
            }
        }

        if (nIndex == 5)
        {
            cout << endl << "Server busy. Cannot accept anymore connections";
        }

        printf("New connection, socket fd is %d, ip is: %s, port: %d\n", nNewClient, inet_ntoa(srv.sin_addr), ntohs(srv.sin_port));
        send(nClient[nIndex], "You have successfully connected to the server!", 47, 0);
    }

}

void DataFromClient()
{
    string command;
    temp = &u;

    for (int nIndex = 0; nIndex < 5; nIndex++)
    {
        if (nClient[nIndex] > 0)
        {
            if (FD_ISSET(nClient[nIndex], &fr))
            {
// Read the data from client
                char sBuff[256] = { 0, };
                int nRet = recv(nClient[nIndex], sBuff, 256, 0);
                if (nRet < 0)
                {
// This happens when client closes connection abruptly
                    cout << endl << "Error at client socket";
                    for (int i = 0; i < list.size(); i++) {
                        if (list.at(i).user == u.user)
                            list.erase(list.begin() + i);
                    }
                    close(nClient[nIndex]);
                    nClient[nIndex] = 0;
                }
                else
                {

                    command = buildCommand(sBuff);
                    cout << command << endl;

                    if (command == "LOGIN") {
                        string info = getData(sBuff, command);
                        loggedUser tempStruct;
                        u.user = info;
                        int passLength = command.length() + info.length();
                        string passInfo = clientPassword(sBuff, passLength);

                        u.password = passInfo;
                        u.socket = nIndex;
                        tempStruct.socket = nIndex;
                        struct sockaddr_in client_addr;
                        socklen_t addrlen;

                        cout << "Assigned user info. Username: " << info << " Socket Index: " << u.socket << endl;

                        string commandSql = "SELECT IIF(EXISTS(SELECT * FROM Users WHERE user_name = '" + info + "' AND password = '" + passInfo + "') , 'USER_PRESENT', 'USER_NOT_PRESENT') result;";
                        sql = commandSql.c_str();
                        sqlite3_exec(db, sql, callback, 0, &zErrMsg);

                        if (result == "USER_PRESENT") {
                            cout << "Logging in... " << endl;
                            send(nClient[nIndex], "200 OK", 7, 0);
                            getpeername(nClient[nIndex], (struct sockaddr*)&client_addr, &addrlen);
                            tempStruct.ip = "";
                            cout << "IP address: " << inet_ntoa(client_addr.sin_addr) << endl;
                            tempStruct.ip = inet_ntoa(client_addr.sin_addr);
                            cout << tempStruct.ip << endl;
                            tempStruct.user = u.user;

                            list.push_back(tempStruct);

                            commandSql = "SELECT ID FROM Users WHERE user_name = '" + info + "' AND password = '" + passInfo + "'";
                            sql = commandSql.c_str();
                            sqlite3_exec(db, sql, callback, 0, &zErrMsg);
                            u.id = stoi(result);

                            pthread_create(&(list.at(list.size() - 1).myThread), NULL, databaseCommands, temp);

}
                        else {
                            cout << "Username or Password Invalid!" << endl;
}
                    }
                    else if (command == "QUIT") {
                        cout << "Quit command!" << endl;
                        send(nClient[nIndex], "200 OK", 27, 0);
                        close(nClient[nIndex]);
                        nClient[nIndex] = 0;
                    }
                    else if (command == "BUY" || command == "SELL" || command == "CHECK" || command == "DEPOSIT" || command == "SHUTDOWN" || command == "LIST" || command == "WHO" || command == "BALANCE" || command == "LOGOUT" || command == "NULL") {
                        send(nClient[nIndex], "Guest Users can only use the login and quit commands.", 54, 0);
                    }
                    else {
                        cout << endl << "Received data from:" << nClient[nIndex] << "[Message: " << sBuff << " size of array: " << strlen(sBuff) << "] Error 400" << endl;
                        send(nClient[nIndex], "Command does not exist.", 24, 0);
                    }
                    break;
                }
            }
        }
    }
}

