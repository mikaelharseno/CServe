#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unistd.h>

#include "libhttp.h"
#include "wq.h"

#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/prctl.h>
/*
 * Global configuration variables.
 * You need to use these in your implementation of handle_files_request and
 * handle_proxy_request. Their values are set up in main() using the
 * command line arguments (already implemented for you).
 */
wq_t work_queue;
int num_threads;
int server_port;
char *server_files_directory;
char *server_proxy_hostname;
int server_proxy_port;


/*
 * Reads an HTTP request from stream (fd), and writes an HTTP response
 * containing:
 *
 *   1) If user requested an existing file, respond with the file
 *   2) If user requested a directory and index.html exists in the directory,
 *      send the index.html file.
 *   3) If user requested a directory and index.html doesn't exist, send a list
 *      of files in the directory with links to each.
 *   4) Send a 404 Not Found response.
 */
void handle_files_request(int fd) {

  /*
   * TODO: Your solution for Task 2 goes here! Feel free to delete/modify *
   * any existing code.
   */
  //int result = select(sock_client
  
  struct http_request *request = http_request_parse(fd);
  if (request == NULL) {
    printf("Request parse returns NULL");
    return;
  }
  //struct http_request *request = malloc(sizeof(struct http_request));
  //request->method = "GET";
  //request->path = "/";

  printf("Method: %s \n", request->method);
  printf("Path: %s \n", request->path);

  char* buff = malloc(500);
  if (buff == NULL) {
    printf("Buffer allocation error \n");
    return;
  }
 /* if (*(server_files_directory) != '/') {
    char* indi = getcwd(buff, 4096);
    if (indi == NULL) {
      printf("Getcwd returns NULL \n");
      return;
    }
  }*/
 /* *(buff+0) = '.';
  *(buff+1) = '/';*/
  *(buff+0) = '\0';
  /*printf("Buff: %s \n", buff);*/
  int strlength = strlen(buff);
  char last = *(buff+strlength-1);
  /*printf("Last: %c \n", last);
  if (last == '/') {
    *(buff+strlength-1) = '\0';
  }

  printf("Method: %s \n", request->method); 
  printf("Path: %s \n", request->path); 
  */
  char adder[10];
  strcpy(adder, "./");
  //Adds / to end of buff before adding server files directory
  if (*(server_files_directory) != '/') {
    strcat(buff,adder); //Adds local if path is relative
    //printf("Buff: %s \n", buff);
  }

  strcat(buff, server_files_directory);
  /*
  strlength = strlen(buff);
  last = *(buff+strlength-1);
  printf("Last: %c \n", last);
  if (last == '/') {
    *(buff+strlength-1) = '\0';
  }*/
  
  strlength = strlen(buff);
  last = *(buff+strlength-1);
  if (last == '/') {
    *(buff+strlength-1) = '\0';
  }
  
  //Adds / to end of buff before adding server files directory
  /*if (*(request->path) != '/') {
    strcat(buff,"/");
    printf("Buff: %s \n", buff);
  }*/
  
  printf("Buff before path: %s \n",buff);
  //printf("Is this the problem? %s \n",request->path);
  strcat(buff, request->path); // Request always starts with /, which we avoid
  printf("Buff after path: %s \n",buff);
  
  if (strcmp(request->method,"GET") != 0) {
    printf("Not equal to GET");
    free(buff);
    return;
  }

/*  strlength = strlen(buff);
  last = *(buff+strlength-1);
  //printf("Last: %c \n", last);
  if (last == '/') {
    *(buff+strlength-1) = '\0';
  }*/
  
  //CHeck if thing is a directory or file
  struct stat temp;
  FILE* filetemp;
  DIR* dirtemp;

  //strcat(buff,adder);

  if (stat(buff,&temp) != 0) {
    http_start_response(fd, 404);
    http_end_headers(fd);

    printf("File/dir stat failed");
    free(buff);
    return;
  }

  if(S_ISDIR(temp.st_mode)) {
    printf("Path is a directory");
    dirtemp = opendir(buff);
    if (dirtemp == NULL) {
      printf("Directory open failed \n");
      free(buff);
      return;
    }
    struct dirent *cur;
    strlength = strlen(buff);
    last = *(buff+strlength-1);
    if (last != '/') {
      strcat(buff,"/");
    }
    strcat(buff,"index.html");
    printf("%s\n",buff);
    if (stat(buff,&temp) != 0) { //Either we can't get to new file or it's not a file
      printf("File index.html in dir not found. Showing directory list.\n");
      char* dircontent = malloc(8192);
      if (dircontent == NULL) {
        printf("Dircontent allocation error \n");
        free(buff);
        return;
      }      
      dircontent[0] = '\0';

      strcat(dircontent, "<a href=\"../\">Parent directory</a>\r\n");
      cur = readdir(dirtemp);
      while (cur != NULL) {
	if ((strcmp(cur->d_name,".") == 0) || (strcmp(cur->d_name,"..")==0)) {
          cur = readdir(dirtemp);
	  continue;
	}
        strcat(dircontent, "<a href=\"./");
	strcat(dircontent, cur->d_name);
	strcat(dircontent, "\">");
	strcat(dircontent, cur->d_name);
	strcat(dircontent, "</a>\r\n");
        cur = readdir(dirtemp);
      }
      closedir(dirtemp);

      char dirfilelen[20];
      dirfilelen[0] = '\0';
      sprintf(dirfilelen,"%d",(int) strlen(dircontent));
      
      http_start_response(fd, 200);
      http_send_header(fd, "Content-Type", "text/html");
      http_send_header(fd, "Content-Length", dirfilelen);
      http_end_headers(fd);
      http_send_string(fd,dircontent);
      
      free(dircontent);
//printf("safe and sound");
      free(buff);	
      return; 
    }
  }

  //Check if file last stat ed is actually a regular file
  if(!(S_ISREG(temp.st_mode))) {
    http_start_response(fd, 404);
    http_end_headers(fd);
  }
  
  // Opens files for both directory index.html and regular files
  filetemp = fopen(buff, "r");
  if (filetemp == NULL) {
    printf("file open failed");
    free(buff);
    return;
  } else {
    printf("open success");
  }

  char* mimetype;
  char filelength[20];
  int filesize = ((int) temp.st_size);
  char* filecontent = malloc(filesize);
  if (filecontent == NULL) {
    printf("File content allocation error \n");
    free(buff);
    return;
  }
  //*filecontent = '\0';
  //char* curline = malloc(600);
  sprintf(filelength, "%d", filesize);
  fread(filecontent, 1, filesize, filetemp);
  fclose(filetemp);

  mimetype = http_get_mime_type(buff);

  http_start_response(fd, 200);
  http_send_header(fd, "Content-Type", mimetype);
  //printf("%s",filelength);
//  printf("\n");
  http_send_header(fd, "Content-Length", filelength);
  http_end_headers(fd);
  //printf("%s",filecontent);
//  printf("\n");
  http_send_data(fd, filecontent, filesize);
  
  free(filecontent);
  free(buff);
}


