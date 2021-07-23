#include <strings.h>
#include <pthread.h>
#include <sqlite3.h>
#include "common.h"
#include "myqueue.h"


pthread_t thread_pool[THREAD_POOL_SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


int DBNum = 0;
int terminate = 0;

char* tempData = "";

int wentThrough2 = 0;


// sql stuff
sqlite3* DB;
char* messaggeError;
char* query = "SELECT * FROM PERSON;";



uint8_t recvline[MAXLINE+1];
uint8_t buff[MAXLINE+1];

void *handle_connection(void *client_socket);

void * thread_function(void *arg);

static int callback(void* data, int argc, char** argv, char** azColName)
{
    int i;
    fprintf(stderr, "%s: ", (const char*)data);
    
    for (i = 0; i < argc; i++) {
        char* tem = argv[i] ? argv[i] : "NULL";
        printf("%s = %s\n", azColName[i], tem);
        if (i == 0) {
            DBNum =  strtol(tem, NULL, 10);
        }
    }
    
    printf("\n");
    return 0;
}

static int callback2(void* data, int argc, char** argv, char** azColName)
{
    int i;
    fprintf(stderr, "%s: ", (const char*)data);
    
    for (i = 0; i < argc; i++) {
        char* tem = argv[i] ? argv[i] : "NULL";
        printf("%s = %s\n", azColName[i], tem);
        
        
        tempData = malloc(strlen(tem) + 1);
        strcat(tempData, tem);
        
        wentThrough2 = 1;
    }
    
    printf("\n");
    return 0;
}

int main(int argc, char **argv) {
    
    // initial database interaction
    int exit = 0;

    
    char sql[] = "CREATE TABLE PERSON("
    "ID INT PRIMARY KEY     NOT NULL, "
    "NAME           TEXT    NOT NULL, "
    "Q1           TEXT     NOT NULL, "
    "Q2           TEXT     NOT NULL, "
    "Q3           TEXT     NOT NULL, "
    "DATE           TEXT     NOT NULL, "
    "TIME           TEXT     NOT NULL, "
    "CANCELLED           INT     NOT NULL);";
    exit = sqlite3_open("example.db", &DB);
    exit = sqlite3_exec(DB, sql, NULL, 0, &messaggeError);
    
    // if the database already exists, then just send a message
    if (exit != SQLITE_OK) {
        printf("Error Create Table\n");
        sqlite3_free(messaggeError);
    }
    
    // if it doesn't exist, then insert default data
    else{
        printf("Table created Successfully\n");
        char sql2[] = "INSERT INTO PERSON VALUES(0, 'NA', 'N', 'N', 'N', '0000000', '00PM', 0);";
        
        exit = 0;
        exit = sqlite3_open("example.db", &DB);
        exit = sqlite3_exec(DB, sql2, NULL, 0, &messaggeError);
        
        if (exit != SQLITE_OK) {
            printf("Error Insert\n");
            sqlite3_free(messaggeError);
        }
        else{
            printf("Inserted Successfully\n");
        }
    }
    
    sqlite3_exec(DB, query, callback, NULL, NULL);
    
    printf("%d\n", DBNum);
    
    
    // dealing with threads
    int listenfd,connfd;
    struct sockaddr_in servaddr;
    
    
    for(int i=0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&thread_pool[i], NULL, thread_function, NULL);
    }
    
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err_n_die("socket error.");
    
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_PORT);
    
    if((bind(listenfd, (SA *) &servaddr, sizeof(servaddr))) < 0)
        err_n_die("bind error.");
    
    if((listen(listenfd, 10)) < 0)
        err_n_die("listen error.");
    
    for( ; ;) {
        struct sockaddr_in addr;
        socklen_t addr_len;
        char client_address[MAXLINE+1];
        
        
        printf("waiting for a connection on port %d\n", SERVER_PORT);
        fflush(stdout);
        connfd = accept(listenfd, (SA *) &addr, &addr_len);
        
        inet_ntop(AF_INET, &addr, client_address, MAXLINE);
        printf("Client connection: %s\n", client_address);
        
        
        int *pclient = malloc(sizeof(int));
        *pclient = connfd;
        pthread_mutex_lock(&mutex);
        enqueue(pclient);
        pthread_mutex_unlock(&mutex);
        // pthread_create(&t, NULL, handle_connection, pclient);
        
        // is terminate button is initiated, stop running server
        if (terminate == 1) {
            break;
        }
        
    }
    sqlite3_close(DB);
    return 0;
}
void * thread_function(void *arg) {
    while(1) {
        pthread_mutex_lock(&mutex);
        int *pclient = dequeue();
        pthread_mutex_unlock(&mutex);
        if(pclient != NULL) {
            handle_connection(pclient);
        }
    }
}
    
