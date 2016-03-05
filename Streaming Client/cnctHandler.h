/*--Author：李宏杰--*/

#include "CommonHeaders.h"

/*
	cnctHandler：客户端网络连接模块，使用Winsock与服务器建立连接，并保存连接信息
*/

/*
	服务器列表结构：serverList

	描述：
	功能为从文件中读取服务器列表，服务于大的网络连接模块。
	设计思路就是通过配置文件来管理服务器信息，若服务器信息有更新，直接从网站或者其它方式替换该配置文件即可。
	使用双向链表来从文件中读取到的存储服务器信息。
	配置文件有相应的标准，详见【我的论文】

	Todo：配置文件标准搁到网上
*/
typedef struct serverList
{
	//每一行存储一个参数
	//ServerArgs代表参数数量，详细的参数见公共头中的常量表：ServerInfoDefine
	string srvArgs[ServerArgc];      

	serverList *prev;
	serverList *next;		
};

typedef struct threadParam
{
	SOCKET socket;
};
/*
	cnctHandler类：提供客户端网络连接管理

	描述：
	基于Winsock实现
	首先是初始化，在构造时设定默认参数为本地服务器参数，并读取本地配置文件，建立服务器信息结构，以双向链表形式存储
	之后便可调用connnectServer函数连接服务器

	使用：

	int connectServer()：连接服务器，成功返回0，否则为-1（代表无可用服务器）或其它值（能连接上但出错，此时代表connect函数返回的错误码，参见MSDN）

	void showSrvInfo():在控制台显示当前使用的服务器信息

*/
class cnctHandler
{
public:

	WSADATA wsaData;

	static cnctHandler *getInstance();

	int connectServer();             

	void showSrvInfo();         

	static void sendMessage(string msg);

	static void getRecvMessage(string &msg);

	~cnctHandler();

private:
	
	//连接成功后调用，开启接收与发送的线程
	void startThread();

	//消息队列

	static queue<string> recvMsgQueue;

	static queue<string> sendMsgQueue;

	static DWORD WINAPI sendThread(LPVOID lparam);

	static DWORD WINAPI recvThread(LPVOID lparam);

	/*
		服务器配置表处理部分
	*/

	serverList *mySrvList;  

	//配置文件的文件名
	string fileName;    

	//获取对应参数，输入下标（常量，见公共头），返回结构体字符串数组中对应下标的字符串值
	string getCfgByIndex(int index);   

	//填写内容
	void getLabelMsg(string name, string buf);    

	//读取配置文件
	bool readFile();  

	/*
		当前正在使用的服务器信息
	*/

	SOCKET srvSocket;

	//地址结构（ipv4/6，IP，端口）
	sockaddr_in srvAddr;   

	//完整的播放地址，eg. http://www.rayion.com/desktop
	string displayAddr;        

	/*
		以下为单例模式相关
	*/

	//构造函数，输入为存储有服务器信息的XML格式配置文件
	cnctHandler(string file = srvSettingFile);

	//仅在构造函数调用，完成构造工作
	void defaultSettings();

	static cnctHandler *instance; 

	//禁止拷贝构造以及赋值
	cnctHandler(const cnctHandler &);
	cnctHandler & operator = (const cnctHandler &);

	//析构处理
	class CGarbo
	{
	public:
		~CGarbo()
		{
			if (instance)delete instance;
		}
	};
	static CGarbo Garbo;
};

//网络模块：标记有消息需要发送
static HANDLE hsNewSendMsg = CreateSemaphore(NULL, 0, BUF_SIZE, NULL);

//网络模块：标记接收到了新的消息，至于是控制还是RTSP，给中间件解决好了
static HANDLE hsNewRecvMsg = CreateSemaphore(NULL, 0, BUF_SIZE, NULL);