/*
 * Opens a connection to the proxy target (hostname=server_proxy_hostname and
 * port=server_proxy_port) and relays traffic to/from the stream fd and the
 * proxy target. HTTP requests from the client (fd) should be sent to the
 * proxy target, and HTTP responses from the proxy target should be sent to
 * the client (fd).
 *
 *   +--------+     +------------+     +--------------+
 *   | client | <-> | httpserver | <-> | proxy target |
 *   +--------+     +------------+     +--------------+
 */
void handle_proxy_request(int fd) {

  /*
   * TODO: Task 4 - Do a DNS lookup of server_proxy_name and open a connection to it.
   */
  struct addrinfo ad;
  ad.ai_socktype = SOCK_STREAM;
  ad.ai_family = AF_INET;
  struct addrinfo *res;
  char port[20];
  sprintf(port,"%d",server_proxy_port);
  int n = getaddrinfo(server_proxy_hostname, port, &ad, &res);
  if (n != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(n));
      return;
  }

  int client_socket_fd = fd;

  int proxy_socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (proxy_socket_fd == -1) {
    printf("Proxy socket failed \n");
    return;
  }

  if (connect(proxy_socket_fd, res->ai_addr, res->ai_addrlen) == -1) {
    printf("Connect to proxy failed \n");
    http_request_parse(client_socket_fd);

    http_start_response(client_socket_fd, 502);
    http_send_header(client_socket_fd, "Content-Type", "text/html");
    http_end_headers(client_socket_fd);
    http_send_string(client_socket_fd, "<center><h1>502 Bad Gateway</h1><hr></center>");
    close(proxy_socket_fd);
    return;
  }
  
  char buf[4096];
  int len;
  int arg2 = 0;
  int stat;
  //pid_t childid;
  pid_t pid = fork();

  if (pid == 0) {
    //Child
    while (1) {
      prctl(PR_GET_PDEATHSIG,arg2);
      if (arg2 > 0) {    
        shutdown(proxy_socket_fd, SHUT_WR);
        close(proxy_socket_fd);
        shutdown(client_socket_fd, SHUT_WR);
        close(client_socket_fd);
        exit(0);
      }
      len = read(proxy_socket_fd,buf,4095);
      buf[len] = '\0';
      if (len > 0) {
        dprintf(client_socket_fd,"%s",buf);
      }
    }
  } else {
    while (1) {
      arg2 = waitpid(pid, &stat, WNOHANG);
      if (arg2 > 0) {
        shutdown(proxy_socket_fd, SHUT_WR);
        close(proxy_socket_fd);
        shutdown(client_socket_fd, SHUT_WR);
        close(client_socket_fd);
        exit(0);
      }
      len = read(client_socket_fd,buf,4096);
      buf[len] = '\0';
      if (len > 0) {
        dprintf(proxy_socket_fd,"%s",buf);
      }
    }
  }
  /*
   * TODO: Task 4 - Attempt to establish a connection to the server with the success/failure
   * stored in connection_status.
   */
}

