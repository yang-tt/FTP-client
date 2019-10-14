#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <netdb.h>    /* getservbyname(), gethostbyname() */
#include    <errno.h>    /* for definition of errno */
#include    <netinet/tcp.h>
#include    <arpa/inet.h>
#include    <termios.h>
#include    <unistd.h>
#include    <curses.h>
#include    <fcntl.h>
#include    <termios.h>
#include    <sys/types.h>


/* define macros*/
#define MAX 100
#define MAXBUF    1024
#define MAXBUF_1    1024
#define STDIN_FILENO    0
#define STDOUT_FILENO    1

/* define FTP reply code */
#define USERNAME    220
#define PASSWORD    331
#define LOGIN        230
#define PATHNAME    257
#define CLOSEDATA    226
#define ACTIONOK    250
#define USERPASS    530


/* Define global variables */
char *host;        /* hostname or dotted-decimal string */
char *port;
char *rbuf, *rbuf1;        /* pointer that is malloc'ed */
char *wbuf, *wbuf1;        /* pointer that is malloc'ed */
struct sockaddr_in servaddr;

int cliopen(char *host, char *port);

void strtosrv(char *str, char *host, char *port);

void cmd_tcp(int sockfd);

void ftp_list(int sockfd);

int ftp_get(int sck, char *pDownloadFileName_s);

int ftp_put(int sck, char *pUploadFileName_sn ,int asciiFlag);

void getpasswd(char *passwd);

int mygetch();

void strrpc(char *str, char *oldstr, char *newstr);

void help();

char *subString(char *str, int star, int len);

void strcut(char *str, int n);

int rest(int, char *);

int main(int argc, char *argv[]) {
	int fd;

	if (0 != argc - 2) {
		printf("%s\n", "missing <hostname>");
		exit(0);
	}

	host = argv[1];
	port = "21";
	/*****************************************************************
	//1. code here: Allocate the read and write buffers before open().
	*****************************************************************/
	rbuf = (char *) malloc(sizeof(char) * MAXBUF_1);
	memset(rbuf, '\0', MAXBUF_1);
	wbuf = (char *) malloc(sizeof(char) * MAXBUF_1);
	memset(wbuf, '\0', MAXBUF_1);
	rbuf1 = (char *) malloc(sizeof(char) * MAXBUF_1);
	memset(rbuf1, '\0', MAXBUF_1);
	wbuf1 = (char *) malloc(sizeof(char) * MAXBUF_1);
	memset(wbuf1, '\0', MAXBUF_1);

	fd = cliopen(host, port);

	cmd_tcp(fd);

	exit(0);
}


/* Establish a TCP connection from client to server */
int cliopen(char *host, char *port) {
	/*************************************************************
	//2. code here :fd = socket(), connect(), return fd
	*************************************************************/
	int sockfd;
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		printf("[TCP] socket() failed. \n");
		exit(1);
	}

	struct sockaddr_in echoServAddr; /* Local address */

	char **addr_list = gethostbyname(host)->h_addr_list;

	struct in_addr *ip = (struct in_addr *) (addr_list[0]);

	char *serverIP = inet_ntoa(*ip);
	unsigned short echoServPort = atoi(port);
	memset(&echoServAddr, 0, sizeof(echoServAddr));
	echoServAddr.sin_family = AF_INET;
	echoServAddr.sin_addr.s_addr = inet_addr(serverIP);
	echoServAddr.sin_port = htons(echoServPort);

	if (connect(sockfd, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0) {
		printf("[TCP] connect() failed. \n");
		exit(1);
	}
	printf("[TCP] Connected to the server: %s\n", serverIP);

	return sockfd;
}


/*
   Compute server's port by a pair of integers and store it in char *port
   Get server's IP address and store it in char *host
*/
void strtosrv(char *str, char *host, char *port) {
	/*************************************************************
	//3. code here
	*************************************************************/
	//printf("str:%s\n",str);
	int i = 0;
	char temp[MAX][MAX], temp2[MAX][MAX];
	char hostport[MAX];
	char *x, *y, *tempHost;
	char *str_in;

	str_in = (char *) malloc(sizeof(char) * strlen(str));
	memset(str_in, '\0', strlen(str));
	strcpy(str_in, str);

	/**********Split" "************/
	x = strtok(str_in, " ");
	for (i; x != NULL; i++) {
		strcpy(temp[i], x);
		x = strtok(NULL, " ");
	}

	/**********Remove"()"************/
	temp[i - 1][strlen(temp[i - 1]) - 1] = '\0';
	strcpy(hostport, temp[i - 1] + 1);
	//printf("hostport: %s\n",hostport);

	/**********Split","************/
	y = strtok(hostport, ",");
	for (i = 0; y != NULL; i++) {
		strcpy(temp2[i], y);
		y = strtok(NULL, ",");
	}

	/**********host************/
	tempHost = temp2[0];
	for (i = 1; i < 4; i++) {
		strcat(tempHost, ".");
		strcat(tempHost, temp2[i]);
	}
	strcpy(host, tempHost);
	//printf("HOST: %s\n",host);

	/**********port************/
	int porta = atoi(temp2[4]);
	int portb = atoi(temp2[5]);
	//printf("porta: %d; portb: %d\n",porta,portb);
	int p = porta * 256 + portb;
	sprintf(port, "%d", p);
	//printf("PORT:%s\n",port);
}


