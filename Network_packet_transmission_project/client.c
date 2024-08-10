#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <ctype.h>


#define SERVER_PORT "45240"  // 主服务器的端口号
#define SERVER_IP "127.0.0.1" // 主服务器的IP地址
#define MAX_DATA_SIZE 1024

// 用于加密用户名和密码的函数
char* encryptData(const char *str) {
    // 首先，计算输入字符串的长度
    int length = 0;
    const char *temp = str;
    while (*temp++) length++;

    // 分配新字符串的内存，长度加1用于存放终止字符'\0'
    char *encrypted = malloc(length + 1);
    if (encrypted == NULL) return NULL; // 内存分配失败的处理

    // 用来遍历新字符串的指针
    char *dest = encrypted;

    // 加密过程
    while (*str) {
        if (isalpha(*str)) {
            int offset = isupper(*str) ? 'A' : 'a';
            *dest = ((*str - offset + 3) % 26) + offset;
        } else if (isdigit(*str)) {
            *dest = ((*str - '0' + 3) % 10) + '0';
        } else {
            *dest = *str; // 处理非字母和非数字字符
        }
        str++;
        dest++;
    }

    // 字符串结束符
    *dest = '\0';

    // 返回新创建的加密字符串
    return encrypted;
}
void toLowerCase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower((unsigned char) str[i]);  // Cast to unsigned char to handle potential negative chars
    }
}
void sendTCPMessage(int sockfd, const char *message) {
    if (send(sockfd, message, strlen(message), 0) == -1) {
        perror("send");
    }
}

bool sendCredentialsGuest(int sockfd, const char *encryptedCredentials, char *username) {
    
    sendTCPMessage(sockfd, encryptedCredentials);
    printf("%s sent an authentication request to the main server.\n", username);
    // printf("\n%d\n", sockfd);

     char server_response[MAX_DATA_SIZE];
   //   printf("77\n");
    int numbytes = recv(sockfd, server_response, MAX_DATA_SIZE - 1, 0);
    if (numbytes == -1) {
        perror("recv");
        return false;
    }
     //printf("88\n");
    server_response[numbytes] = '\0'; // Null-terminate the response

    // Print the appropriate welcome message based on server response
    if (strstr(server_response, "yes for guest")) {
        printf("Welcome guest %s!\n", username); // Should decrypt the username for actual username
        return true;
    // } else if(strstr(server_response, "no")) {
    //     printf("Failed login and error in sendCredentialsGuest . %s\n", username);
    //     return false;
    }
    else{
         printf("server_response: %s\n", server_response);
     //   printf("error in sendCredentialsGuest. %s\n", username);
        return false;
    }
}
bool sendCredentialsAndValidate(int sockfd, const char *encryptedCredentials, char *username) {
    // Send encrypted credentials to server
    sendTCPMessage(sockfd, encryptedCredentials);
    printf("%s sent an authentication request to the main server.\n", username);

    // Receive authentication result from server
    char server_response[MAX_DATA_SIZE];
    int numbytes = recv(sockfd, server_response, MAX_DATA_SIZE - 1, 0);
    // printf("recv success sendCredentialsAndValidate.\n");
    if (numbytes == -1) {
        perror("recv");
        return false;
    }
    server_response[numbytes] = '\0'; // Null-terminate the response

    // Print the appropriate welcome message based on server response
    if (strstr(server_response, "yes for member")) {
        printf("Welcome member %s!\n", username); // Should decrypt the username for actual username
        return true;
    } else if(strstr(server_response, "username error")) {
        printf("Failed login: Username does not exist.\n");
        return false;
    }else if(strstr(server_response, "passord error")) {
        printf("Failed login: Password does not match.\n");
        return false;
    }
    else{
        printf("server_response: %s\n", server_response);
      //  printf("error in sendCredentialsAndValidate. %s\n", username);
        return false;
    }
}