void* thread_loop(void* arg) {
  printf("One thread starts \n");
  int tfd;
  void (*func)(int) = (void (*)(int)) arg;
  printf("%d\n",(int) arg);
  while (1) {
    printf("Thread working\n");
    tfd = wq_pop(&work_queue);
    printf("Working on fd %d \n",tfd);
    func(tfd);
    shutdown(tfd, SHUT_WR);
    close(tfd);
    printf("Handshake on port %d\n",tfd);
  }
}
   


void init_thread_pool(int num_threads, void (*request_handler)(int)) {
  /*
   * TODO: Part of your solution for Task 3 goes here!
   */
  pthread_t thr;
  size_t i;

  for(i=0;i<num_threads;i++) {
     printf("Thread created! \n");
     pthread_create( &thr, NULL , thread_loop , request_handler);
  }
  printf("Here I am!");
}

/*
 * Opens a TCP stream socket on all interfaces with port number PORTNO. Saves
 * the fd number of the server socket in *socket_number. For each accepted
 * connection, calls request_handler with the accepted fd number.
 */
void serve_forever(int *socket_number, void (*request_handler)(int)) {
  /*
   * TODO: Part of your solution to Task 1 goes here!
   */
/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
     //int sockfd, newsockfd, portno, clilen;
     //char buffer[256];
     //struct sockaddr_in serv_addr, cli_addr;
     //int n;
     //clilen = sizeof(cli_addr);
     //newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
   /*  if (newsockfd < 0) 
          error("ERROR on accept");
     bzero(buffer,256);
     n = read(newsockfd,buffer,255);
     if (n < 0) error("ERROR reading from socket");
     printf("Here is the message: %s\n",buffer);
     n = write(newsockfd,"I got your message",18);
     if (n < 0) error("ERROR writing to socket");
     return 0; */
  if (num_threads < 1) {
    num_threads = 1;
  }
  int fd = socket(AF_INET, SOCK_STREAM, 0);
//printf("fd:%d\n",fd);
  *socket_number = fd; //Sets socker number to fd
  if (fd == -1) {
    printf("Error opening socket \n");
    return;
  }
  
  struct sockaddr_in addr;
  bzero((char *) &addr, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(server_port);
//  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
//  addr.sin_addr.s_addr = htonl(0xc0a9a2a2);
//  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (bind(fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) < 0) { //Addr or struct sockaddr_in ???
    printf("Error binding \n");
    return;
  }

  /* Setting some mysterious settings for our socket. See man setsockopt for
     more info if curious. */
  int socket_option = 1;
  if (setsockopt(*socket_number, SOL_SOCKET, SO_REUSEADDR, &socket_option,
        sizeof(socket_option)) == -1) {
    perror("Failed to set socket options");
    exit(errno);
  }
  

//printf("addr:%d\n",htonl(INADDR_LOOPBACK));
  /*
   * TODO: Part of your solution to Task 1 goes here!
   */

 // request_handler(fd);

  wq_init(&work_queue);

  init_thread_pool(num_threads, request_handler);

  int indi = listen(fd, 20);
  if (indi == -1) {
    printf("Listen failed");
    return;
  }

  struct sockaddr_in addrnew;
  socklen_t lensock = sizeof(struct sockaddr_in);
//  char buf[4096];
  int fdnew,len;  
  while (1) {
    printf("In loop\n");
    //recvfrom(fd, buf, 4096,MSG_WAITALL,(struct sockaddr*) &addrnew, &lensock);
    fdnew = accept(fd, (struct sockaddr*) &addrnew, &lensock);
    //ioctl(fdnew, FIONREAD, &len);
    printf("Accepted new from %lu port %hu with connection type %hd \n",addrnew.sin_addr.s_addr,addrnew.sin_port,addrnew.sin_family);
    //request_handler(fdnew);
    //do {
    //  len = read(fdnew, buf, 4096);
    //  printf("%s \n",buf);
    //} while (len > 0);
    //fdnew = accept(fd, (struct sockaddr*) &addrnew, &lensock);
    //printf("Accepted new from %lu port %hu with connection type %hd \n",addrnew.sin_addr.s_addr,addrnew.sin_port,addrnew.sin_family);
    wq_push(&work_queue,fdnew);
  }
}

