#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <ctype.h>

#define TCP_PORT "45240"  // TCP port number for communication with clients
#define UDP_PORT 44240  // UDP port number for communication with back-end servers
#define BACKLOG 10        // How many pending connections queue will hold
#define LOCALHOST "127.0.0.1"
#define UDP_PORT_S "41240"
#define UDP_PORT_D "42240"
#define UDP_PORT_U "43240"
#define MAX_BUFFER_SIZE 1024

void sendTCPMessage(int sockfd, const char *message);
// 房间状态结构
typedef struct {
    char roomCode[10];
    int count;
} RoomStatus;

void toLowerCase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower((unsigned char) str[i]);  // Cast to unsigned char to handle potential negative chars
    }
}
// 解密函数，将每个字符向后偏移3位
void decrypt(char *str) {
    while (*str) {
        if (isalpha(*str)) {  // 字母字符
            int offset = isupper(*str) ? 'A' : 'a';
            *str = (*str - offset - 3 + 26) % 26 + offset;
        } else if (isdigit(*str)) {  // 数字字符
            *str = (*str - '0' - 3 + 10) % 10 + '0';
        }
        // 特殊字符不加密，直接跳过
        str++;
    }
}

// 验证用户名和密码的函数
int authenticateUser(char *username, char* password, int client_fd) {
    // 解密凭据
    decrypt(username);
    decrypt(password);
        // 打印解密后的凭据以便调试（在生产代码中应移除）
    // printf("Decrypted username: %s\n", username);
    // printf("Decrypted password: %s\n", password);
   // bool isAuthenticated = false;
    if (password && (password[0] != '\0' && password[0] != '\n')){
        printf("The main server received the authentication for %s using TCP over port %s.\n", username, TCP_PORT);
        // 打开注册用户的用户名和密码文件
        FILE *file = fopen("member.txt", "r");
        char line[1024], fileUsername[50], filePassword[50];
        if (!file) {
            perror("Cannot open file");
            return false;
        }
        
        while (fgets(line, sizeof(line), file) != NULL) {
            if (sscanf(line, "%49[^,], %49s", fileUsername, filePassword) == 2) {
                decrypt(fileUsername);
                decrypt(filePassword);
                // printf("Username: %s, Password: %s\n", fileUsername, filePassword);
                if (strcmp(username, fileUsername) == 0){
                    if(strcmp(password, filePassword) == 0) {
                      //  isAuthenticated = true;
                        sendTCPMessage(client_fd, "yes for member");
                        printf("The main server sent the authentication result to the client.\n");
                        fclose(file);
                        // printf("M41s\n");
                        return 1;
                    }
                    else{
                       // isAuthenticated = false;
                        sendTCPMessage(client_fd, "password error");
                        printf("The main server sent the authentication result to the client.\n");
                        fclose(file);
                        // printf("M42s\n");
                        return 100;
                    }
                }
            } else {
                printf("Error parsing line: %s\n", line);
            //    printf("M43\n");
            }
        }
       // isAuthenticated = false;
        sendTCPMessage(client_fd, "username error");
        printf("The main server sent the authentication result to the client.\n");
        // printf("M44s\n");
        fclose(file);
        return 100;
    }
    else{
       // isAuthenticated = true;
        printf("The main server received the guest request for %s using TCP over port %s. The main server accepts %s as a guest.\n", username, TCP_PORT, username);
        sendTCPMessage(client_fd, "yes for guest");
        printf("The main server sent the guest response to the client.\n");
        // printf("M45s\n");
    }
    return 1;
}

