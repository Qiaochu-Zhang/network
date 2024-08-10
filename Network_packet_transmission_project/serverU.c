#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>

#define SERVER_PORT 43240  // 请替换为您的端口号配置
#define MAXBUFLEN 1024
#define ROOM_STATUS_FILE "suite.txt"
#define LOCALHOST "127.0.0.1"
#define MAIN_SERVER_PORT 44240


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

// 处理从主服务器收到的请求
void handleRequests(int sockfd, RoomStatus *roomStatusArray, int roomStatusCount) {
    struct sockaddr_in clientAddr;
    socklen_t addr_len = sizeof clientAddr;
    char buf[MAXBUFLEN];
    int numbytes;
    int change =0;

    // 进入无限循环监听请求
    while (1) {
        memset(buf, 0, MAXBUFLEN);
        // 接收主服务器的请求
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
                                 (struct sockaddr *)&clientAddr, &addr_len)) == -1) {
            perror("serverU: recvfrom");
            exit(1);
        }

        // 确保字符串以null终止
        buf[numbytes] = '\0';
        printf("serverU: Received request: %s\n", buf);

        // 解析接收到的房间代码和请求类型
        char receivedRoomCode[10];
        char requestType[20];
        sscanf(buf, "%s %s", receivedRoomCode, requestType);

        // 初始化响应消息
        char response[MAXBUFLEN];
        memset(response, 0, MAXBUFLEN);
        toLowerCase(requestType);
        // 处理房间可用性查询或预订请求
        if (strcmp(requestType, "availability") == 0) {
            printf("The Server U received an availability request from the main server.\n");

            // 查询房间可用性
            int found = 0;
            for (int i = 0; i < roomStatusCount; i++) {
                if (strcmp(roomStatusArray[i].roomCode, receivedRoomCode) == 0) {
                    found = 1;
                    if (roomStatusArray[i].count > 0) {
                        snprintf(response, MAXBUFLEN, "The requested room is available.");
                        printf("Room %s is available.\n", receivedRoomCode);
                    } else {
                        snprintf(response, MAXBUFLEN, "The requested room is not available.");
                        printf("Room %s is not available.\n", receivedRoomCode);
                    }
                    break;
                }
            }
            if (!found) {
                snprintf(response, MAXBUFLEN, "Not able to find the room layout.");
                printf("Not able to find the room layout.");
            }
        } else if (strcmp(requestType, "reservation") == 0) {
            printf("The Server U received a reservation request from the main server.\n");
            // 处理房间预订请求
            int found = 0;
            for (int i = 0; i < roomStatusCount; i++) {
                if (strcmp(roomStatusArray[i].roomCode, receivedRoomCode) == 0) {
                    found = 1;
                    if (roomStatusArray[i].count > 0) {
                        roomStatusArray[i].count--; // 减少一个预订量
                        printf("Successful reservation. The count of Room %s is now %d.\n", receivedRoomCode, roomStatusArray[i].count);
                        change=1;
                        snprintf(response, MAXBUFLEN, "Successful reservation. Room %s count is now %d.", receivedRoomCode, roomStatusArray[i].count);
                    } else {
                        printf("Cannot make a reservation. Room %s is not available.", receivedRoomCode);
                        snprintf(response, MAXBUFLEN, "Cannot make a reservation. Room %s is not available.", receivedRoomCode);
                    }
                    break;
                }
            }
            if (!found) {
                snprintf(response, MAXBUFLEN, "Cannot make a reservation. Not able to find the room layout.");
                printf("Cannot make a reservation. Not able to find the room layout.");
            }
        } else {
            snprintf(response, MAXBUFLEN, "Invalid request type.");
            printf("Invalid request type.");
        }

        // 将响应发送回主服务器
        if ((numbytes = sendto(sockfd, response, strlen(response), 0,
                               (struct sockaddr *)&clientAddr, addr_len)) == -1) {
            perror("serverU: sendto");
            exit(1);
        }
        if(change ==0){

         printf("The Server U finished sending the response to the main server.\n");
        }
        else{
         printf("The Server U finished sending the response and the updated room status to the main server.\n");
         change =0;
        }
    }
}

// 加载房间状态数据
int loadRoomStatus(RoomStatus *roomStatusArray, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return -1;
    }

    char line[1024];
    int index = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        // 使用 %49[^,] 来确保不会读过界，且只读取到第一个逗号为止的内容
        if (sscanf(line, "%49[^,], %d", roomStatusArray[index].roomCode, &roomStatusArray[index].count) == 2) {
            index++;
        } else {
            printf("Error parsing line: %s\n", line);
        }
    }
    fclose(file);
    return index;
}

int main(void) {
    RoomStatus roomStatusArray[100];  // 假设最多100个房间状态
    int roomStatusCount;
    int sockfd;
    struct sockaddr_in serverAddr, mainServerAddr;
    int numbytes;

    printf("The Server U is up and running using UDP on port %d.\n", SERVER_PORT);

    // 从文件加载房间状态
    roomStatusCount = loadRoomStatus(roomStatusArray, ROOM_STATUS_FILE);
    if (roomStatusCount == -1) {
        exit(1);
    }

    // 创建UDP套接字用于接收请求和发送房间状态
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("serverU: socket");
        exit(1);
    }

    // 设置服务器地址
    memset(&serverAddr, 0, sizeof serverAddr);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(LOCALHOST);

    // 绑定套接字
    if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof serverAddr) == -1) {
        close(sockfd);
        perror("serverU: bind");
        exit(1);
    }

    // 设置主服务器地址
    memset(&mainServerAddr, 0, sizeof mainServerAddr);
    mainServerAddr.sin_family = AF_INET;
    mainServerAddr.sin_port = htons(MAIN_SERVER_PORT); // 替换为主服务器监听的端口号
    mainServerAddr.sin_addr.s_addr = inet_addr(LOCALHOST); // 替换为主服务器的IP地址

    // 发送房间状态到主服务器
    for (int i = 0; i < roomStatusCount; i++) {
        numbytes = sendto(sockfd, &roomStatusArray[i], sizeof(RoomStatus), 0,
                          (struct sockaddr *)&mainServerAddr, sizeof mainServerAddr);
        if (numbytes == -1) {
            perror("serverU: sendto main server");
            exit(1);
        }
        break;
    }
    printf("The Server U has sent the room status to the main server.\n");

    // 调用handleRequests处理查询和预订
    handleRequests(sockfd, roomStatusArray, roomStatusCount);

    // 关闭套接字
    close(sockfd);
    return 0;
}
