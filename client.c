#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* sockfd, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <stdbool.h>
#include <ctype.h>

#include "helpers.h"
#include "requests.h"
#include "parson.h"

bool manage_input(char *str)
{
    if (strcmp(str, "") == 0)
        return false;
    return true;
}

int main(int argc, char *argv[])
{
    int connected = 0, in_library = 0, exit_flag = 0;
    char host[16] = "34.246.184.49";
    int port = 8080;
    int sockfd;
    int ok;

    char *user[1], *cookies[1], token[BUFLEN], *addbook[1];
    char command[BUFLEN];

    while (!exit_flag)
    {
        fgets(command, BUFLEN, stdin);

        if (strncmp(command, "register", 8) == 0 || strncmp(command, "login", 5) == 0)
        {
            // Deschide conexiunea
            char *username = calloc(BUFLEN, sizeof(char));
            char *password = calloc(BUFLEN, sizeof(char));

            printf("username=");
            fgets(username, BUFLEN, stdin);
            username[strcspn(username, "\n")] = 0;
            printf("password=");
            fgets(password, BUFLEN, stdin);
            password[strcspn(password, "\n")] = 0;

            if (strchr(username, ' ') || strchr(password, ' '))
            {
                printf("ERROR! Invalid user or password. No whitespaces please!\n");
                continue;
            }

            JSON_Value *value = json_value_init_object();
            JSON_Object *object = json_value_get_object(value);
            json_object_set_string(object, "username", username);
            json_object_set_string(object, "password", password);
            user[0] = json_serialize_to_string(value);

            if (strncmp(command, "register", 8) == 0)
            { // register

                sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);

                // char *message = compute_post_request(host, command, "application/json", user, 1, NULL, 0, token);
                // send_to_server(sockfd, message);
                // char *response = receive_from_server(sockfd);
                char *response = recv_post_req(sockfd, host, REGISTER, user, NULL);

                close_connection(sockfd);
                printf("%s\n", response);
                if (strstr(response, "HTTP/1.1 2"))
                {
                    printf("SUCCESS! You are now registered.\n");
                }
                else
                {
                    printf("ERROR! creating the account.\n");
                }
            }
            else
            { // login
                if (connected)
                {
                    printf("ERROR! already logged in.\n");
                }
                else
                {
                    sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);

                    char *response = recv_post_req(sockfd, host, LOGIN, user, NULL);

                    close_connection(sockfd);
                    char *cookie = strstr(response, "Set-Cookie: ");
                    if (cookie == NULL)
                    {
                        printf("ERROR! logging in!\n");
                        connected = 0;
                        in_library = 0;
                        // close_connection(sockfd);
                        continue;
                    }
                    strtok(cookie, ";");
                    cookie += 12;
                    cookies[0] = cookie;
                    if (cookie != NULL)
                    {
                        printf("SUCCESS! You are now logged in!\n");
                        connected = 1;
                    }
                }
            }

            free(username);
            free(password);
           // close_connection(sockfd);
        }
        else if (strncmp(command, "logout", 6) == 0)
        {
            if (connected == 1)
            {
                // Deschide conexiunea
                sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);

                connected = 0;
                in_library = 0;
                recv_get_req(sockfd, host, LOGOUT, token, cookies, "get");
                printf("SUCCESS!\n");

                close_connection(sockfd);
            }
            else
            {
                printf("ERROR!: You are not logged in!\n");
            }
        }
        else if (strncmp(command, "enter_library", 13) == 0)
        {
            if (connected && !in_library)
            {
                // Deschide conexiunea
                sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);

                char *tok = strstr(recv_get_req(sockfd, host, ACCESS, token, cookies, "get"), "token");
                if (tok == NULL)
                {
                    printf("ERROR!: NO ACCESS IN LIBRARY!\n");
                }
                else
                {
                    tok += 8;
                    memset(token, 0, BUFLEN);
                    strcpy(token, tok);
                    token[strlen(token) - 2] = '\0';
                    in_library = 1;
                    printf("SUCCESS!\n");
                }

                close_connection(sockfd);
            }
            else if (!connected)
            {
                printf("ERROR!: You are not logged in.\n");
            }
            else
            {
                printf("You are already in the library.\n");
            }
        }
        else if (strncmp(command, "get_books", 9) == 0)
        {
            if (in_library == 1)
            {
                // Deschide conexiunea
                sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);

                char *res = recv_get_req(sockfd, host, BOOKS, token, cookies, "get");
                printf("%s\n", strstr(res, "["));

                close_connection(sockfd);
            }
            else
            {
                printf("ERROR!: You are not in the library.\n");
            }
        }
        else if (strncmp(command, "get_book", 8) == 0)
        {
            if (in_library == 1)
            {
                char route[BUFLEN], bok[BUFLEN];
                int book_id = 0;

                printf("id=");
                scanf("%s", bok);
                book_id = atoi(bok);
                if (book_id <= 0)
                {
                    printf("ERROR!: WRONG FORMAT! Try again!\n");
                    continue;
                }

                sprintf(route, "%s/%d", BOOKS, book_id);

                // Deschide conexiunea
                sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);

                char *res = recv_get_req(sockfd, host, route, token, cookies, "get");
                printf("%s\n", res);
                if (strstr(res, "HTTP/1.1 404 Not Found") != NULL)
                {
                    printf("ERROR!: NO BOOK WAS FOUND!\n");
                }
                else
                {
                    printf("%s\n", strstr(res, "["));
                }

                close_connection(sockfd);
            }
            else
            {
                printf("ERROR!: You are not in the library.\n");
            }
        }
        else if (strncmp(command, "add_book", 8) == 0)
        {
            if (in_library == 1)
            {
                char titlu[BUFLEN], auth[BUFLEN], genre[BUFLEN], publisher[BUFLEN], pagenum[BUFLEN];
                int pages;
                bool res;

                printf("title=");
                fgets(titlu, LINELEN, stdin);
                titlu[strcspn(titlu, "\n")] = 0;
                res = manage_input(titlu);
                if (!res)
                {
                    printf("ERROR ! INPUT GREȘIT! TRY AGAIN\n");
                    continue;
                }

                printf("author=");
                fgets(auth, LINELEN, stdin);
                auth[strcspn(auth, "\n")] = 0;
                res = manage_input(auth);
                if (!res)
                {
                    printf("ERROR ! INPUT GREȘIT! TRY AGAIN\n");
                    continue;
                }

                printf("genre=");
                fgets(genre, LINELEN, stdin);
                genre[strcspn(genre, "\n")] = 0;
                res = manage_input(genre);
                if (!res)
                {
                    printf("ERROR ! INPUT GREȘIT! TRY AGAIN\n");
                    continue;
                }

                printf("publisher=");
                fgets(publisher, LINELEN, stdin);
                publisher[strcspn(publisher, "\n")] = 0;
                res = manage_input(publisher);
                if (!res)
                {
                    printf("ERROR ! INPUT GREȘIT! TRY AGAIN\n");
                    continue;
                }

                printf("page_count=");
                fgets(pagenum, LINELEN, stdin);
                pagenum[strcspn(pagenum, "\n")] = 0;
                res = manage_input(pagenum);
                if (!res)
                {
                    printf("ERROR ! INPUT GREȘIT! TRY AGAIN\n");
                    continue;
                }

                pages = atoi(pagenum);
                if (pages <= 0)
                {
                    printf("ERROR! : Tip de date incorect pentru numărul de pagini\n");
                    continue;
                }

                JSON_Value *value = json_value_init_object();
                JSON_Object *object = json_value_get_object(value);
                json_object_set_string(object, "title", titlu);
                json_object_set_string(object, "author", auth);
                json_object_set_string(object, "genre", genre);
                json_object_set_number(object, "page_count", pages);
                json_object_set_string(object, "publisher", publisher);
                addbook[0] = json_serialize_to_string_pretty(value);

                // Deschide conexiunea
                sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);

                recv_post_req(sockfd, host, BOOKS, addbook, token);
                printf("SUCCESS!\n");

                close_connection(sockfd);
            }
            else
            {
                printf("ERROR!: You are not in the library.\n");
            }
        }
        else if (strncmp(command, "delete_book", 11) == 0)
        {
            if (in_library == 1)
            {
                char route[BUFLEN], bok[BUFLEN];
                int book_id = 0;

                printf("id=");
                fgets(bok, BUFLEN, stdin);
                bok[strcspn(bok, "\n")] = 0;
                book_id = atoi(bok);
                if (book_id <= 0)
                {
                    printf("ERROR!: WRONG FORMAT! Try again!\n");
                    continue;
                }

                sprintf(route, "%s/%d", BOOKS, book_id);

                // Deschide conexiunea
                sockfd = open_connection(host, port, AF_INET, SOCK_STREAM, 0);

                char *deleted = strstr(recv_get_req(sockfd, host, route, token, cookies, "delete"), "No book was deleted!");
                if (deleted != NULL)
                {
                    printf("ERROR!: NO BOOK! Entered id is not valid!\n");
                }
                else
                {
                    printf("SUCCESS!\n");
                }

                close_connection(sockfd);
            }
            else
            {
                printf("ERROR!: You are not in the library.\n");
            }
        }
        else if (strncmp(command, "exit", 4) == 0)
        {
            exit_flag = 1;
        }
    }

    return 0;
}