void setupUDP() {
    int udp_sockfd;
    struct sockaddr_in udp_server_addr;
    udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sockfd == -1) {
        perror("serverM: UDP socket creation failed");
        exit(1);
    }

    memset(&udp_server_addr, 0, sizeof(udp_server_addr));
    udp_server_addr.sin_family = AF_INET;
    udp_server_addr.sin_port = htons(UDP_PORT);
    udp_server_addr.sin_addr.s_addr = inet_addr(LOCALHOST);

    if (bind(udp_sockfd, (struct sockaddr *)&udp_server_addr, sizeof(udp_server_addr)) == -1) {
        perror("serverM: UDP bind failed");
        close(udp_sockfd);
        exit(1);
    }

    printf("UDP server setup complete on port %d.\n", UDP_PORT);
    RoomStatus status;
    struct sockaddr_in backendAddr;
    socklen_t addr_len = sizeof(backendAddr);
    int numbytes;

    while (true) {
        //printf("M10s\n");
        numbytes = recvfrom(udp_sockfd, &status, sizeof(RoomStatus), 0,
                            (struct sockaddr *)&backendAddr, &addr_len);
        if (numbytes == -1) {
            perror("recvfrom failed");
            continue;  // 这里选择继续监听下一个消息而不是退出
        }
        //printf("M11s\n");
        char backendIdentifier = status.roomCode[0];  // 假设房间代码以 S/D/U 开头
        //printf("M12s\n");
        printf("The main server has received the room status from Server %c using UDP over port %d.\n", backendIdentifier, UDP_PORT);
        // 处理接收到的状态...
    }     
//printf("M15s\n");
    close(udp_sockfd); // 在适当的时候关闭UDP套接字
}