void handleMemberRequest(int sockfd, char *username) {
    char room_code[MAX_DATA_SIZE];
    char action[MAX_DATA_SIZE];
    char message[MAX_DATA_SIZE * 2];
    int numbytes;

    // 主循环以持续接收用户命令
    while (1) {
            memset(room_code, 0, sizeof(room_code));
          memset(action, 0, sizeof(action));
        printf("\n-----Start a new request-----\n");
        printf("Please enter the room code: ");
        fgets(room_code, MAX_DATA_SIZE, stdin);
        room_code[strcspn(room_code, "\n")] = 0; // 移除换行符
        // 检查用户是否想要查询房间或进行预订
        printf("Would you like to search for the availability or make a reservation? (Enter 'Availability' to search for the availability or Enter 'Reservation' to make a reservation): ");
        fgets(action, MAX_DATA_SIZE, stdin);
        action[strcspn(action, "\n")] = 0; // 移除换行符
        toLowerCase(action);
        // 根据用户选择进行操作
        if (strcmp(action, "availability") != 0 && strcmp(action, "reservation") != 0) {
            printf("Invalid action specified.\n");
            continue;
        }

        // 构建发送给服务器的消息
        snprintf(message, sizeof(message), "%s %s", room_code, action);

        // 发送消息到主服务器
        sendTCPMessage(sockfd, message);

        printf("%s sent a %s request to the main server.\n", username, action);

        // 接收服务器响应
        char server_response[MAX_DATA_SIZE];
        if ((numbytes = recv(sockfd, server_response, MAX_DATA_SIZE - 1, 0)) == -1) {
            perror("recv server response");
            continue; // 如果接收失败，继续下一次循环
        }

        server_response[numbytes] = '\0'; // 确保字符串正确终结
        printf("%s\n", server_response);

        // // 根据服务器的响应执行相应操作
        // if (strcmp(server_response, "The requested room is available.") == 0) {
        //     printf("Congratulations! The room is available for reservation.\n");
        //     // 进行预订流程
        // } else if (strcmp(server_response, "The requested room is not available.") == 0) {
        //     printf("Sorry! The requested room is not available.\n");
        //     // 提示用户选择其他房间或操作
        // } else if (strcmp(server_response, "Not able to find the room layout.") == 0) {
        //     printf("Oops! Not able to find the room.\n");
        //     // 提示用户重新输入房间代码
        // }
        // 注意：此处应该有额外的逻辑来处理发送 "Availability" 和 "Reservation" 请求的细节
        
    }
}


void handleGuestRequest(int sockfd, const char *username) {
    char room_code[MAX_DATA_SIZE];
    char action[MAX_DATA_SIZE];
    char message[MAX_DATA_SIZE * 2];
    int numbytes;

    // 显示欢迎信息
   // printf("Welcome guest %s!\n", username);

    // 主循环以持续接收游客命令
    while (1) {
         memset(room_code, 0, sizeof(room_code));
          memset(action, 0, sizeof(action));
        printf("Please enter the room code: ");
        fgets(room_code, MAX_DATA_SIZE, stdin);
        room_code[strcspn(room_code, "\n")] = 0; // 移除换行符

        // if (strcmp(room_code, "quit") == 0) {
        //     printf("Exiting program.\n");
        //     break; // 如果用户输入 'quit'，则退出循环
        // }

        printf("Would you like to search for the availability or make a reservation (Enter “Availability” to search for the availability or Enter “Reservation” to make a reservation): ");
        fgets(action, MAX_DATA_SIZE, stdin);
        action[strcspn(action, "\n")] = 0; // 移除换行符
        toLowerCase(action);
        // 如果用户尝试进行预订，则显示错误消息
        if (strcmp(action, "reservation") == 0) {
            printf("%s sent an reservation request to the main server.\n", username);
            printf("Permission denied: Guest cannot make a reservation.\n");
            continue;
        } else if (strcmp(action, "availability") == 0) {
            // 构建发送给服务器的消息
            snprintf(message, sizeof(message), "%s %s", room_code, action);
          //  printf("%s\n", message);
            // 发送查询可用性请求到主服务器
            sendTCPMessage(sockfd, message);

            printf("Guest %s sent an availability request to the main server.\n", username);

            // 接收服务器响应
            char server_response[MAX_DATA_SIZE];
            if ((numbytes = recv(sockfd, server_response, MAX_DATA_SIZE - 1, 0)) == -1) {
                perror("recv server response");
                continue; // 如果接收失败，继续下一次循环
            }

            server_response[numbytes] = '\0'; // 确保字符串正确终结
            printf("Server response: %s\n", server_response);

            // 根据服务器的响应执行相应操作
            if (strstr(server_response, "The requested room is available") != NULL) {
                printf("The requested room is available.\n");
            } else if (strstr(server_response, "The requested room is not available") != NULL) {
                printf("The requested room is not available.\n");
            } else if (strstr(server_response, "Not able to find the room layout") != NULL) {
                printf("Not able to find the room layout.\n");
            }
        } else {
            printf("Invalid action specified.\n");
        }
        // 注意：此处应该有额外的逻辑来处理 'quit' 命令以及其它可能的情况
    }
}


