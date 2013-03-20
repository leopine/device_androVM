
#include <strings.h>

#include <cutils/log.h>
#include <cutils/properties.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define LOG_TAG "Genyd"

#define SERVER_PORT 22666

static int init_server(void)
{
  int server_sock = -1;
  struct sockaddr_in server_addr;
  long host_addr = -1;

  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERVER_PORT);

  if((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
      SLOGE("Cannot create socket");
      return -1;
    }

  if (bind(server_sock, (sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
      SLOGE("Cannot bind socket");
      return -1;
    }
 
  if (listen(server_sock, 5) == -1)
    {
      SLOGE("Cannot listen on socket");
      close(server_sock);
      return -1;
    }
 
  return server_sock;
}

int wait_client(int server_sock)
{
  int client_sock = -1;
  sockaddr_in client_addr;
  int client_addr_size = sizeof(client_addr);

  client_sock = accept(server_sock, (sockaddr *)&client_addr, &client_addr_size);

  if (client_sock == -1)
    {
      SLOGE("Cannot accept connection");
    }
  return (client_sock);
}

static char *read_from_socket(int socket)
{
  int len = 0;
  char *cmd = 0;
  char buffer[1024];

  if ((len = recv(socket, buffer, 1023, 0)) < 0)
    {
      SLOGE("recv() error");
    }

  if (len <= 0)
    {
      bcopy("quit\n", buffer, 6);
      len = 5;
    }
  
  cmd = (char *)malloc((len + 1) * sizeof(*cmd));
  bcopy(buffer, cmd, len * sizeof(*cmd));

  return cmd;
}

static void treate_client(int server_sock, int client_sock)
{
  fd_set readfs;
  (void)server_sock; // To be monitored to manage multi-client

  while (1)
    {
      FD_ZERO(&readfs);
      FD_SET(client_sock, &readfs);
   
      if (select(client_sock + 1, &readfs, NULL, NULL, NULL) < 0)
	{
	  SLOGE("select() error");
	  return;
	}

      if(FD_ISSET(client_sock, &readfs))
	{
	  char *cmd = read_from_socket(client_sock);

	  if (!cmd)
	    continue;

	  if (strcmp(cmd, "quit\n") == 0)
	    {
	      free(cmd);
	      return;
	    }
	  else
	    {
	      SLOGI("Received \"%s\"", cmd);
	      free(cmd);
	    }
	}
    }
}

int main(int argc, char**argv)
{
  SLOGI("Starting genyd...");

  int server_sock = init_server();

  if (server_sock > 0)
    {
      int client_sock = wait_client(server_sock);

      if (client_sock > 0)
	{
	  SLOGI("Got client");

	  int i = 0;

	  treate_client(server_sock, client_sock);

	  close(client_sock);
	}

      close(server_sock);
    }

  SLOGI("Exiting");
      
  return 0;
}