void *handle_connection(void *client_socket) {
    int n;
        
    int connfd = *((int*) client_socket);
    free(client_socket);
    while ((n = read(connfd, recvline, MAXLINE-1)) > 0) {
        fprintf(stdout, "\n%s\n\n%s", bin2hex(recvline, n), recvline);
            
        if(recvline[n-1] == '\n') {
            break;
        }
        memset(recvline, 0, MAXLINE);
    }
    
    if (n < 0) {
            err_n_die("read error");
    }

        
    if (strstr((const char*)recvline, "favicon.ico") != NULL) {
            // contains
            
            
    }else if(strstr((const char*)recvline, "GET /?name") != NULL) {
            char webpage[20000];
            
            char *req = (char*)recvline;
            char delim[] = "=";
            char delim2[] = " ";
            
            char *tempptr = strtok(req, delim);
            tempptr = strtok(NULL, delim2);
            
            
            char *name = tempptr;
            
            char *t;
            char tt[strlen(name)];
            if (strstr(name, "+") != NULL) {
                int i = 0;
                for (t = name; *t != '\0'; t++) {
                    if(*t == '+') {
                        tt[i] = ' ';
                    }
                    else {
                        tt[i] = *t;
                    }
                    i++;
                }
                name = tt;
            }
            
            sprintf(webpage, "HTTP/1.0 200 OK\r\n\r\n"
                    "<!DOCTYPE html>\r\n"
                    "<html>\r\n"
                    "<head>\r\n"
                    "<title>Coronavirus ChatBot Scheduler</title>\r\n"
                    "<style>\r\n"
                    "body {background-color: powderblue;"
                    "text-align: center;"
                    "border: 3px solid blue;}\r\n"
                    "form {display: inline-block;}\r\n"
                    "</style>\r\n"
                    "</head>\r\n"
                    "<body>\r\n"
                    "Chatbot: <p id=\"cb\">Hello, %s. I would like to ask you some questions. Have you been outside your state for the past week?</p>\r\n"
                    "%s:<p\"pat\"></p>\r\n"
                    "<input id=\"resp\">\r\n"
                    "<form>\r\n"
                    "<input style=\"display:none\" id=\"am\" name=\"ap\" type=\"radio\">AM<br>\r\n"
                    "<input style=\"display:none\" id=\"pm\" name=\"ap\" type=\"radio\">PM\r\n"
                    "</form>\r\n"
                    "<button id=\"enter\" onClick=\"myFunction()\">Enter</button>\r\n"
                    "<form method=\"GET\">\r\n"
                    "<input style=\"display:none\" id=\"q1\" type=\"text\" name=\"q1\"/>\r\n"
                    "<input style=\"display:none\" id=\"q2\" type=\"text\" name=\"q2\"/>\r\n"
                    "<input style=\"display:none\" id=\"q3\" type=\"text\" name=\"q3\"/>\r\n"
                    "<input style=\"display:none\" id=\"date\" type=\"text\" name=\"date\"/>\r\n"
                    "<input style=\"display:none\" id=\"time\" type=\"text\" name=\"time\"/>\r\n"
                    "<input style=\"display:none\" id=\"name\" type=\"text\" name=\"name\"/>\r\n"
                    "<input style=\"display:none\" id=\"next\" type=\"submit\" value=\"Next\"/>\r\n"
                    "</form>\r\n"
                    "<form method=\"GET\">\r\n"
                    "<input type=\"submit\" value=\"Back to Menu\" />\r\n"
                    "</form>\r\n"
                    "<script type=\"text/javascript\">\r\n"
                    "var i = 0;\r\n"
                    "var mess = [\"Have you been feeling sick/feverish in general recently?\", \"Have you had cold/flu like symptoms recently?\", \"What date would you like your appointment? (format: mmddyyyy)\", \"What time would you like on that day? (format ex: 10 AM or 9 PM)\", \"Thank you for your time! Have a nice day!\"];\r\n"
                    "var apprResp = [\"yes\", \"no\", \"y\", \"n\"];\r\n"
                    "function myFunction() {\r\n"
                    "document.getElementById(\"name\").value = \"%s\";\r\n"
                    "if (i <= 0) {\r\n"
                    "if (checkValid(document.getElementById(\"resp\").value)) {\r\n"
                    "document.getElementById(\"q1\").value = document.getElementById(\"resp\").value; \r\n"
                    "document.getElementById(\"cb\").innerHTML = mess[i];\r\n"
                    "i++\r\n"
                    "}\r\n"
                    "}\r\n"
                    "else if (i == 1) {\r\n"
                    "if (checkValid(document.getElementById(\"resp\").value)) {\r\n"
                    "document.getElementById(\"q2\").value = document.getElementById(\"resp\").value; \r\n"
                    "document.getElementById(\"cb\").innerHTML = mess[i];\r\n"
                    "i++\r\n"
                    "}\r\n"
                    "}\r\n"
                    "else if (i == 2) {\r\n"
                    "if (checkValid(document.getElementById(\"resp\").value)) {\r\n"
                    "document.getElementById(\"q3\").value = document.getElementById(\"resp\").value; \r\n"
                    "document.getElementById(\"cb\").innerHTML = mess[i];\r\n"
                    "i++\r\n"
                    "}\r\n"
                    "}\r\n"
                    "else if (i == 3) {\r\n"
                    "if (checkValid3(document.getElementById(\"resp\").value)) {\r\n"
                    "document.getElementById(\"date\").value = document.getElementById(\"resp\").value; \r\n"
                    "document.getElementById(\"cb\").innerHTML = mess[i];\r\n"
                    "i++\r\n"
                    "document.getElementById(\"am\").style.display = \"block\";\r\n"
                    "document.getElementById(\"pm\").style.display = \"block\";\r\n"
                    "}\r\n"
                    "}\r\n"
                    "else {\r\n"
                    "var wu = document.getElementById(\"am\")\r\n"
                    "var vu = document.getElementById(\"pm\")\r\n"
                    "if (checkValid2(document.getElementById(\"resp\").value, wu, vu)) {\r\n"
                    "document.getElementById(\"time\").value = document.getElementById(\"resp\").value; \r\n"
                    "if(wu.checked === true) {\r\n"
                    "document.getElementById(\"time\").value += \"AM\"\r\n"
                    "}\r\n"
                    "else {\r\n"
                    "document.getElementById(\"time\").value += \"PM\"\r\n"
                    "}\r\n"
                    "document.getElementById(\"cb\").innerHTML = mess[i];\r\n"
                    "document.getElementById(\"enter\").disabled = true;\r\n"
                    "document.getElementById(\"am\").style.display = \"none\";\r\n"
                    "document.getElementById(\"pm\").style.display = \"none\";\r\n"
                    "document.getElementById(\"next\").style.display = \"block\";\r\n"
                    "}\r\n"
                    "}\r\n"
                    "} \r\n"
                    "function checkValid(x) {\r\n"
                    "for(var j = 0; j < apprResp.length; j++) {\r\n"
                    "if(x.toLowerCase() === apprResp[j]) {\r\n"
                    "return true;\r\n"
                    "}\r\n"
                    "}\r\n"
                    "alert(\"Invalid input!\");\r\n"
                    "return false;\r\n"
                    "}\r\n"
                    "function checkValid2(y, w, v) {\r\n"
                    "if(y.length <= 2 && y.length > 0) {\r\n"
                    "if(y.length === 2 && y.charAt(0) === '0') {\r\n"
                    "alert(\"Invalid input!\");\r\n"
                    "return false;\r\n"
                    "}\r\n"
                    "if((w.checked === false && v.checked == false) || parseInt(y) > 12) {\r\n"
                    "alert(\"Invalid input!\");\r\n"
                    "return false;\r\n"
                    "}\r\n"
                    "return true;\r\n"
                    "}\r\n"
                    "else {\r\n"
                    "alert(\"Invalid input!\");\r\n"
                    "return false;\r\n"
                    "}\r\n"
                    "}\r\n"
                    "function checkValid3(z) {\r\n"
                    "if(z.length === 8) {\r\n"
                    "return true;\r\n"
                    "}\r\n"
                    "else {\r\n"
                    "alert(\"Invalid input!\");\r\n"
                    "return false;\r\n"
                    "}\r\n"
                    "}\r\n"
                    "</script>\r\n"
                    "</body>\r\n"
                    "</html>\r\n", name, name, name);
            
            
            snprintf((char*)buff, sizeof(buff), "%s", webpage);
            
            write(connfd, (char*) buff, strlen((char *)buff));
            close(connfd);
            // strcpy(webpage, "");
    }else if (strstr((const char*)recvline,"GET /?q1=") != NULL) {
            char *req = (char*)recvline;
        
            char delim[] = "=";
           char delim2[] = " ";
           char delim3[] = "&";

           
            char *tempptr = strtok(req, delim);
           tempptr = strtok(NULL, delim3);
           
           char *q1 = tempptr;
           
           tempptr = strtok(NULL, delim);
           tempptr = strtok(NULL, delim3);
           
           char *q2 = tempptr;
           
           tempptr = strtok(NULL, delim);
           tempptr = strtok(NULL, delim3);
           
           char *q3 = tempptr;
           
           tempptr = strtok(NULL, delim);
           tempptr = strtok(NULL, delim3);
           
           char *date = tempptr;
           
           tempptr = strtok(NULL, delim);
           tempptr = strtok(NULL, delim3);
           
           char *time = tempptr;
           
           tempptr = strtok(NULL, delim);
           tempptr = strtok(NULL, delim2);
           
           char *name = tempptr;
            char *t;
            char tt[strlen(name)];
           if (strstr(name, "+") != NULL) {
                int i = 0;
                for (t = name; *t != '\0'; t++) {
                    if(*t == '+') {
                        tt[i] = ' ';
                    }
                    else {
                        tt[i] = *t;
                    }
                    i++;
                }
                name = tt;
            }
           

            char altmes[50];
            
            if (DBNum == 0){
         
                char sql2[100];
                
                DBNum++;
                
                sprintf(sql2,"INSERT INTO PERSON VALUES(%d, '%s', '%s', '%s', '%s', '%s', '%s', %d);", DBNum, name, q1, q2, q3, date, time, 0);
                
                // exit = 0;
                int exit2 = 0;
                exit2 = sqlite3_open("example.db", &DB);
                exit2 = sqlite3_exec(DB, sql2, NULL, 0, &messaggeError);
                
                if (exit2 != SQLITE_OK) {
                    printf("Error Insert\n");
                    sqlite3_free(messaggeError);
                } else{
                    printf("Inserted Successfully\n");
                }
                
                strcpy(altmes, "");
           } else {
                
                char query3[100];
               
               wentThrough2 = 0;
                
                sprintf(query3, "SELECT NAME FROM PERSON WHERE DATE = '%s' AND TIME = '%s' AND CANCELLED = 0;", date, time);
                
                sqlite3_exec(DB, query3, callback2, NULL, NULL);
                
                if(wentThrough2 == 1) {
                    printf("%s\n", tempData);
                    wentThrough2 = 0;
                
                    char webpage[] = "HTTP/1.0 200 OK\r\n\r\n"
                    "<!DOCTYPE html>\r\n"
                    "<html>\r\n"
                    "<head>\r\n"
                    "<title>Coronavirus ChatBot Scheduler</title>\r\n"
                    "<style>\r\n"
                    "body {background-color: powderblue;}\r\n"
                    "</style>\r\n"
                    "</head>\r\n"
                    "<body>\r\n"
                    "<h1>Sorry...</h1>\r\n"
                    "<p>Sorry, but this time has already been taken. Please login with your name and try again.</p>\r\n"
                    "<form method=\"GET\">\r\n"
                    "<input id=\"get\" type=\"submit\" value=\"Back to Menu\" />\r\n"
                    "</form>\r\n"
                    "</body>\r\n"
                    "</html>\r\n";
                
                    snprintf((char*)buff, sizeof(buff), "%s", webpage);
                
                    write(connfd, (char*) buff, strlen((char *)buff));
                    close(connfd);
                
                    return NULL;
                
                
                }else {
                    printf("Not found!\n");
                    char query4[100];
                
                    sprintf(query4, "SELECT CANCELLED FROM PERSON WHERE NAME = '%s';", name);
                
                    sqlite3_exec(DB, query4, callback2, NULL, NULL);
                
                    if(wentThrough2 == 1) {
                        wentThrough2 = 0;
                        if (strstr((const char*)tempData,"1") != NULL) {
                
                            char sql3[100];
                            sprintf(sql3, "UPDATE PERSON SET CANCELLED = 0, Q1 = '%s', Q2 = '%s', Q3 = '%s', DATE = '%s', TIME = '%s' WHERE NAME = '%s';", q1, q2, q3, date, time, name);
                
                            int exit2 = 0;
                            exit2 = sqlite3_open("example.db", &DB);
                            exit2 = sqlite3_exec(DB, sql3, NULL, 0, &messaggeError);
                
                            if (exit2 != SQLITE_OK) {
                                printf("Error Update\n");
                                sqlite3_free(messaggeError);
                            }
                            else{
                                printf("Updated Successfully\n");
                            }
                
                            strcpy(altmes, "");
                        }
                        else {
                            char sql3[100];
                            sprintf(sql3, "UPDATE PERSON SET Q1 = '%s', Q2 = '%s', Q3 = '%s', DATE = '%s', TIME = '%s' WHERE NAME = '%s';", q1, q2, q3, date, time, name);
                
                            int exit2 = 0;
                            exit2 = sqlite3_open("example.db", &DB);
                            exit2 = sqlite3_exec(DB, sql3, NULL, 0, &messaggeError);
                
                            if (exit2 != SQLITE_OK) {
                                printf("Error Update\n");
                                sqlite3_free(messaggeError);
                            }
                            else{
                                printf("Updated Successfully\n");
                            }
                
                            strcpy(altmes, "");
                
                            strcpy(altmes, "You have sucessfully rescheduled.");
                        }
                    } else {
                        printf("Not found!\n");
                
                        char sql3[100];
                
                        DBNum++;
                
                        sprintf(sql3,"INSERT INTO PERSON VALUES(%d, '%s', '%s', '%s', '%s', '%s', '%s', 0);", DBNum, name, q1, q2, q3, date, time);
                
                        int exit2 = 0;
                        exit2 = sqlite3_open("example.db", &DB);
                        exit2 = sqlite3_exec(DB, sql3, NULL, 0, &messaggeError);
                
                        if (exit2 != SQLITE_OK) {
                        printf("Error Insert\n");
                        sqlite3_free(messaggeError);
                        } else {
                        printf("Inserted Successfully\n");
                        }
                
                        strcpy(altmes, "");
                        
                        
                    }
                
                }
           }
               char webpage[10000];
               sprintf(webpage,"HTTP/1.0 200 OK\r\n\r\n"
                       "<!DOCTYPE html>\r\n"
                       "<html>\r\n"
                       "<head>\r\n"
                       "<title>Coronavirus ChatBot Scheduler</title>\r\n"
                       "<style>\r\n"
                       "body {background-color: powderblue;}\r\n"
                       "</style>\r\n"
                       "</head>\r\n"
                       "<body>\r\n"
                       "<h1>Thank you %s for your time.</h1>\r\n"
                       "<p>%s We will be seeing you at %s on %s.</p>\r\n"
                       "<form method=\"GET\">\r\n"
                       "<input id=\"get\" type=\"submit\" value=\"Back to Menu\" />\r\n"
                       "</form>\r\n"
                       "</body>\r\n"
                       "</html>\r\n", name, altmes, time, date);
               
               snprintf((char*)buff, sizeof(buff), "%s", webpage);
               
               write(connfd, (char*) buff, strlen((char *)buff));
               close(connfd);
                
               
           
        }else if (strstr((const char*)recvline, "POST /") != NULL){
            
            
               char webpage[] = "HTTP/1.0 200 OK\r\n\r\n"
                    "<!DOCTYPE html>\r\n"
                    "<html>\r\n"
                    "<head>\r\n"
                    "<title>Coronavirus ChatBot Scheduler</title>\r\n"
                    "<style>\r\n"
                    "body {background-color: powderblue;}\r\n"
                    "</style>\r\n"
                    "</head>\r\n"
                    "<body>\r\n"
                    "<h1>Cancel or Check Appointment</h1>\r\n"
                    "<p>Type your name and we'll see if we can check or cancel appointment.</p>\r\n"
                    "<form method=\"GET\">\r\n"
                    "<label for=\"choice\">Choose either check or cancel:</label>\r\n"
                    "<select name=\"choice\" id=\"choice\">\r\n"
                    "<option value=\"check\">Check</option>\r\n"
                    "<option value=\"cancel\">Canel</option>\r\n"
                    "</select>\r\n"
                    "<br><br>\r\n"
                    "<input type=\"text\" name=\"name\"/>\r\n"
                    "<input id=\"get\" type=\"submit\" value=\"Next\" />\r\n"
                    "</form>\r\n"
                    "<form method=\"GET\">\r\n"
                    "<input id=\"get\" type=\"submit\" value=\"Back to Menu\" />\r\n"
                    "</form>\r\n"
                    "</body>\r\n"
                    "</html>\r\n";
            
               snprintf((char*)buff, sizeof(buff), "%s", webpage);
            
               write(connfd, (char*) buff, strlen((char *)buff));
               close(connfd);
            
        
            
        } else if (strstr((const char*)recvline,"GET /?choice=cancel") != NULL) {
            
            char webpage[10000];
            char *req = (char*)recvline;
            char delim[] = "=";
            char delim2[] = " ";
            char delim3[] = "&";
            
            char *tempptr = strtok(req, delim);
            tempptr = strtok(NULL, delim3);
            tempptr = strtok(NULL, delim);
            tempptr = strtok(NULL, delim2);
            
            
            char *name = tempptr;
            char *t;
            char tt[strlen(name)];
            if (strstr(name, "+") != NULL) {
                int i = 0;
                for (t = name; *t != '\0'; t++) {
                    if(*t == '+') {
                        tt[i] = ' ';
                    }
                    else {
                        tt[i] = *t;
                    }
                    i++;
                }
                name = tt;
            }
            
             char query4[100];
             
             sprintf(query4, "SELECT CANCELLED FROM PERSON WHERE NAME = '%s';", name);
             
             sqlite3_exec(DB, query4, callback2, NULL, NULL);
             
            if (wentThrough2 == 0 || strstr((const char*)tempData,"1") != NULL) {
             
                if (wentThrough2 == 1) {
                    wentThrough2 = 0;
                }
                sprintf(webpage, "HTTP/1.0 200 OK\r\n\r\n"
                "<!DOCTYPE html>\r\n"
                "<html>\r\n"
                "<head>\r\n"
                "<title>Coronavirus ChatBot Scheduler</title>\r\n"
                "<style>\r\n"
                "body {background-color: powderblue;}\r\n"
                "</style>\r\n"
                "</head>\r\n"
                "<body>\r\n"
                "<h1>Appointment doesn't exist.</h1>\r\n"
                "<p>Sorry, %s, but it seems we didn't find an appointment under your name to cancel.</p>\r\n"
                "<form method=\"GET\">\r\n"
                "<input id=\"get\" type=\"submit\" value=\"Back to Menu\" />\r\n"
                "</form>\r\n"
                "</body>\r\n"
                "</html>\r\n", name);
                snprintf((char*)buff, sizeof(buff), "%s", webpage);
             
                write(connfd, (char*) buff, strlen((char *)buff));
                close(connfd);
             
                return NULL;
             } else {
                wentThrough2 = 0;
                char sql3[100];
                sprintf(sql3, "UPDATE PERSON SET CANCELLED = 1 WHERE NAME = '%s';", name);
             
                int exit2 = 0;
                exit2 = sqlite3_open("example.db", &DB);
                exit2 = sqlite3_exec(DB, sql3, NULL, 0, &messaggeError);
             
                if (exit2 != SQLITE_OK) {
                    printf("Error Update\n");
                    sqlite3_free(messaggeError);
                }
                else{
                    printf("Updated Successfully\n");
                }
             
             
             }
             
             char* time;
             char* date;
             
             sprintf(query4, "SELECT TIME FROM PERSON WHERE NAME = '%s';", name);
             
             sqlite3_exec(DB, query4, callback2, NULL, NULL);
             
             time = malloc(strlen(tempData) + 1);
             strcat(time, tempData);
             
             
             sprintf(query4, "SELECT DATE FROM PERSON WHERE NAME = '%s';", name);
             
             sqlite3_exec(DB, query4, callback2, NULL, NULL);
             
             date = malloc(strlen(tempData) + 1);
             strcat(date, tempData);
             
             
             sprintf(webpage, "HTTP/1.0 200 OK\r\n\r\n"
             "<!DOCTYPE html>\r\n"
             "<html>\r\n"
             "<head>\r\n"
             "<title>Coronavirus ChatBot Scheduler</title>\r\n"
             "<style>\r\n"
             "body {background-color: powderblue;}\r\n"
             "</style>\r\n"
             "</head>\r\n"
             "<body>\r\n"
             "<h1>Okay, %s...</h1>\r\n"
             "<p>You have sucessfully cancelled your appointment that was scheduled at %s on %s.</p>\r\n"
             "<form method=\"GET\">\r\n"
             "<input id=\"get\" type=\"submit\" value=\"Back to Menu\" />\r\n"
             "</form>\r\n"
             "</body>\r\n"
             "</html>\r\n", name, time, date);
             snprintf((char*)buff, sizeof(buff), "%s", webpage);
             
             write(connfd, (char*) buff, strlen((char *)buff));
             close(connfd);
            
             return NULL;

            
    }
    else if (strstr((const char*)recvline,"GET /?choice=check") != NULL) {
            
            char webpage[10000];
            char *req = (char*)recvline;
            char delim[] = "=";
            char delim2[] = " ";
            char delim3[] = "&";
            
            char *tempptr = strtok(req, delim);
            tempptr = strtok(NULL, delim3);
            tempptr = strtok(NULL, delim);
            tempptr = strtok(NULL, delim2);
            
            
            char *name = tempptr;
            char *t;
            char tt[strlen(name)];
            if (strstr(name, "+") != NULL) {
                int i = 0;
                for (t = name; *t != '\0'; t++) {
                    if(*t == '+') {
                        tt[i] = ' ';
                    }
                    else {
                        tt[i] = *t;
                    }
                    i++;
                }
                name = tt;
            }
            
            
             char query4[100];
             
             sprintf(query4, "SELECT CANCELLED FROM PERSON WHERE NAME = '%s';", name);
             
             sqlite3_exec(DB, query4, callback2, NULL, NULL);
             
             if (wentThrough2 == 0 || strstr((const char*)tempData,"1") != NULL) {
             
                if (wentThrough2 == 1) {
                    wentThrough2 = 0;
                }
             
                sprintf(webpage, "HTTP/1.0 200 OK\r\n\r\n"
                "<!DOCTYPE html>\r\n"
                "<html>\r\n"
                "<head>\r\n"
                "<title>Coronavirus ChatBot Scheduler</title>\r\n"
                "<style>\r\n"
                "body {background-color: powderblue;}\r\n"
                "</style>\r\n"
                "</head>\r\n"
                "<body>\r\n"
                "<h1>Sorry...</h1>\r\n"
                "<p>Sorry, %s, but you don't have an appointment. You can create one at the home page.</p>\r\n"
                "<form method=\"GET\">\r\n"
                "<input id=\"get\" type=\"submit\" value=\"Back to Menu\" />\r\n"
                "</form>\r\n"
                "</body>\r\n"
                "</html>\r\n", name);
                snprintf((char*)buff, sizeof(buff), "%s", webpage);
             
                write(connfd, (char*) buff, strlen((char *)buff));
                close(connfd);
             
                return NULL;
             
             
             } else {
                wentThrough2 = 0;
             
                char* time;
                char* date;
             
                sprintf(query4, "SELECT TIME FROM PERSON WHERE NAME = '%s';", name);
             
                sqlite3_exec(DB, query4, callback2, NULL, NULL);
             
                time = malloc(strlen(tempData) + 1);
                strcat(time, tempData);
             
             
                sprintf(query4, "SELECT DATE FROM PERSON WHERE NAME = '%s';", name);
             
                sqlite3_exec(DB, query4, callback2, NULL, NULL);
             
                date = malloc(strlen(tempData) + 1);
                strcat(date, tempData);
             
                sprintf(webpage, "HTTP/1.0 200 OK\r\n\r\n"
                "<!DOCTYPE html>\r\n"
                "<html>\r\n"
                "<head>\r\n"
                "<title>Coronavirus ChatBot Scheduler</title>\r\n"
                "<style>\r\n"
                "body {background-color: powderblue;}\r\n"
                "</style>\r\n"
                "</head>\r\n"
                "<body>\r\n"
                "<h1>Okay, %s...</h1>\r\n"
                "<p>Your appointment is at %s on %s.</p>\r\n"
                "<form method=\"GET\">\r\n"
                "<input id=\"get\" type=\"submit\" value=\"Back to Menu\" />\r\n"
                "</form>\r\n"
                "</body>\r\n"
                "</html>\r\n", name, time, date);
                snprintf((char*)buff, sizeof(buff), "%s", webpage);
             
                write(connfd, (char*) buff, strlen((char *)buff));
                close(connfd);
             
                return NULL;
             
             }
            
                    
                    
    } else if (strstr((const char*)recvline,"GET /?term=") != NULL) {
            
            terminate = 1;
            
            char webpage[] = "HTTP/1.0 200 OK\r\n\r\n"
            "<!DOCTYPE html>\r\n"
            "<html>\r\n"
            "<head>\r\n"
            "<title>Coronavirus ChatBot Scheduler</title>\r\n"
            "<style>\r\n"
            "body {background-color: powderblue;}\r\n"
            "</style>\r\n"
            "</head>\r\n"
            "<body>\r\n"
            "<h1>Server is terminated...</h1>\r\n"
            "<p>Thank you for trying out the server.</p>\r\n"
            "</body>\r\n"
            "</html>\r\n";
            snprintf((char*)buff, sizeof(buff), "%s", webpage);
            
            write(connfd, (char*) buff, strlen((char *)buff));
            close(connfd);
            
        
    } else{
            char webpage[] = "HTTP/1.0 200 OK\r\n\r\n"
            "<!DOCTYPE html>\r\n"
            "<html>\r\n"
            "<head>\r\n"
            "<title>Coronavirus ChatBot Scheduler</title>\r\n"
            "<style>\r\n"
            "body {background-color: powderblue;}\r\n"
            "</style>\r\n"
            "</head>\r\n"
            "<body>\r\n"
            "<h1>Coronavirus ChatBot Scheduler</h1>\r\n"
            "<p>If you want to set up an appointment for a COVID-19 test, please log in your full name.</p>\r\n"
            "<form method=\"GET\">\r\n"
            "<input type=\"text\" name=\"name\"/>\r\n"
            "<input id=\"get\" type=\"submit\" value=\"submit\" />\r\n"
            "</form>\r\n"
            "<br><br>\r\n"
            "<form method=\"POST\" enctype=\"text/plain\">\r\n"
            "<input id=\"get\" type=\"submit\" value=\"Cancel or Check Appointment\" />\r\n"
            "</form>\r\n"
            "<form method=\"GET\" enctype=\"text/plain\">\r\n"
            "<input id=\"t\" type=\"text\" name=\"term\"/>\r\n"
            "<input id=\"get\" type=\"submit\" value=\"Terminate the server\" />\r\n"
            "</form>\r\n"
            "<script type=\"text/javascript\">\r\n"
            "document.getElementById(\"t\").style.display = \"none\";\r\n"
            "</script>\r\n"
            "</body>\r\n"
            "</html>\r\n";
            
            snprintf((char*)buff, sizeof(buff), "%s", webpage);
            
            write(connfd, (char*) buff, strlen((char *)buff));
            close(connfd);
    }
    return NULL;

        
}

    