int main() {
    //int sockfd, numbytes;
    int sockfd;
    //char room_code[MAX_DATA_SIZE];
    char username[MAX_DATA_SIZE], password[MAX_DATA_SIZE];
    struct sockaddr_in server_addr;
    char encrypted_credentials[2 * MAX_DATA_SIZE + 2]; // 加密后的用户名和密码

    // 创建TCP套接字
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("client: socket");
        return 1;
    }

    // 设置服务器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(SERVER_PORT));
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    memset(server_addr.sin_zero, '\0', sizeof server_addr.sin_zero);

    // 连接到主服务器
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof server_addr) == -1) {
        close(sockfd);
        perror("client: connect");
        return 1;
    }
    printf("Client is up and running.\n");
     while (1) {
        printf("Please enter the username: ");
        scanf("%s", username);
       // printf("1111\n ");
        // 清除缓冲区中的残留数据
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }

        printf("Please enter the password (Press \"Enter\" to skip for guests): ");
        fgets(password, MAX_DATA_SIZE, stdin);
        password[strcspn(password, "\n")] = 0; // 移除换行符
       // printf("222\n ");
        char *encrypted_username = encryptData(username);
        char *encrypted_password = encryptData(password);
        if (strlen(password) == 0) {
        //   printf("333\n ");
            sprintf(encrypted_credentials, "%s, %s", encrypted_username, encrypted_password);
            if (sendCredentialsGuest(sockfd, encrypted_credentials, username)) {
              //  printf("Welcome guest %s!\n", username); // Should be decrypted username
                handleGuestRequest(sockfd, username); // handleGuestRequest should be modified to send a guest request
            } else {
               // printf("Failed : error in sendCredentialsGuest.\n");
                // Clear username and password for the next loop iteration
                memset(username, 0, sizeof(username));
                memset(password, 0, sizeof(password));
            }
        } else {
            //printf("444\n ");
            // Encrypt the username and password
            // char *encrypted_username = encryptData(username);
            // char *encrypted_password = encryptData(password);
            // Create encrypted credentials string
            sprintf(encrypted_credentials, "%s, %s", encrypted_username, encrypted_password);
            if (sendCredentialsAndValidate(sockfd, encrypted_credentials, username)) {
              //  printf("Welcome member %s!\n", username); // Should be decrypted username
                handleMemberRequest(sockfd, username);
            } else {
                printf("Failed login: Username or password is incorrect.\n");
                // Clear username and password for the next loop iteration
                memset(username, 0, sizeof(username));
                memset(password, 0, sizeof(password));
            }
        }
    }
    close(sockfd);
    return 0;
}
