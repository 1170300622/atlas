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
#include "unistd.h"

using namespace std;

// 矩阵操作

// 二维转一维
vector<double> twoToOne(vector<vector<double>> data) {
    vector<double> res;
    for (int i = 0; i < data.size(); i++) {
        res.push_back(data[i][0]);
    }
    return res;
}

// 矩阵转置()
vector<vector<double>> matrixTranspose(vector<vector<double>> array) {
    vector<vector<double>> transArray;
    vector<double> tempArray;
    for (int i = 0; i < array[0].size(); i++) {
        for (int j = 0; j < array.size(); j++) {
            tempArray.push_back(array[j][i]);
        }
        transArray.push_back(tempArray);
        tempArray.clear();
    }
    return transArray;
}

vector<vector<double>> make_zero_martix(int m, int n) {
    //创建0矩阵
    vector<vector<double>> array;
    vector<double> temparay;
    for (int i = 0; i < m; ++i)// m*n 维数组
    {
        for (int j = 0; j < n; ++j)
            temparay.push_back(i * j);
        array.push_back(temparay);
        temparay.erase(temparay.begin(), temparay.end());
    }
    return array;
}

//按第一行展开计算|A|
double getA(vector<vector<double>> arcs, int n)
{
    if (n == 1)
    {
        return arcs[0][0];
    }
    double ans = 0;
    vector<vector<double>> temp = make_zero_martix(arcs.size(), arcs.size());
    int i, j, k;
    for (i = 0; i < n; i++)
    {
        for (j = 0; j < n - 1; j++)
        {
            for (k = 0; k < n - 1; k++)
            {
                temp[j][k] = arcs[j + 1][(k >= i) ? k + 1 : k];
            }
        }
        double t = getA(temp, n - 1);
        if (i % 2 == 0)
        {
            ans += arcs[0][i] * t;
        }
        else
        {
            ans -= arcs[0][i] * t;
        }
    }
    return ans;
}

//计算每一行每一列的每个元素所对应的余子式，组成A*
void  getAStart(vector<vector<double>> arcs, int n, vector<vector<double>>& ans)
{
    if (n == 1)
    {
        ans[0][0] = 1;
        return;
    }
    int i, j, k, t;
    vector<vector<double>> temp = make_zero_martix(n, n);
    for (i = 0; i < n; i++)
    {
        for (j = 0; j < n; j++)
        {
            for (k = 0; k < n - 1; k++)
            {
                for (t = 0; t < n - 1; t++)
                {
                    temp[k][t] = arcs[k >= i ? k + 1 : k][t >= j ? t + 1 : t];
                }
            }
            ans[j][i] = getA(temp, n - 1);  //此处顺便进行了转置
            if ((i + j) % 2 == 1)
            {
                ans[j][i] = -ans[j][i];
            }
        }
    }
}

//得到给定矩阵src的逆矩阵保存到des中。
bool GetMatrixInverse(vector<vector<double>> src, int n, vector<vector<double>>& des)
{
    double flag = getA(src, n);
    vector<vector<double>> t = make_zero_martix(n, n);
    if (0 == flag)
    {
        cout << "原矩阵行列式为0，无法求逆。请重新运行" << endl;
        return false;//如果算出矩阵的行列式为0，则不往下进行
    }
    else
    {
        getAStart(src, n, t);
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                des[i][j] = t[i][j] / flag;
            }
        }
    }
    return true;
}

//矩阵求逆
vector<vector<double>> inverse(vector<vector<double>> matrix_before) {    
    bool flag;
    vector<vector<double>> matrix_after = make_zero_martix(matrix_before.size(), matrix_before.size());
    flag = GetMatrixInverse(matrix_before, matrix_before.size(), matrix_after);
    return matrix_after;
}

//两矩阵相乘
vector<vector<double>> multiplication(vector<vector<double>> m1, vector<vector<double>> m2) {    
    int m = m1.size();
    int n = m1[0].size();
    int p = m2[0].size();
    vector<vector<double>> array;
    vector<double> temparay;
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < p; j++) {
            double sum = 0;
            for (int k = 0; k < n; k++) {
                sum += m1[i][k] * m2[k][j];
            }
            temparay.push_back(sum);
        }
        array.push_back(temparay);
        temparay.clear();
    }
    
    return array;
}


