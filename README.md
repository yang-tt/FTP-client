# FTP-client

Goal: Complete a FTP client program based on Linux command line terminal.

1. The well-known port number (TCP Port 21) of FTP server is used and there is no need to
input it. The ftp server can be identified by IP address, such as:

./ftpcli 10.3.255.85

./ftpcli 127.0.0.1

2. FTP client connects FTP server using TCP protocol. It can receive the FTP replies coming from server and translate it into natural language, and change the user’s commands into FTP command before sending.

3. Support user’s authentication by username & password, with hidden password function.

4. Support user’s interactive operation and provide the following commands: 
list (ls), directory operation (pwd, cd, mkdir), upload a file (put), download a file (get), delete a file (delete), renaming a file (rename), transfer mode (ascii, binary), and exit (quit).

5. Be able to handle errors: invalid commands, missing parameters, requested file already existed.

6. Detailed designing document and user manual.

7. Detailed annotation of code and nice programming style.

8. Stable and friendly to users.

9. Two persons as a group.