void sigchld_handler(int s) {
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// 函数用于向客户端发送消息
void sendTCPMessage(int sockfd, const char *message) {
    if (send(sockfd, message, strlen(message), 0) == -1) {
        perror("send");
    }
}

// 这个函数现在接受 username 参数
int handleMemberRequest(int tcp_client_fd, const char *username) {
    char buffer[MAX_BUFFER_SIZE];
    int numbytes;
    struct sockaddr_in server_addr;
    int udp_sockfd;
    socklen_t addr_len;
    char udp_response[MAX_BUFFER_SIZE];
        char room_code[10];
        char action[20];
   // printf("receive handle member");

    // 持续监听客户端请求
    while (1) {
        // 清除buffer
        memset(buffer, 0, sizeof(buffer));
        memset(action, 0, sizeof(action));
        memset(room_code, 0, sizeof(room_code));

        // 接收客户端的请求
        numbytes = recv(tcp_client_fd, buffer, sizeof(buffer) - 1, 0);
        if (numbytes == -1) {
            perror("recv from tcp_client");
            break;
        }
        if (numbytes == 0) {
            // 客户端已经关闭连接
            printf("Client has closed the connection\n");
            break;
        }

        buffer[numbytes] = '\0';
        // 检查接收到的消息是否是"BACK"
        if (strcmp(buffer, "BACK") == 0) {
            return 100;  // 如果接收到的消息是"BACK"，返回 100
        }

        // 日志消息：收到客户端的请求
       // printf("The main server has received the request on Room %s from %s using TCP over port.\n", buffer, username);
        
        // 解析房间代码
        sscanf(buffer, "%s %s", room_code, action);
        toLowerCase(action);
        printf("The main server has received the %s request on Room %s from %s using TCP over port %s.\n", action, room_code, username, TCP_PORT);
        // 确定服务器类型并设置端口号
        int server_udp_port;
        switch (room_code[0]) {
            case 'S': server_udp_port = atoi(UDP_PORT_S); break;
            case 'D': server_udp_port = atoi(UDP_PORT_D); break;
            case 'U': server_udp_port = atoi(UDP_PORT_U); break;
            default:
                sendTCPMessage(tcp_client_fd, "Invalid room code.\n");
                continue;
        }

        // 日志消息：向子服务器发送请求
        printf("The main server sent a request to Server %c.\n", room_code[0]);

        // 创建 UDP 套接字
        udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (udp_sockfd == -1) {
            perror("UDP socket creation failed");
            continue;
        }

        // 设置 UDP 目标服务器地址
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_udp_port);
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 本地地址

        // 发送 UDP 请求到指定服务器
        if (sendto(udp_sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("sendto failed");
            close(udp_sockfd);
            continue;
        }

        // 接收服务器的回应
        addr_len = sizeof(server_addr);
        if ((numbytes = recvfrom(udp_sockfd, udp_response, sizeof(udp_response) - 1, 0, (struct sockaddr *)&server_addr, &addr_len)) == -1) {
            perror("recvfrom failed");
            close(udp_sockfd);
            continue;
        }

        udp_response[numbytes] = '\0'; // Null-terminate the string

        if (strcmp(action, "availability") == 0) {
            // 打印接收UDP响应的消息
            printf("The main server received the response from Server %c using UDP over port %d.\n", room_code[0], server_udp_port);

            // 将UDP响应转发至TCP客户端
            sendTCPMessage(tcp_client_fd, udp_response);

            // 打印发送给客户端的消息
            printf("The main server sent the availability information to the client.\n");
        } else if (strcmp(action, "reservation") == 0) {
            // 打印接收UDP响应及房间状态更新的消息
            printf("The main server received the response and the updated room status from Server %c using UDP over port %d.\n",  room_code[0], server_udp_port);

            // 模拟房间状态更新日志
            printf("The room status of Room %s has been updated.\n", room_code);

            // 将预订结果发送至客户端
            sendTCPMessage(tcp_client_fd, udp_response);

            // 打印发送预订结果给客户端的消息
            printf("The main server sent the reservation result to the client.\n");
        }
        // 关闭 UDP 套接字
        close(udp_sockfd);
    }

    // 关闭 TCP 连接
    close(tcp_client_fd);
    return 0;
}

int handleGuestRequest(int tcp_client_fd, const char *username) {
    // printf("M19s\n");
    char buffer[MAX_BUFFER_SIZE];
    int numbytes;
    struct sockaddr_in server_addr;
    int udp_sockfd;
    socklen_t addr_len;
    // printf("M20s\n");
    char udp_response[MAX_BUFFER_SIZE];
    //printf("receive handle guest");
    // printf("M21s\n");
    // 循环监听客户端请求
    char room_code[10];
    char action[20];
    while (1) {
        // 清除buffer
       
        memset(buffer, 0, sizeof(buffer));
        memset(action, 0, sizeof(action));
        memset(room_code, 0, sizeof(room_code));
        // 接收客户端的请求
        // printf("M21.5s\n");
        numbytes = recv(tcp_client_fd, buffer, sizeof(buffer) - 1, 0);
        // printf("M22s\n");
        if (numbytes == -1) {
            perror("recv from tcp_client");
            break;
        }
        if (numbytes == 0) {
            // 客户端已经关闭连接
            printf("Client has closed the connection\n");
            // printf("M23s\n");
            break;
        }

        buffer[numbytes] = '\0';
        // if (strcmp(buffer, "BACK") == 0) {
        //     return 100;  // 如果接收到的消息是"BACK"，返回 100
        // }
        // printf("M24s\n");
        // 检查是否是预订请求
        sscanf(buffer, "%s %s", room_code, action);
        toLowerCase(action);
        printf("The main server has received the %s request on Room %s from %s using TCP over port %s.\n", action, room_code, username, TCP_PORT);
        if (strstr(buffer, "Reservation") != NULL) {
            // 非会员尝试进行预订
            printf("%s cannot make a reservation.\n", username);
            sendTCPMessage(tcp_client_fd, "Permission denied: Guest cannot make a reservation.\n");
            printf("The main server sent the error message to the client.\n");
            continue; // 继续监听下一个请求
        }
        // printf("M25s\n");
        // 根据房间代码首字母选择服务器端口
        int server_udp_port;
        switch (room_code[0]) {
            case 'S': server_udp_port = atoi(UDP_PORT_S); break;
            case 'D': server_udp_port = atoi(UDP_PORT_D); break;
            case 'U': server_udp_port = atoi(UDP_PORT_U); break;
            default:
                sendTCPMessage(tcp_client_fd, "Invalid room code.\n");
                continue;
        }

         // 日志消息：向子服务器发送请求
        printf("The main server sent a request to Server %c.\n", room_code[0]);

        // 创建 UDP 套接字
        udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (udp_sockfd == -1) {
            perror("UDP socket creation failed");
            continue;
        }

        // 设置服务器地址
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_udp_port);
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 本地地址

        // 发送查询请求到服务器
        if (sendto(udp_sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("sendto failed");
            close(udp_sockfd);
            continue;
        }

        // 接收服务器响应
        addr_len = sizeof(server_addr);
        if ((numbytes = recvfrom(udp_sockfd, udp_response, sizeof(udp_response) - 1, 0, (struct sockaddr *)&server_addr, &addr_len)) == -1) {
            perror("recvfrom failed");
            close(udp_sockfd);
            continue;
        }
        udp_response[numbytes] = '\0'; // Null-terminate the string


            // 打印接收UDP响应的消息
            printf("The main server received the response from Server %c using UDP over port %d.\n", room_code[0], server_udp_port);

            // 将UDP响应转发至TCP客户端
            sendTCPMessage(tcp_client_fd, udp_response);

            // 打印发送给客户端的消息
            printf("The main server sent the availability information to the client.\n");

        // 关闭 UDP 套接字
        close(udp_sockfd);
    }

    // 客户端处理结束，关闭连接
    close(tcp_client_fd);
    return 0;
}

int main(void) {
    int tcp_sockfd, client_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sigaction sa;
    int yes=1;
    socklen_t sin_size;
    int rv;
    char s[INET_ADDRSTRLEN];
     struct sockaddr_storage their_addr;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // Use IPv4 only
    hints.ai_socktype = SOCK_STREAM; // TCP socket

    if ((rv = getaddrinfo(LOCALHOST, TCP_PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((tcp_sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("serverM: socket");
            continue;
        }

        if (setsockopt(tcp_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(tcp_sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(tcp_sockfd);
            perror("serverM: bind");
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL) {
        fprintf(stderr, "serverM: failed to bind\n");
        exit(1);
    }

    if (listen(tcp_sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("The main server is up and running.\n");
      // 创建子进程来处理UDP通信
    pid_t pid = fork();
    if (pid == 0) {
        // 子进程
        setupUDP();
       // printf("MMM100\n");
        exit(0);
    } else if (pid < 0) {
        perror("UDP fork failed");
        exit(1);
    }


    while (1) {
        sin_size = sizeof their_addr;
        client_fd = accept(tcp_sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
       // printf("serverM: got connection from %s\n", s);

        if (!fork()) {  // this is the child process
            close(tcp_sockfd);  // close the listen socket
            // printf("M10s\n");
            char credentials[100];
            char username[50], password[50];
            while (1) {
                memset(credentials, 0, sizeof(credentials));
                memset(username, 0, sizeof(username));
                memset(password, 0, sizeof(password));
                int numbytes = recv(client_fd, credentials, sizeof(credentials) - 1, 0);
                // printf("M11s\n");
                if (numbytes <= 0) {
                    if (numbytes == 0) {
                        printf("Client disconnected\n");
                    } else {
                        perror("recv");
                    }
                    break; // Exit the loop to close the socket
                }
                credentials[numbytes] = '\0'; // null-terminate string
                char* token = strtok(credentials, ", ");
                // printf("M12s\n");
                if (token) {
                    // printf("M13s\n");
                    strncpy(username, token, sizeof(username) - 1);
                    username[sizeof(username) - 1] = '\0';

                    token = strtok(NULL, ", ");
                    if (token && (token[0] != '\0' && token[0] != '\n')) {
                        strncpy(password, token, sizeof(password) - 1);
                        password[sizeof(password) - 1] = '\0';
                        // printf("M14s\n");

                        if (authenticateUser(username, password,client_fd)!=100) {
                            handleMemberRequest(client_fd, username);
                             
                            // printf("M15s\n");
                            break; // Successful authentication, exit the loop
                        }
                        else{
                            // printf("M32s\n");
                            continue;
                        }
                        // If authentication fails, loop will continue to receive new credentials
                    } else {
                        // if(authenticateUser(username, password,client_fd)==100){
                        //     printf("M33s\n");
                        //     continue;
                        // }
                        // else{
                            authenticateUser(username, password,client_fd);
                            // printf("M16s\n");
                            //printf("%d, %s\n", client_fd,username);
                            // printf("M17s\n");
                            handleGuestRequest(client_fd, username);
                            // printf("M18s\n");
                        // }
                        
                    }
                } 
                else {
                    char errorMsg[] = "folk wonrgly.\n";
                    send(client_fd, errorMsg, sizeof(errorMsg), 0);
                    break; // Exit the loop due to invalid input
                }
            }
            close(client_fd);  // close client socket
            exit(0);
        }
        close(client_fd);  // parent closes off this client's socket
    }

    return 0;
}