vector<double> estimate(vector<double> x, vector<double> yk, int p, vector<vector<double>> Z, vector<vector<double>> V) {
    int n = x.size();
    for (int i = 0; i < n-p; i++) {
        V[i][0] = yk[i + p] - x[i+p];
    }
    for (int i = 0; i < n-p; i++) {
        for (int j = p-1; j >= 0; j--) {
            Z[i][p-1-j] = yk[j+i] - x[j+i];
        }
    }
    return twoToOne(multiplication(inverse(multiplication(matrixTranspose(Z), Z)), multiplication(matrixTranspose(Z), V)));
}

/*
    x：         原始序列
    yk:         当前序列
    para:       参数序列
    p:          窗口大小
    threshold:  判断更改界限
*/
vector<double> arx(vector<double> x, vector<double> yk, vector<double> para, int p, double threshold) {
    int n = yk.size();
    vector<double> res(n);
    for (int i = 0; i < p; i++) {
        res[i] = x[i];
    }

    for (int t = p; t < n; t++) {
        res[t] = x[t];
        for (int i = 0; i < p; i++) {
            res[t] += para[i] * (yk[t - i - 1] - x[t - i - 1]);
        }
        if (abs(res[t] - yk[t]) < threshold) {
            res[t] = yk[t];
        }
    }
    return res;
}

/*
    x：             原始序列
    tempY:          当前改动的序列
    r:              更改y的位置
*/
void evaluate(vector<double> x, vector<double> tempY, int &r) {
    int n = x.size();
    double min = DBL_MAX;
    for (int i = 0; i < n; i++) {
        double temp = abs(tempY[i] - x[i]);
        if (temp > 0.05 && temp < min) {
            min = temp;
            r = i;
        }
    }
}


/* 
   x:                   初始单维数据
   y0:                  初始部分标记数据
   max_num_iterations:  最大迭代次数
   p                    窗口大小
   mark                 标记

*/
vector<double> IMR(vector<double> x, vector<double> preY, int max_num_iterations, int p, int n) {
    // 初始Z
    vector<vector<double>> Z(n-p, vector<double>(p));
    // // 初始V
    vector<vector<double>> V(n-p, vector<double>(1));

    vector<double> para(p);
    int r;
    // 迭代
    for (int k = 0; k < max_num_iterations; k++) {
        
        para = estimate(x, preY, p, Z, V);

        vector<double> tempY = arx(x, preY, para, p, 0.01);
        r = -1;
        evaluate(x, tempY, r);
        if (r >= 0) {
            preY[r] = tempY[r];
        } else {
            break;
        }
        tempY.clear();
        
    }   
    return preY;
}

// 训练(获得参数集)
vector<double> getParameter(vector<double> x, int p) {
    int n = x.size();
    // 初始Z
    vector<vector<double>> Z(n-p, vector<double>(p));
    for (int i = 0; i < n-p; i++) {
        for (int j = 0; j < p; j++) {
            Z[i][j] = x[p - 1 + i - j];
        }
    }
    // // 初始V
    vector<vector<double>> V(n-p, vector<double>(1));
    for (int i = 0; i < n-p; i++) {
       V[i][0] = x[i + p - 1];
    }
    return twoToOne(multiplication(inverse(multiplication(matrixTranspose(Z), Z)), multiplication(matrixTranspose(Z), V)));
}


// 根据AR获得初步预测
bool ar(vector<double> x, vector<double> &y, vector<double> parameter, int p, int n) {
    bool flag = false;
    for (int i = p; i < n; i++) {
        double temp = 0.0;
        for (int j = 0; j < p; j++) {
            temp += parameter[j] * x[i-j-1];
        }
        double t = abs(temp - x[i]);
        if (t > 0.1 && t < 0.5) {
            y[i] = temp;
            flag = true;
        }
    }
    return flag;
}

