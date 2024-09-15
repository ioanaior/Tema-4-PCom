#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

char *recv_post_req(int socket, char host[16], char *command, char *user[1], char *token) {
	char *message = compute_post_request(host, command, "application/json", user, 1, NULL, 0, token);
	send_to_server(socket, message);

	return receive_from_server(socket);
}

char *recv_get_req(int socket, char host[16], char *command, char *token, char *cookies[1], char *get_delete)
{
    char *message;
    if (strcmp(get_delete, "get") == 0)
    {
        message = compute_get_request(host, command, NULL, cookies, 1, token);
        printf("GET REQUEST:\n");
        printf("%s\n", message);
    }
    else if (strcmp(get_delete, "delete") == 0)
    {
        message = compute_delete_request(host, command, NULL, NULL, 0, token);
        printf("DELETE REQUEST:\n");
        printf("%s\n", message);
    }
    else
    {
        // handle invalid method
        return NULL;
    }
    send_to_server(socket, message);
    return receive_from_server(socket);
}

char *compute_get_request(char *host, char *url, char *query_params,
                          char **cookies, int cookies_count, char *jwtoken)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL, request params (if any) and protocol type
    if (query_params != NULL)
    {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    }
    else
    {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Step 3 (optional): add headers and/or cookies, according to the protocol format
    //mai intai jwt ig
    if (jwtoken != NULL)
    {
        sprintf(line, "Authorization: Bearer %s", jwtoken);
        compute_message(message, line);
    }
    if (cookies != NULL)
    {
        sprintf(line, "Cookie: %s", cookies[0]);
        char *temp = calloc(LINELEN, sizeof(char));
        for (int i = 1; i < cookies_count; i++)
        {
            sprintf(temp, "; %s", cookies[i]);
            strcat(line, temp);
        }
        free(temp);
    }
    compute_message(message, line);
    free(line);
    // Step 4: add final new line
    compute_message(message, "");
    return message;
}

char *compute_post_request(char *host, char *url, char *content_type, char **body_data,
                           int body_data_fields_count, char **cookies, int cookies_count, char *jwt_token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *body_data_buffer = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);

    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);
    /* Step 3: add necessary headers (Content-Type and Content-Length are mandatory)
            in order to write Content-Length you must first compute the message size
    */
     if (jwt_token != NULL)
    {
        sprintf(line, "Authorization: Bearer %s", jwt_token);
        compute_message(message, line);
    }
    int len = 0;
    for (int i = 0; i < body_data_fields_count; i++)
    {
        strcat(body_data_buffer, body_data[i]);
        len += strlen(body_data[i]);
    }
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);
    sprintf(line, "Content-Length: %d", len);
    compute_message(message, line);
    // Step 4 (optional): add cookies
    if (cookies != NULL)
    {
        sprintf(line, "Cookie: %s", cookies[0]);
        char *temp = calloc(LINELEN, sizeof(char));

        for (int i = 1; i < cookies_count; i++)
        {
            sprintf(temp, "; %s", cookies[i]);
            strcat(line, temp);
        }

        free(temp);
        compute_message(message, line);
    }
    // Step 5: add new line at end of header
    compute_message(message, "");
    // Step 6: add the actual payload data
    memset(line, 0, LINELEN);
    strcat(message, body_data_buffer);

    free(line);
    free(body_data_buffer);
    return message;
}
char *compute_delete_request(char *host, char *url, char *query_params,
                            char **cookies, int cookies_count, char *jwtoken)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    //Step1 : scriem URL, protocol etc
   if (query_params != NULL)
    {
        sprintf(line, "DELETE %s?%s HTTP/1.1", url, query_params);
    }
    else
    {
        sprintf(line, "DELETE %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Step 2: add the host
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Step 3 (optional): add headers and/or cookies, according to the protocol format
     if (jwtoken != NULL)
    {
        sprintf(line, "Authorization: Bearer %s", jwtoken);
        compute_message(message, line);
    }
    if (cookies != NULL)
    {
        sprintf(line, "Cookie: %s", cookies[0]);
        char *temp = calloc(LINELEN, sizeof(char));
        for (int i = 1; i < cookies_count; i++)
        {
            sprintf(temp, "; %s", cookies[i]);
            strcat(line, temp);
        }
        free(temp);
        compute_message(message, line);
    }
    compute_message(message, line);
    free(line);

    // Step 4: add final new line
    compute_message(message, "");
    return message;
}