#include "include/cmdline.h"
#include <iostream>
#include <cmath>
#include <stdlib.h>
#include <vector>
#include "lstm.h"
#include "fstream"
#include "json/json.h"
#include "sys/socket.h"
#include "sys/types.h"
#include "netinet/in.h"
#include "unistd.h"
#include "arpa/inet.h"

using namespace std;

// 字符分割
double* split(string str, string pattern, int n)
{
    int pos;
	double* result = new double[n];
    str += pattern;//扩展字符串以方便操作
    int size = str.size();
	int j = 0;
    for (int i = 0; i < size; i++)
    {
        pos = str.find(pattern, i);
        if (pos < size)
        {
            string s = str.substr(i, pos - i);
            result[j] = atof(s.c_str());
            i = pos + pattern.size() - 1;
        }
		j++;
    }
    return result;
}

// 写json文件
bool writeFileJson(vector<double*> x, vector<int> p, int nodeNum, int n, double inconsistency, string filename) {
	Json::Value root;

	// 格式为value存储值，error存储判断结果
    for (int i = 0; i < n; i++) {
		for (int j = 0; j < nodeNum; j++) {
			root["value"][i].append(x[i][j]);
		}
		root["value"][i].append(p[i]);
	}
    root["inconsistency"].append(inconsistency);
	Json::StyledWriter sw;
	ofstream os;
	os.open(filename, ios::out | ios::trunc);
	if (!os.is_open()) {
		cout << "error: can not find or create the file which named" + filename << endl;;
	}
	os << sw.write(root);
    os.close();
	return true;
}

// 传输json
bool transportJson(vector<double*> x, vector<int> p, int nodeNum, int n) {
    Json::Value root;

	// 格式为value存储值，error存储判断结果
    for (int i = 0; i < n; i++) {
		for (int j = 0; j < nodeNum; j++) {
			root["value"][i].append(x[i][j]);
		}
	}
    for (int i = 0; i < n; i++) {
        root["error"].append(p[i]);
    }
	Json::StyledWriter sw;

    // 参数含义 AF_INET：网络通信,  SOCK_STREAM：TCP, 0：默认协议。
    int socketId = socket(AF_INET, SOCK_STREAM, 0);
    if (socketId == -1) {
        cout << "Failed to create socket" << endl;
        return false;
    }
    // 服务器IP地址
    const char *severIp = "192.168.3.154";
    // 结构体
    struct sockaddr_in severAddr;
    bzero(&severAddr, sizeof(severAddr));
    severAddr.sin_family = AF_INET;
    //将点分十进制串转换成网络字节序二进制值，此函数对IPv4地址和IPv6地址都能处理。
    inet_pton(AF_INET, severIp, &severAddr.sin_addr);
    //端口，暂设为8121
    severAddr.sin_port = htons(8122);
    
    // 链接函数
    int status = connect(socketId, (struct sockaddr *)&severAddr, sizeof(severAddr));
    if (status == -1) {
        cout << "Failed to connect" << endl;
        return false;
    }
    string tmp = sw.write(root);
    char *sendBuf = new char[tmp.length()+1];
    strcpy(sendBuf, tmp.c_str());
	int length = strlen(sendBuf);
	status = send(socketId, &sendBuf, length, 0);

    if (status == -1) {
		cout << "Error sending message" << endl;
        return false;
	}
    sleep(2);
    cout << "successfully transport" << endl;
    // 关闭socket套接字
    close(socketId);
    return true;
}


int main(int argc, char* argv[]){

	// 正确参数应为训练数据文件名，实际数据训练名，训练次数，异常阈值
	cmdline::parser parser;

	parser.add<string>("trainFileName", 't', "train file name", true);
	parser.add<string>("actualFileName", 'a', "actual file name", true);
	parser.add<string>("writeFileName", 'f', "write file name", false, "liangm_workplace/lstm");
	parser.add<int>("epoch", 'e', "epoch num", false, 1000);
	parser.add<double>("threshold", 's', "threshold", false, 25);
	parser.parse_check(argc, argv);

	int epoch = parser.get<int>("epoch");
	double threshold = parser.get<double>("threshold");
	string writeFileName = parser.get<string>("writeFileName") + ".json";
    // 训练集
	string trainFileName = parser.get<string>("trainFileName") + ".csv";
    ifstream trainData(trainFileName, ios::in);
    if (!trainData.is_open()) {
		cout << trainFileName << endl;
        cout << "trainData open failed!" << endl;
		return 0;
    }
    vector<double*> x;
    string temp;
    getline(trainData, temp);
	// 获得维数
    int nodeNum = 1;
	for (int i = 0; i < temp.size(); i++) {
		if (temp[i] == ',') {
			nodeNum++;
		}
	}
	do {
        x.push_back(split(temp, ",", nodeNum));
    } while(getline(trainData, temp));

	vector<double *> trainSet;
	vector<double *> labelSet;

	trainSet.assign(x.begin(), x.end()-1);
	labelSet.assign(x.begin()+1, x.end());
	int hideNum = 64;
	//初始化
	Lstm *lstm = new Lstm(nodeNum, hideNum, nodeNum);

	//投入训练
	cout<<"   /*** start training ***/"<<endl;
	lstm->train(trainSet, labelSet, epoch, 0, 0.000001);
	cout<<"   /*** finish training ***/"<<endl << "#########################" << endl;
	x.clear();
	// 在实际数据集检测异常
	cout << "   /*** start detecting ***/" << endl;
	
	// 训练集
	string actualFileName = parser.get<string>("actualFileName") + ".csv";
    ifstream actualData(actualFileName, ios::in);
    if (!actualData.is_open()) {
        cout << "actualData open failed!" << endl;
		return 0;
    }
	while(getline(actualData, temp)) {
        x.push_back(split(temp, ",", nodeNum));
    }
	int n = x.size();
	vector<int> output(n, 0);
	int errorSum = 0;
	FOR(i, n-1){
		double *z = lstm->predict(x[i]);
		double *temp = x[i+1];
		double e = 0;
		for (int j = 0; j < nodeNum; j++) {
			e += pow(temp[j] - z[j], 2);
		}
		e = sqrt(e);
		if (e > threshold) {
			cout<<"test now  : "<< i << "   error: " << e <<endl;
			errorSum++;
			output[i+1] = 1;
		}
		
		// free(temp);

		free(z);
	}
	cout << "   /*** finish detecting ***/" << endl;
	//  不一致度
	double inconsistency = (double) errorSum/(double) n;
	cout << "Inconsistency:		 " << inconsistency << endl;
	
	writeFileJson(x, output, nodeNum, n, inconsistency, writeFileName);
	transportJson(x, output, nodeNum, n);
	lstm->~Lstm();
	FOR(i, trainSet.size()){
		free(trainSet[i]);
	}
	FOR(i, n) {
		free(x[i]);
	}
	x.clear();
	trainSet.clear();
	labelSet.clear();
	return 0;
}