// 写json文件
bool writeFileJson(vector<double> x, vector<int> p, double inconsistency, string writeFileName) {
    Json::Value root;
    int n = x.size();
    for (int i = 0; i < n; i++) {
        root["value"][i].append(x[i]);
        root["value"][i].append(p[i]);
    }
    root["inconsistency"].append(inconsistency);
    
    ofstream os;
    os.open(writeFileName, ios::out | ios::trunc); //打开并指定文件以输出方式打开（往文件里写）
	if (!os.is_open()) {               
		cout << "error: can not find or create the file which named" + writeFileName << endl;
        return false;
    }
    Json::StyledWriter sw; 
	os << sw.write(root);       //输出到文件  
	os.close();                 //关闭文件
    return true;
}

// 传输
bool transportJson(vector<double> x, vector<int> p) {
    // 写json对象
    Json::Value root;
    for (double i : x) {
        root["value"].append(i);
    }
    for (int i = 0; i < p.size(); i++) {
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
    // 192.168.3.154
    // 结构体
    struct sockaddr_in severAddr;
    bzero(&severAddr, sizeof(severAddr));
    severAddr.sin_family = AF_INET;
    //将点分十进制串转换成网络字节序二进制值，此函数对IPv4地址和IPv6地址都能处理。
    inet_pton(AF_INET, severIp, &severAddr.sin_addr);
    //端口，暂设为8121
    severAddr.sin_port = htons(8888);
    
    // 链接函数
    int status = connect(socketId, (struct sockaddr *)&severAddr, sizeof(severAddr));
    if (status == -1) {
        cout << "Failed to connect" << endl;
        return false;
    }
    string tmp = sw.write(root);
    char *sendBuf = new char[tmp.length()+1];
    strcpy(sendBuf, tmp.c_str());
    int n = strlen(sendBuf);
    status = send(socketId, &sendBuf, n, 0);
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



int main(int argc, char* argv[])
{
    
    cmdline::parser parser;

	parser.add<string>("trainFileName", 't', "train file name", true);
	parser.add<string>("actualFileName", 'a', "actual file name", true);
    parser.add<string>("writeFileName", 'f', "write file name", false, "liangm_workplace/imr");
	parser.add<int>("epoch", 'e', "epoch num", false, 1000);
    parser.add<int>("window", 'w', "window size", false, 5);
	parser.add<double>("threshold", 's', "threshold", false, 0.1);
    
	parser.parse_check(argc, argv);

    int window = parser.get<int>("window");
    int max_num_iterations = parser.get<int>("epoch");
    string writeFileName = parser.get<string>("writeFileName") + ".json";
    // 训练集
    ifstream trainData(parser.get<string>("trainFileName") + ".csv", ios::in);
    if (!trainData.is_open()) {
        cout << "trainData open failed!" << endl;
    }
    vector<double> x;
    string temp;
    while (getline(trainData, temp)) {
        x.push_back(atof(temp.c_str()));
    }
    // 训练得到参数
    cout<<"   /*** start training ***/"<<endl;
    vector<double> parameter = getParameter(x, window);
    cout<<"   /*** finish training ***/"<<endl << "#########################" << endl;
    x.clear();

    cout << "   /*** start detecting ***/" << endl;
    // 实际集
    ifstream testData(parser.get<string>("actualFileName") + ".csv", ios::in);
    if (!testData.is_open()) {
        cout << "testData open failed!" << endl;
    }

    while (getline(testData, temp)) {
        x.push_back(atof(temp.c_str()));
    }
    int n = x.size();
    vector<double> y(x);
    // 无变化
    if (!ar(x, y, parameter, window, n)) {
        cout << "No abnormal data" << endl;
        return 1;
    }
    cout << "          IMR" << endl;
    double threshold = parser.get<double>("threshold");
    vector<double> res = IMR(x, y, max_num_iterations, window, n);
    vector<int> output(n, 0);
    int errorSum = 0;//
    for (int i = 0; i < n; i++) {
        double t = abs(res[i] - x[i]);
        if (t > threshold) {
            cout << "        error:" << i+1 << endl;
            output[i] = 1;
	        errorSum++;
        }
    }
    cout << "   /*** finish detecting ***/" << endl;
    //  不一致度
	double inconsistency = (double) errorSum/(double) n;
	cout << "Inconsistency:		 " << inconsistency << endl;
	
	writeFileJson(x, output, inconsistency, writeFileName);
	//transportJson(x, output);
    

    return 0;
}