int server_fd;
void signal_callback_handler(int signum) {
  printf("Caught signal %d: %s\n", signum, strsignal(signum));
  /*
   * TODO: Part of your solution to Task 1 goes here!
   */
}

char *USAGE =
  "Usage: ./httpserver --files www_directory/ --port 8000 [--num-threads 5]\n"
  "       ./httpserver --proxy inst.eecs.berkeley.edu:80 --port 8000 [--num-threads 5]\n";

void exit_with_usage() {
  fprintf(stderr, "%s", USAGE);
  exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  /* Registering signal handler. When user enteres Ctrl-C, signal_callback_handler will be invoked. */
  struct sigaction sa;
  sa.sa_flags = SA_RESTART;
  sa.sa_handler = &signal_callback_handler;
  sigemptyset(&sa.sa_mask);
  sigaction (SIGINT, &sa, NULL);

  /* Default settings */
  server_port = 8000;
  void (*request_handler)(int) = NULL;

  int i;
  for (i = 1; i < argc; i++) {
    if (strcmp("--files", argv[i]) == 0) {
      request_handler = handle_files_request;
      free(server_files_directory);
      server_files_directory = argv[++i];
      if (!server_files_directory) {
        fprintf(stderr, "Expected argument after --files\n");
        exit_with_usage();
      }
    } else if (strcmp("--proxy", argv[i]) == 0) {
      request_handler = handle_proxy_request;

      char *proxy_target = argv[++i];
      if (!proxy_target) {
        fprintf(stderr, "Expected argument after --proxy\n");
        exit_with_usage();
      }

      char *colon_pointer = strchr(proxy_target, ':');
      if (colon_pointer != NULL) {
        *colon_pointer = '\0';
        server_proxy_hostname = proxy_target;
        server_proxy_port = atoi(colon_pointer + 1);
      } else {
        server_proxy_hostname = proxy_target;
        server_proxy_port = 80;
      }
    } else if (strcmp("--port", argv[i]) == 0) {
      char *server_port_string = argv[++i];
      if (!server_port_string) {
        fprintf(stderr, "Expected argument after --port\n");
        exit_with_usage();
      }
      server_port = atoi(server_port_string);
    } else if (strcmp("--num-threads", argv[i]) == 0) {
      char *num_threads_str = argv[++i];
      if (!num_threads_str || (num_threads = atoi(num_threads_str)) < 1) {
        fprintf(stderr, "Expected positive integer after --num-threads\n");
        exit_with_usage();
      }
    } else if (strcmp("--help", argv[i]) == 0) {
      exit_with_usage();
    } else {
      fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
      exit_with_usage();
    }
  }

  if (server_files_directory == NULL && server_proxy_hostname == NULL) {
    fprintf(stderr, "Please specify either \"--files [DIRECTORY]\" or \n"
                    "                      \"--proxy [HOSTNAME:PORT]\"\n");
    exit_with_usage();
  }
  if (request_handler == NULL) {
    printf("Not getting handler");
  }
  serve_forever(&server_fd, request_handler);

  return EXIT_SUCCESS;
}
