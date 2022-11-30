#include "iostream"
#include "math.h"
#include "stdlib.h"
#include "vector"
#include "assert.h"
#include "string.h"
#include "fstream"
#include "cfloat"
#include "include/cmdline.h"
#include "json/json.h"
#include "sys/socket.h"
#include "sys/types.h"
#include "netinet/in.h"
#include "unistd.h"
#include "arpa/inet.h"

using namespace std;

int main() {
    int MAXSIZE = 1024;
    // 参数含义 AF_INET：网络通信,  SOCK_STREAM：TCP, 0：默认协议。
    int socketId = socket(AF_INET, SOCK_STREAM, 0);
    // 服务器IP地址
    // const char *severIp = "192.168.227.134";
    // 结构体
    struct sockaddr_in severAddr, clientAddr;
    bzero(&severAddr, sizeof(severAddr));
    severAddr.sin_family = AF_INET;
    severAddr.sin_addr.s_addr = INADDR_ANY;
    //端口，暂设为8121
    severAddr.sin_port = htons(8888);
    // 将点分十进制串转换成网络字节序二进制值，此函数对IPv4地址和IPv6地址都能处理。
    // inet_pton(AF_INET, severIp, &severAddr.sin_addr);
    // 将套接字文件与服务器端口地址绑定
    int kk = bind(socketId, (struct sockaddr *)&severAddr, sizeof(severAddr));
    if (kk == -1) {
        cout << "bind Error" << endl;
    } 
    kk = listen(socketId, 1);
    if (kk == -1) {
        cout << "listen Error" << endl;
    } 
    cout << "======bind success,waiting for client's request======\n" << endl;
    socklen_t clientLen = sizeof(clientAddr);
    int clientId;
    char buf[MAXSIZE];
    int i = 0;
    while (1) {
        clientId = accept(socketId, (struct sockaddr *)&clientAddr, &clientLen);
    
        Json::Value val;
        Json::Reader reader;
        string res;
        memset(buf, '\0', sizeof(buf));
        while (1) {
            int n = recv(clientId, buf, MAXSIZE/sizeof(char), 0);
            if (n > 0) {
                res += buf;
            } else {
                break;
            }
        }
        reader.parse(res, val);
    
        Json::StyledWriter sw;
        //输出到文件  
	    ofstream os;
        cout << "--------------------------------" << endl;
        cout << "write json" << endl;
	    os.open("liangm_workplace/lstm.json", ios::out | ios::app);
	    if (!os.is_open())
		    cout << "error: can not find or create the file which named \" imr.json\"." << endl;
	    os << sw.write(val);
	    os.close();
        close(clientId);
        close(socketId);
        break;
    }

    return 0;
}