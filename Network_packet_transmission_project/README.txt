a. Name: Qiaochu Zhang
b. Student ID: 9111547240
c. I don't complete the optional part.
d. Files include:
serverM.c : main server implementation, connected to the member.txt
serverS.c : single room server connected to the single.txt
serverD.c : double room server connected to the double.txt
serverU.c : suite room server connected to the suite.txt
client.c : client server for client to input request
Makefile : makefile to compile
e. username and password are concatenated and delimited by a comma and one space during transmission, named as "encrypted_credentials". 
The room code and action(also named as requestType, reservation or availability) are delimited by one space during transmission.
f. Need to put single.txt, double.txt, suite.txt, member.txt under the same directory.
g. I mainly referred to Beej’s Guide. Code is written by myself.