/* Read and write as command connection */
void cmd_tcp(int sockfd) {
	int maxfdp1, nread, nwrite, fd, replycode, tread, cmdLen;
	char *host;
	char *port;
	char *x;
	char dir[MAX][MAX];
	int i = 0;
	char *notice;
	fd_set rset;
	int asciiFlag = 1; //1.ascii; 0.binary

	host = (char *) malloc(sizeof(char) * MAX);
	memset(host, '\0', MAX);
	port = (char *) malloc(sizeof(char) * MAX);
	memset(port, '\0', MAX);
	notice = (char *) malloc(sizeof(char) * MAX);
	memset(notice, '\0', MAX);

	FD_ZERO(&rset);
	maxfdp1 = sockfd + 1;    /* check descriptors [0..sockfd] */

	for (;;) {

//		printf("FLAG: %d\n", asciiFlag);


		FD_ZERO(&rset);
		FD_SET(STDIN_FILENO, &rset);//FD_SET(s,*set)：向集合添加描述字s
		FD_SET(sockfd, &rset);

		rbuf = (char *) malloc(sizeof(char) * MAXBUF_1);
		memset(rbuf, '\0', MAXBUF_1);
		wbuf = (char *) malloc(sizeof(char) * MAXBUF_1);
		memset(wbuf, '\0', MAXBUF_1);


		if (select(maxfdp1, &rset, NULL, NULL, NULL) < 0)
			printf("select error\n");

		/* data to read on stdin */
		if (FD_ISSET(STDIN_FILENO, &rset)) {
			// printf("!!in FD_ISSET(STDIN_FILENO\n");

			if ((nread = read(STDIN_FILENO, rbuf, MAXBUF)) < 0)//nread：真实读取的字节数
				printf("read error from stdin\n");
			else {
				rbuf = strcat(rbuf, "\n\n");
			}

			/* send username */
			if (replycode == USERNAME) {
				nwrite = nread + 5;
				sprintf(wbuf, "USER %s", rbuf);//int sprintf( char *buffer, const char *format, [ argument] … );
				if (write(sockfd, wbuf, nwrite) != nwrite)//用nwrite控制位数
					printf("write error\n");
				//else printf("!!write successfully; wbuf:%s\n----END----\n\n",wbuf);
				continue;
			}


			/* send command */
			if (replycode == LOGIN || replycode == CLOSEDATA || replycode == PATHNAME || replycode == ACTIONOK) {
//				if (rbuf[0] == ' ') {
//					notice = "Please check your input.\nftp>";
//					continue;
//				}

				int socketfd;
				cmdLen = strlen(rbuf) - 3;
//				printf("=====>Len: %d\n", cmdLen);
//				printf("=====>CMD: %s\n", rbuf);
				/* ls - list files and directories*/
				if (strncmp(rbuf, "ls", 2) == 0) {
					if (cmdLen != 2) {
						if (write(STDOUT_FILENO, "?Invalid command\nftp>", 21) != 21)
							printf("write error to stdout\n");
						continue;
					}

					sprintf(wbuf, "%s", "PASV\n");
					write(sockfd, wbuf, 5);
					sprintf(wbuf1, "%s", "LIST -al\n");
					nwrite = 9;
					continue;
				}

				/* pwd -  print working directory */
				if (strncmp(rbuf, "pwd", 3) == 0) {
					if (cmdLen != 3) {
						if (write(STDOUT_FILENO, "?Invalid command\nftp>", 21) != 21)
							printf("write error to stdout\n");
						continue;
					}
					sprintf(wbuf, "%s", "PWD\n");
					write(sockfd, wbuf, 4);
					continue;
				}

				/*************************************************************
				// 5. code here: cd - change working directory/ CWD
				*************************************************************/
				/* cd - change working directory */
				if (strncmp(rbuf, "cd", 2) == 0) {

					x = strtok(rbuf, " ");
					i = 0;
					for (i; x != NULL; i++) {
						strcpy(dir[i], x);
						x = strtok(NULL, " ");
					}
					if (i != 2) {
						notice = "Please check your input.\n(usage: cd remote-directory)\nftp>";
						nread = strlen(notice);
						if (write(STDOUT_FILENO, notice, nread) != nread)
							printf("write error to stdout\n");
						continue;
					}

					sprintf(wbuf, "CWD %s", dir[i - 1]);
					nwrite = strlen(dir[i - 1]) + 4;
					if (write(sockfd, wbuf, nwrite) != nwrite)//用nwrite控制位数
						printf("write error\n");
					continue;
				}

				/*************************************************************
				// 5.1 code here: mkdir -  make new directory/ MKD
				*************************************************************/
				if (strncmp(rbuf, "mkdir", 5) == 0) {

					x = strtok(rbuf, " ");
					i = 0;
					for (i; x != NULL; i++) {
						strcpy(dir[i], x);
						x = strtok(NULL, " ");
					}
					if (i != 2) {
						notice = "Please check your input.\n(usage: mkdir remote-directory)\nftp>";
						nread = strlen(notice);
						if (write(STDOUT_FILENO, notice, nread) != nread)
							printf("write error to stdout\n");
						continue;
					}

					sprintf(wbuf, "MKD %s", dir[i - 1]);
					nwrite = strlen(dir[i - 1]) + 4;
					if (write(sockfd, wbuf, nwrite) != nwrite)//用nwrite控制位数
						printf("write error\n");
					continue;
				}

				/*************************************************************
				// 6. code here: quit - quit from ftp server
				*************************************************************/
				if (strncmp(rbuf, "quit", 4) == 0) {
					if (cmdLen != 4) {
						if (write(STDOUT_FILENO, "?Invalid command\nftp>", 21) != 21)
							printf("write error to stdout\n");
						continue;
					}
					sprintf(wbuf, "%s", "QUIT\n");
					write(sockfd, wbuf, 5);
					continue;
				}
				/*************************************************************
				// 6.1 code here: delete -  delete a file/ DELE
				*************************************************************/
				if (strncmp(rbuf, "delete", 6) == 0) {
					x = strtok(rbuf, " ");
					i = 0;
					for (i; x != NULL; i++) {
						strcpy(dir[i], x);
						x = strtok(NULL, " ");
					}
					if (i != 2) {
						notice = "Please check your input.\n(usage: delete remote-filename)\nftp>";
						nread = strlen(notice);
						if (write(STDOUT_FILENO, notice, nread) != nread)
							printf("write error to stdout\n");
						continue;
					}

					sprintf(wbuf, "DELE %s\n", dir[i - 1]);
					nwrite = strlen(dir[i - 1]) + 5;
					if (write(sockfd, wbuf, nwrite) != nwrite)//用nwrite控制位数
						printf("write error\n");
					//else printf("\nwrite successfully; wbuf:%s\n----END----\n\n",wbuf);
					continue;
				}

				/*************************************************************
				// 6.2 code here: rename -  rename the file/ RNFR & RNTO
				*************************************************************/
				if (strncmp(rbuf, "rename", 6) == 0) {
					if (cmdLen != 6) {
						if (write(STDOUT_FILENO, "?Invalid command\nftp>", 21) != 21)
							printf("write error to stdout\n");
						continue;
					}

					printf("[From name:] ");
					memset(rbuf, 0, MAXBUF);
					memset(wbuf, 0, MAXBUF);
					scanf("%s", rbuf);

					sprintf(wbuf, "RNFR %s\n", rbuf);
					nwrite = strlen(wbuf);
					write(sockfd, wbuf, nwrite);
					memset(rbuf, 0, MAXBUF);
					memset(wbuf, 0, MAXBUF);
					continue;
				}

				/*************************************************************
				// 7. code here: get - get file from ftp server
				*************************************************************/
				if (strncmp(rbuf, "get", 3) == 0) {
					x = strtok(rbuf, " ");
					i = 0;
					for (i; x != NULL; i++) {
						strcpy(dir[i], x);
						x = strtok(NULL, " ");
					}
//					printf("===>%d", i);
					if (i != 2) {
						notice = "Please check your input.\n(usage: get remote-filename)\nftp>";
						nread = strlen(notice);
						if (write(STDOUT_FILENO, notice, nread) != nread)
							printf("write error to stdout\n");
						continue;
					}
//					printf("===>[%s]", dir[1]);
					if (dir[1][0] == '\n') {
						notice = "Please check your input.\n(usage: get remote-filename)\nftp>";
						nread = strlen(notice);
						if (write(STDOUT_FILENO, notice, nread) != nread)
							printf("write error to stdout\n");
						continue;
					}

					if (strlen(dir[i - 1]) <= 0) {
						printf("please input filename\n");
						continue;
					}//dir[i-1]: file to get
					ftp_get(sockfd, dir[i - 1]);
					memset(rbuf, 0, MAXBUF);
					memset(wbuf, 0, MAXBUF);
					memset(rbuf1, 0, MAXBUF);
					continue;
				}

				/*************************************************************
				// 8. code here: put -  put file upto ftp server
				*************************************************************/
				if (strncmp(rbuf, "put", 3) == 0) {
					x = strtok(rbuf, " ");
					i = 0;
					for (i; x != NULL; i++) {
						strcpy(dir[i], x);
						x = strtok(NULL, " ");
					}
					if (i != 2) {
						notice = "Please check your input.\n(usage: put local-filename)\nftp>";
						nread = strlen(notice);
						if (write(STDOUT_FILENO, notice, nread) != nread)
							printf("write error to stdout\n");
						continue;
					}


					if (strlen(dir[i - 1]) <= 0) {
						printf("Please input filename\n");
						continue;
					}//dir[i-1]: file to get
					ftp_put(sockfd, dir[i - 1], asciiFlag);
					memset(rbuf, 0, MAXBUF);
					memset(wbuf, 0, MAXBUF);
					memset(rbuf1, 0, MAXBUF);
					continue;
				}


				/*************************************************************
				// 8.1 code here: ascii -  use ascii to represent data
				*************************************************************/
				if (strncmp(rbuf, "ascii", 5) == 0) {
					if (cmdLen != 5) {
						if (write(STDOUT_FILENO, "?Invalid command\nftp>", 21) != 21)
							printf("write error to stdout\n");
						continue;
					}
					sprintf(wbuf, "%s", "TYPE A\n");
					write(sockfd, wbuf, 7);
					continue;
				}
				/*************************************************************
				// 8.2 code here: binary -  use binary to represent data
				*************************************************************/
				if (strncmp(rbuf, "binary", 6) == 0) {
					if (cmdLen != 6) {
						if (write(STDOUT_FILENO, "?Invalid command\nftp>", 21) != 21)
							printf("write error to stdout\n");
						continue;
					}
					sprintf(wbuf, "%s", "TYPE I\n");
					write(sockfd, wbuf, 7);
					continue;
				}

				if (strncmp(rbuf, "rest", 4) == 0) {

					x = strtok(rbuf, " ");
					i = 0;
					for (i; x != NULL; i++) {
						strcpy(dir[i], x);
						x = strtok(NULL, " ");
					}
					if (i != 2) {
						notice = "Please check your input.\n(usage: rest from-point)\nftp>";
						nread = strlen(notice);
						if (write(STDOUT_FILENO, notice, nread) != nread)
							printf("write error to stdout\n");
						continue;
					}


					if (strlen(dir[i - 1]) <= 0) {
						printf("Please input from-point\n");
						continue;
					}//dir[i-1]: file to get


					rest(sockfd, dir[i - 1]);


					continue;

				}


				write(sockfd, rbuf, nread);//nread：buflength



			}


		}
			/**************************************************************/
			/* data to read from socket */
		else if (FD_ISSET(sockfd, &rset)) {
			if ((nread = recv(sockfd, rbuf, MAXBUF, 0)) < 0)
				printf("recv error\n");
			else if (nread == 0)
				break;

			/* set replycode and wait for user's input */
			if (strncmp(rbuf, "220", 3) == 0 || strncmp(rbuf, "530", 3) == 0) {
				strcat(rbuf, "Your name: ");
				nread += 12;
				replycode = USERNAME;

			} else if (strncmp(rbuf, "150", 3) == 0) {
				if (write(STDOUT_FILENO, rbuf, nread) != nread)
					printf("write error to stdout\n");
				ftp_list(fd);
				if (write(STDOUT_FILENO, "ftp> ", 5) != 5)
					printf("write error to stdout\n");
				continue;


			} else if (strncmp(rbuf, "331", 3) == 0) {
				strcat(rbuf, "Your password: ");
				nread += 17;
				if (write(STDOUT_FILENO, rbuf, nread) != nread)
					printf("write error to stdout\n");
				getpasswd(rbuf);

				nwrite = nread + 5;
				rbuf = strcat(rbuf, "\n");
				sprintf(wbuf, "PASS %s\n", rbuf);
				printf("\n");
				if (write(sockfd, wbuf, nwrite) != nwrite)//用nwrite控制位数
					printf("write error\n");
				continue;

			} else if (strncmp(rbuf, "227", 3) == 0) {
				strtosrv(rbuf, host, port);
				fd = cliopen(host, port);
				write(sockfd, wbuf1, nwrite);
				nwrite = 0;

			} else if (strncmp(rbuf, "350", 3) == 0) {
				strcat(rbuf, "[To name:] ");
				nread += 11;
				if (write(STDOUT_FILENO, rbuf, nread) != nread)
					printf("write error to stdout\n");

				if ((nread = read(STDIN_FILENO, rbuf, MAXBUF)) < 0)//nread：真实读取的字节数
					printf("read error from stdin\n");

				sprintf(wbuf, "RNTO %s\n", rbuf);
				if (write(sockfd, wbuf, nwrite) != nwrite)
					printf("write error\n");
				replycode = 250;
				continue;

			} else if (strncmp(rbuf, "221", 3) == 0) {
				if (write(STDOUT_FILENO, rbuf, nread) != nread)
					printf("write error to stdout\n");
				break;

			} else {
				/*************************************************************
				// 9. code here: handle other response coming from server
				*************************************************************/

				if (strncmp(rbuf, "230", 3) == 0) {
					replycode = LOGIN;
					strcat(rbuf, "WELCOME!\n");
					nread += 9;
				}

				/* Cd */
				if (strncmp(rbuf, "226", 3) == 0) {
					replycode = CLOSEDATA;
				}


				if (strncmp(rbuf, "214", 3) == 0) {
					help();
					if (write(STDOUT_FILENO, "ftp> ", 5) != 5)
						printf("write error to stdout\n");
					continue;
				}

				if (strncmp(rbuf, "500", 3) == 0) {
					strcat(rbuf, "You may want to enter:\n");
					nread += 23;

					if (write(STDOUT_FILENO, rbuf, nread) != nread)
						printf("write error to stdout\n");
					help();
					if (write(STDOUT_FILENO, "ftp> ", 5) != 5)
						printf("write error to stdout\n");
					continue;
				}
				if (strncmp(rbuf, "250", 3) == 0) {

					replycode = 250;
				}

				if (strncmp(rbuf, "501", 3) == 0) {
					strcat(rbuf, "Please check your input.\n");
					nread += 24;
					replycode = 501;
				}
				if (strncmp(rbuf, "200 Type set to A", 17) == 0) {
					asciiFlag = 1;
//					replycode = 200;
				}
				if (strncmp(rbuf, "200 Type set to I", 17) == 0) {
					asciiFlag = 0;
//					replycode = 200;
				}
				strcat(rbuf, "ftp> ");
				nread += 5;

			}
			if (write(STDOUT_FILENO, rbuf, nread) != nread)
				printf("write error to stdout\n");
		}
	}

	if (close(sockfd) < 0)
		printf("close error\n");
}


