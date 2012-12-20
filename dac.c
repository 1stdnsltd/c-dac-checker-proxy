#include<stdio.h>
#include<string.h>	//strlen
#include<stdlib.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>	//write
#include<netdb.h>
#include<arpa/inet.h>
#include<pthread.h> //for threading , link with lpthread

#define DAC_HOSTNAME "dac.nominet.org.uk";
#define BUFFER_SIZE 1024

void *connection_handler(void *);

struct thread_args {
	int *server;
	int *nominet;
};

int main(int argc , char *argv[]) {
	struct thread_args args;
	char *nominet_server_name = DAC_HOSTNAME;
	char ip[100];
	struct hostent *he;
	struct in_addr **addr_list;
	int socket_desc , nominet_socket_desc, new_socket , c , *new_sock, *nominet;
	struct sockaddr_in server , client, nominet_server;
	char *message, server_reply[1000];
	int i;
int optval;
   socklen_t optlen = sizeof(optval);

	nominet_socket_desc = socket(AF_INET , SOCK_STREAM, 0);
	if (nominet_socket_desc == -1 ){
		printf("Could not connect to nominet\n");
		return 1001;
	}

/* Check the status for the keepalive option */
   if(getsockopt(nominet_socket_desc, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
      perror("getsockopt()");
      close(nominet_socket_desc);
      exit(EXIT_FAILURE);
   }
   printf("SO_KEEPALIVE is %s\n", (optval ? "ON" : "OFF"));

   /* Set the option active */
   optval = 1;
   optlen = sizeof(optval);
   if(setsockopt(nominet_socket_desc, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
      perror("setsockopt()");
      close(nominet_socket_desc);
      exit(EXIT_FAILURE);
   }
   printf("SO_KEEPALIVE set on socket\n");

   /* Check the status again */
   if(getsockopt(nominet_socket_desc, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
      perror("getsockopt()");
      close(nominet_socket_desc);
      exit(EXIT_FAILURE);
   }
   printf("SO_KEEPALIVE is %s\n", (optval ? "ON" : "OFF"));

if ( (he = gethostbyname( nominet_server_name) ) == NULL ) {
	herror("Unable to get Nominet IP address\n");
	return 1003;
} else {
	addr_list = (struct in_addr **) he->h_addr_list;
	for ( i = 0; addr_list[i] != NULL; i++ ) {
		strcpy( ip, inet_ntoa( *addr_list[i] ) );
	}
}

	nominet_server.sin_addr.s_addr = inet_addr( ip );
	nominet_server.sin_family = AF_INET;
	nominet_server.sin_port = htons( 3043 );

if (connect(nominet_socket_desc, (struct sockaddr *)&nominet_server, sizeof(nominet_server)) < 0) {
	puts("Connect to nominet failed\n");
	return 1002;
} else {
	//puts("connected to nominet\n");
}


	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if ( socket_desc == -1 ) {
		printf( "Could not create socket\n" );
	}

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( 3043 );

	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0 ) {
		puts("Failed to bind to socket for incomming connections\n");
		return 1;
	} else {
		puts("bind done\n");
	}
	//Listen
	listen(socket_desc , 3);
	//Accept and incoming connection
	puts("Waiting for incoming connections...\n");
	c = sizeof(struct sockaddr_in);
	while( (new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) ) {
		puts("Connection accepted\n");
		//Reply to the client
		//message = "Hello Client , I have received your connection. And now I will assign a handler for you\n";
		//write(new_socket , message , strlen(message));
		pthread_t sniffer_thread;
		//new_sock = malloc(1);
		//*new_sock = new_socket;
		//nominet = malloc(1);
		//*nominet = nominet_socket_desc;
		args.nominet = malloc(1);
		*args.nominet = nominet_socket_desc;
		args.server = malloc(1);
		*args.server = new_socket;
		if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void *)&args) < 0) {
			perror("could not create thread\n");
			return 1;
		}
		//Now join the thread , so that we dont terminate before the thread
		//pthread_join( sniffer_thread , NULL);
		puts("Handler assigned\n");
	}
	if ( new_socket < 0 ) {
		perror("accept failed\n");
		return 1;
	}
	return 0;
}
/*
 * This will handle connection for each client
 * */
void *connection_handler( void *args ) {
	//Get the socket descriptor
	struct thread_args *get = args;
	int *sock = get->server;
	int *nominet = get->nominet;
	char *message;
	char buffer[BUFFER_SIZE];
	int readsock;
	char nomdata[BUFFER_SIZE];
	int nomcount;
	//Send some messages to the client
	//message = "Greetings! I am your connection handler\n";
	//write(*sock , message , strlen(message));
	//message = "Its my duty to communicate with you\n";
	//write(*sock , message , strlen(message));
	if (( readsock = recv(*sock, buffer, BUFFER_SIZE - 1, 0) == -1)) {
		perror("Failed to recieve Data");
		exit(1004);
	}
	//puts("GotHere\n");
	//buffer[readsock] = 'a';
	printf("String: %s\n", buffer);

	write(*nominet, buffer, strlen(buffer));
	if (( nomcount = recv(*nominet, nomdata, BUFFER_SIZE -1, 0) == -1 ) ) {
		perror("Data failed to recieve from Nominet");
		exit(1005);
	}

	//nomdata[nomcount] = ;
	printf("Nominet: %s\n", nomdata);
	write(*sock, nomdata, strlen(nomdata));
	///puts(buffer);
//	write(*sock, readsock, strlen(readsock));
	//Free the socket pointer
	//free();
	return 0;
}