/* Read and write as data transfer connection */
void ftp_list(int sockfd) {
	int nread;
	for (;;) {
		/* data to read from socket */
		if ((nread = recv(sockfd, rbuf1, MAXBUF, 0)) < 0)
			printf("recv error\n");
		else if (nread == 0)
			break;

		if (write(STDOUT_FILENO, rbuf1, nread) != nread)
			printf("send error to stdout\n");
	}

	if (close(sockfd) < 0)
		printf("close error\n");
}

/* download file from ftp server */
int ftp_get(int sockfd, char *pDownloadFileName_s) {
	/*************************************************************
	// 10. code here
	*************************************************************/
	int tread, nwrite, replycode, socketfd, nread;
	char *host;
	char *port;
	char *notice;
	host = (char *) malloc(sizeof(char) * MAX);
	memset(host, '\0', MAX);
	port = (char *) malloc(sizeof(char) * MAX);
	memset(port, '\0', MAX);
	notice = (char *) malloc(sizeof(char) * MAX);
	memset(notice, '\0', MAX);

	memset(wbuf, 0, MAXBUF);
	memset(wbuf1, 0, MAXBUF);
	sprintf(wbuf, "%s", "PASV\n");
	write(sockfd, wbuf, 5);


	if (tread = read(sockfd, rbuf, MAXBUF) < 0) {
		printf("%s\n", rbuf);
		printf("Read error from sockfd\n");
		close(socketfd);
		return 0;
	} else {
		printf("%s\n", rbuf);
	}


	strtosrv(rbuf, host, port);
	socketfd = cliopen(host, port);

	pDownloadFileName_s[strlen(pDownloadFileName_s) - 3] = '\0';

	sprintf(wbuf1, "RETR %s\n", pDownloadFileName_s);
	rbuf[strlen(rbuf) - 1] = 0;//解决win下末尾为/r的问题，需要加上/r判断
	nwrite = strlen(wbuf1);

	if (write(sockfd, wbuf1, nwrite) != nwrite) {
		printf("get write error\n");
		close(socketfd);
		return 0;
	}

	memset(rbuf, 0, MAXBUF);
	recv(sockfd, rbuf, MAXBUF, 0);
	if (strncmp(rbuf, "550", 3) == 0) {
		sprintf(notice, "%sftp>", rbuf);
		tread = strlen(notice);
		if (write(STDOUT_FILENO, notice, tread) != tread)
			printf("write error to stdout\n");
		return 0;
	}

	if (strncmp(rbuf, "150", 3) == 0) {
		printf("%s", rbuf);
	} else {
		write(STDOUT_FILENO, rbuf, nread);
		close(socketfd);
		return 0;
	}


	int file_get = open(pDownloadFileName_s, O_WRONLY | O_CREAT,
						S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
	if (file_get == -1) {
		printf("Failed to open remote file [%s]. \n", pDownloadFileName_s);
		close(file_get);
		close(socketfd);
		return 0;
	}

	for (;;) {
		/* data to read from socket */
		memset(rbuf, 0, MAXBUF);
		if ((nread = recv(socketfd, rbuf, MAXBUF, 0)) < 0)
			printf("recv error\n");
		else if (nread == 0)
			break;

		int rr = 0;
		int rrr = 0;
		int rrlength = strlen(rbuf);
		for (rr = 0; rr < rrlength - 1; rr++) {
			if (rbuf[rr] == '\r' && rbuf[rr + 1] == '\n') {
				rbuf[rr] = '\n';
				rrlength--;
				for (rrr = rr + 1; rrr < rrlength - 1; rrr++)
					rbuf[rrr] = rbuf[rrr + 1];

			}
		}


		if (write(file_get, rbuf, nread) != nread) {
			printf("write error to file\n");
			close(file_get);
			close(socketfd);
			return 0;
		}

	}
	if (close(file_get) < 0) {
		printf("close file failed\n");
		close(socketfd);
		return 0;
	}


	if (close(socketfd) < 0)
		printf("close socket for get failed\n");
	return 0;

}

/* upload file to ftp server */
int ftp_put(int sockfd, char *pUploadFileName_s, int asciiFlag) {
	/*************************************************************
	// 11. code here
	***********************************t**************************/
	int tread, nwrite, replycode, socketfd;
	char *host;
	char *port;
	char *notice;
	host = (char *) malloc(sizeof(char) * MAX);
	memset(host, '\0', MAX);
	port = (char *) malloc(sizeof(char) * MAX);
	memset(port, '\0', MAX);
	notice = (char *) malloc(sizeof(char) * MAX);
	memset(notice, '\0', MAX);

	pUploadFileName_s[strlen(pUploadFileName_s) - 3] = '\0';
	int file_put = open(pUploadFileName_s, O_RDONLY,
						S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
	close(file_put);
	if (file_put == -1) {
		sprintf(notice, "Local file [%s] does not exist.\nftp>", pUploadFileName_s);
		tread = strlen(notice);
		if (write(STDOUT_FILENO, notice, tread) != tread)
			printf("write error to stdout\n");
		return 0;
	} else {
		printf("Local file [%s] exists. \n", pUploadFileName_s);
	}

	memset(wbuf, 0, MAXBUF);
	memset(wbuf1, 0, MAXBUF);
	sprintf(wbuf, "%s", "PASV\n");
	write(sockfd, wbuf, 5);

	if (tread = read(sockfd, rbuf, MAXBUF) < 0) {
//		printf("%s\n", rbuf);
		printf("Read error from sockfd\n");
		close(socketfd);
		return 0;
	} else {
		printf("%s\n", rbuf);
	}


	strtosrv(rbuf, host, port);
	socketfd = cliopen(host, port);

	sprintf(wbuf1, "STOR %s\n", pUploadFileName_s);
	nwrite = strlen(wbuf1);
	write(sockfd, wbuf1, nwrite);
	memset(rbuf, '\0', MAXBUF);
	recv(sockfd, rbuf, MAXBUF, 0);
	if (strncmp(rbuf, "150", 3) == 0)
		printf("%s\n", rbuf);
	else {
		printf("%s\n", rbuf);
		close(socketfd);
		return 0;
	}
	file_put = open(pUploadFileName_s, O_RDONLY,
					S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
	if (file_put == -1) {
		printf("Local file does not exist. \n");
		close(file_put);
		close(socketfd);
		return 0;
	}
	int put_len;
	for (;;) {
		memset(wbuf, '\0', MAXBUF);
		memset(wbuf1, '\0', MAXBUF);
		put_len = read(file_put, wbuf, MAXBUF);


		if (asciiFlag == 1) {
			int rr = 0;
			int rrr = 0;
			int rrlength = strlen(wbuf);
			for (rr = 0; rr < rrlength - 1; rr++) {
				if (wbuf[rr] == '\n') {
					wbuf1[rrr] == '\r';
					rrr++;
					wbuf1[rrr] = '\n';
				} else {
					wbuf1[rrr] = wbuf[rr];
				}
				rrr++;
			}
			printf("==>wbuf: %s", wbuf1);
		} else {
			wbuf1 = wbuf;
		}

		if (put_len > 0) {
			send(socketfd, wbuf1, put_len, 0);
		} else
			break;
	}

	if (close(file_put) < 0) {
		printf("close file failed\n");
		close(socketfd);
		return 0;
	}


	if (close(socketfd) < 0)
		printf("close socket for get failed\n");
	return 0;
}

/* hidden password */
void getpasswd(char *passwd) {
	//printf("in getpassword\n");
	int c, n = 0;
	do {
		c = mygetch();
		if (c != '\n' && c != 'r' && c != 127) {
			passwd[n] = c;
//			printf("*");
			n++;
		} else if ((c != '\n' | c != '\r') && c == 127) {//判断是否是回车或则退格
			if (n > 0) {
				n--;
				printf("\b \b");//输出退格
			}
		}
	} while (c != '\n' && c != '\r');
	passwd[n] = '\0';//消除一个多余的回车
}

/* get the input character */
int mygetch() {
	struct termios oldt, newt;
	int ch;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	ch = getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return ch;
}

void strrpc(char *str, char *oldstr, char *newstr) {

	char bstr[strlen(str)];//转换缓冲区
	memset(bstr, 0, sizeof(bstr));
	int i = 0;
	for (i = 0; i < strlen(str); i++) {
		if (!strncmp(str + i, oldstr, strlen(oldstr))) {//查找目标字符串
			strcat(bstr, newstr);
			i += strlen(oldstr) - 1;
		} else {
			strncat(bstr, str + i, 1);//保存一字节进缓冲区
		}
	}
	strcpy(str, bstr);
}


void help() {
	printf("               ====================================COMMAND TABLE====================================\n");
	printf("               = pwd            View the absolute path of the current working directory.           =\n");
	printf("               = cd             Switch current working directory.                                  =\n");
	printf("               = mkdir          Make a directory.                                                  =\n");
	printf("               = rename         Change the name of the file.                                       =\n");
	printf("               = ls             List the files and subdirectories.                                 =\n");
	printf("               = get            Download the file.                                                 =\n");
	printf("               = put            Upload the file.                                                   =\n");
	printf("               = binary/ascii   binary/ascii.                                                      =\n");
	printf("               = quit           Terminate this program.                                            =\n");
	printf("               = rest           Continue transmission.                                             =\n");
	printf("               = help           View more detail about this program.                               =\n");
	printf("               ====================================COMMAND TABLE====================================\n");

}


char *subString(char *src, int start, int end) {
	char *dest;
	int i = start;
	if (start > strlen(src))
		return;
	if (end > strlen(src))
		end = strlen(src);
	while (i < end) {
		dest[i - start] = src[i];
		i++;
	}
	dest[i - start] = '\0';
	return dest;
}

//删除开始n个字符的函数
void strcut(char *str, int n) {
	int num = 0;
	for (; num < strlen(str) - n; num++)
		*(str + num) = *(str + num + n);
	str[num] = '\0';

}


int rest(int sockfd, char *fromPoint) {

	rbuf = (char *) malloc(sizeof(char) * MAXBUF_1);
	memset(rbuf, '\0', MAXBUF_1);
	wbuf = (char *) malloc(sizeof(char) * MAXBUF_1);
	memset(wbuf, '\0', MAXBUF_1);

	printf("Please input (get local-filename remote-filename) or (put remote-filename local-filename)\n");

	int nread;

	if ((nread = read(STDIN_FILENO, rbuf, MAXBUF)) < 0)//nread：真实读取的字节数
		printf("read error from stdin\n");
	else {
		rbuf = strcat(rbuf, "\n\n");
	}

	int i = 0;
	char *x;
	char dir[MAX][MAX];
	char *notice;
	notice = (char *) malloc(sizeof(char) * MAX);
	memset(notice, '\0', MAX);


	if (strncmp(rbuf, "put", 3) == 0) {

		x = strtok(rbuf, " ");
		i = 0;
		for (i; x != NULL; i++) {
			strcpy(dir[i], x);
			x = strtok(NULL, " ");
//			printf("===>[%s]", dir[i]);
		}
		if (i != 3) {
			notice = "Please check your input.\n(usage: put remote-filename local-filename)\nftp>";
			nread = strlen(notice);
			if (write(STDOUT_FILENO, notice, nread) != nread)
				printf("write error to stdout\n");
			return -1;
		}


		if (strlen(dir[i - 1]) <= 0) {
			printf("Please input filename\n");
			return -1;
		}//dir[i-1]: file to get

		char *pUploadFileName_s = dir[i - 1];
		int tread, nwrite, replycode, socketfd;
		char *host;
		char *port;
		char *notice;
		host = (char *) malloc(sizeof(char) * MAX);
		memset(host, '\0', MAX);
		port = (char *) malloc(sizeof(char) * MAX);
		memset(port, '\0', MAX);
		notice = (char *) malloc(sizeof(char) * MAX);
		memset(notice, '\0', MAX);

		pUploadFileName_s[strlen(pUploadFileName_s) - 3] = '\0';
		int file_put = open(pUploadFileName_s, O_RDONLY,
							S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
		close(file_put);
		if (file_put == -1) {
			sprintf(notice, "Local file [%s] does not exist.\nftp>", pUploadFileName_s);
			tread = strlen(notice);
			if (write(STDOUT_FILENO, notice, tread) != tread)
				printf("write error to stdout\n");
			return 0;
		} else {
			printf("Local file [%s] exists. \n", pUploadFileName_s);
		}

		memset(wbuf, 0, MAXBUF);
		memset(wbuf1, 0, MAXBUF);
		sprintf(wbuf, "%s", "PASV\n");
		write(sockfd, wbuf, 5);

		if (tread = read(sockfd, rbuf, MAXBUF) < 0) {
			printf("%s\n", rbuf);
			printf("Read error from sockfd\n");
			close(socketfd);
			return 0;
		} else {
			printf("%s\n", rbuf);
		}


		strtosrv(rbuf, host, port);
		socketfd = cliopen(host, port);

		sprintf(wbuf, "REST %s\n", fromPoint);
		nwrite = strlen(fromPoint) + 6;
		write(sockfd, wbuf, nwrite);

		if ((nread = recv(sockfd, rbuf, MAXBUF, 0)) < 0)
			printf("recv error\n");
		else if (nread == 0) {
			return 0;
		}
		if (write(STDOUT_FILENO, rbuf, nread) != nread) {
			printf("write error to stdout\n");
		}

		if (strncmp(rbuf, "350", 3) != 0) {
			sprintf(notice, "%sftp>", rbuf);
			tread = strlen(notice);
			if (write(STDOUT_FILENO, notice, tread) != tread)
				printf("write error to stdout\n");
			return 0;
		}

		sprintf(wbuf1, "STOR %s\n", dir[i - 2]);
		nwrite = strlen(wbuf1);
		write(sockfd, wbuf1, nwrite);
		memset(rbuf, '\0', MAXBUF);
		recv(sockfd, rbuf, MAXBUF, 0);
		if (strncmp(rbuf, "150", 3) == 0)
			printf("%s\n", rbuf);
		else {
			printf("%s\n", rbuf);
			close(socketfd);
			return 0;
		}
		file_put = open(pUploadFileName_s, O_RDWR, 0);
		if (file_put == -1) {
			printf("Local file does not exist. \n");
			close(file_put);
			close(socketfd);
			return 0;
		}
		printf("lseek(file_put, fromPoint, SEEK_SET): %d\n", lseek(file_put, atoi(fromPoint), SEEK_SET));


		int put_len;
		for (;;) {
			memset(wbuf, '\0', MAXBUF);
			put_len = read(file_put, wbuf, MAXBUF);
			if (put_len > 0) {
				send(socketfd, wbuf, put_len, 0);
			} else
				break;
		}

		if (close(file_put) < 0) {
			printf("close file failed\n");
			close(socketfd);
			return 0;
		}


		if (close(socketfd) < 0)
			printf("close socket for get failed\n");
		return 0;

	}


	if (strncmp(rbuf, "get", 3) == 0) {
		x = strtok(rbuf, " ");
		i = 0;
		for (i; x != NULL; i++) {
			strcpy(dir[i], x);
			x = strtok(NULL, " ");
		}
		if (i != 3) {
			notice = "Please check your input.\n(usage: get local-filename remote-filename)\nftp>";
			nread = strlen(notice);
			if (write(STDOUT_FILENO, notice, nread) != nread)
				printf("write error to stdout\n");
			return -1;
		}


		if (strlen(dir[i - 1]) <= 0) {
			printf("Please input filename\n");
			return -1;
		}//dir[i-1]: file to get



		char *pDownloadFileName_s = dir[i - 1];
		int tread, nwrite, replycode, socketfd, nread;
		char *host;
		char *port;
		char *notice;
		host = (char *) malloc(sizeof(char) * MAX);
		memset(host, '\0', MAX);
		port = (char *) malloc(sizeof(char) * MAX);
		memset(port, '\0', MAX);
		notice = (char *) malloc(sizeof(char) * MAX);
		memset(notice, '\0', MAX);


		int file_get_local = open(dir[i - 2], O_RDWR,
								  S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH |
								  S_IXOTH);
		close(file_get_local);
		if (file_get_local == -1) {
			sprintf(notice, "Local file [%s] does not exist.\nftp>", dir[i - 2]);
			tread = strlen(notice);
			if (write(STDOUT_FILENO, notice, tread) != tread)
				printf("write error to stdout\n");
			return 0;
		} else {
			printf("Local file [%s] exists. \n", dir[i - 2]);
		}


		memset(wbuf, 0, MAXBUF);
		memset(wbuf1, 0, MAXBUF);
		sprintf(wbuf, "%s", "PASV\n");
		write(sockfd, wbuf, 5);


		if (tread = read(sockfd, rbuf, MAXBUF) < 0) {
			printf("%s\n", rbuf);
			printf("Read error from sockfd\n");
			close(socketfd);
			return 0;
		} else {
			printf("%s\n", rbuf);
		}


		strtosrv(rbuf, host, port);
		socketfd = cliopen(host, port);


		sprintf(wbuf, "REST %s\n", fromPoint);
		nwrite = strlen(fromPoint) + 6;
		write(sockfd, wbuf, nwrite);

		if ((nread = recv(sockfd, rbuf, MAXBUF, 0)) < 0)
			printf("recv error\n");
		else if (nread == 0) {
			return 0;
		}
		if (write(STDOUT_FILENO, rbuf, nread) != nread) {
			printf("write error to stdout\n");
		}

		if (strncmp(rbuf, "350", 3) != 0) {
			sprintf(notice, "%sftp>", rbuf);
			tread = strlen(notice);
			if (write(STDOUT_FILENO, notice, tread) != tread)
				printf("write error to stdout\n");
			return 0;
		}


		pDownloadFileName_s[strlen(pDownloadFileName_s) - 3] = '\0';

		sprintf(wbuf1, "RETR %s\n", pDownloadFileName_s);
		rbuf[strlen(rbuf) - 1] = 0;//解决win下末尾为/r的问题，需要加上/r判断
		nwrite = strlen(wbuf1);

		if (write(sockfd, wbuf1, nwrite) != nwrite) {
			printf("get write error\n");
			close(socketfd);
			return 0;
		}

		memset(rbuf, 0, MAXBUF);
		recv(sockfd, rbuf, MAXBUF, 0);
		if (strncmp(rbuf, "550", 3) == 0) {
			sprintf(notice, "%sftp>", rbuf);
			tread = strlen(notice);
			if (write(STDOUT_FILENO, notice, tread) != tread)
				printf("write error to stdout\n");
			return 0;
		}

		if (strncmp(rbuf, "150", 3) == 0) {
			printf("%s", rbuf);
		} else {
			write(STDOUT_FILENO, rbuf, nread);
			close(socketfd);
			return 0;
		}


		int file_get = open(pDownloadFileName_s, O_WRONLY | O_CREAT,
							S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
		if (file_get == -1) {
			printf("Failed to open remote file [%s]. \n", pDownloadFileName_s);
			close(file_get);
			close(socketfd);
			return 0;
		}

		file_get_local = open(dir[i - 2], O_RDWR | O_APPEND,
							  S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH |
							  S_IXOTH);
		if (file_get_local == -1) {
			printf("Failed to open LOCAL file [%s]. \n", dir[i - 2]);
			close(file_get);
			close(socketfd);
			return 0;
		}

		for (;;) {
			/* data to read from socket */
			memset(rbuf, 0, MAXBUF);
			if ((nread = recv(socketfd, rbuf, MAXBUF, 0)) < 0)
				printf("recv error\n");
			else if (nread == 0)
				break;

			int rr = 0;
			int rrr = 0;
			int rrlength = strlen(rbuf);
			for (rr = 0; rr < rrlength - 1; rr++) {
				if (rbuf[rr] == '\r' && rbuf[rr + 1] == '\n') {
					rbuf[rr] = '\n';
					rrlength--;
					for (rrr = rr + 1; rrr < rrlength - 1; rrr++)
						rbuf[rrr] = rbuf[rrr + 1];

				}
			}

			if (write(file_get_local, rbuf, nread) != nread) {
				printf("write error to file\n");
				close(file_get);
				close(socketfd);
				close(file_get_local);
				return 0;
			}

		}
		if (close(file_get) < 0) {
			printf("close file failed\n");
			close(socketfd);
			close(file_get_local);
			return 0;
		}


		if (close(socketfd) < 0)
			printf("close socket for get failed\n");

		close(file_get_local);

		memset(rbuf, 0, MAXBUF);
		memset(wbuf, 0, MAXBUF);
		memset(rbuf1, 0, MAXBUF);
		return 0;
	}

	return 0;

}
