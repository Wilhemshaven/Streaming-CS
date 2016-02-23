/*--Author：李宏杰--*/

#include "cnctHandler.h"

cnctHandler *cnctHandler::instance = new cnctHandler(srvSettingFile);

/*
	构造函数（传入文件名为参数，否则读取公共头中指定的文件路径）
*/
cnctHandler::cnctHandler(string file)
{
	fileName = file;

	defaultSettings();
}

cnctHandler::cnctHandler()
{
	fileName = srvSettingFile;

	defaultSettings();
}

/*
	默认设置：
	1、启动Winsock
	2、把服务器信息设为本地（LOCALHOST）
	3、读取服务器配置文件
*/
void cnctHandler::defaultSettings()
{
	mySrvList = new serverList;

	WSAStartup(MAKEWORD(2, 2), &wsaData);
	srvSocket = socket(AF_INET, SOCK_STREAM, 0);

	srvAddr.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &srvAddr.sin_addr);
	srvAddr.sin_port = htons(80);

	//读入配置文件
	if (readFile())
	{
		cout << "读取配置文件成功！" << endl;
	}
	else
	{
		cout << "读取配置文件失败！" << endl;
	}
}

/*
	连接服务器：自动从头开始选择第一个可用的（能连接上）的服务器进行连接

	Todo：增加切换服务器功能
*/
int cnctHandler::connectServer()
{
	serverList *waitToCnct = mySrvList->next;

	//返回值：若成功会改为0，否则为-1（代表无可用服务器）或其它值（能连接上但出错，此时代表connect函数返回的错误码，参见MSDN）
	int isConnected = -1;

	//从服务器链表中读出相关信息，并挨个尝试连接
	while (waitToCnct != NULL)
	{
		//Tip. 必须先使用临时变量存数据去连接，Connect函数估计比较复杂，会因为权限问题无法连接的

		//拼接完整的连接路径字符串
		string serverTmp;
		serverTmp = getCfgByIndex(protocol) + "://" + getCfgByIndex(hostName) + "/" + getCfgByIndex(displayRoute);
		if (serverTmp.length() > 5)cout << "尝试连接服务器（75秒超时）：" << serverTmp << endl;

		/*
			解析域名
			Todo：这个不合法怎么解决呢？
		*/
		addrinfo *res;
		int errorCode = getaddrinfo(getCfgByIndex(hostName).c_str(), NULL, NULL, &res);

		if (errorCode != 0)
		{
			cout << "域名解析失败，非法域名。错误代码：" << WSAGetLastError() << endl;
		}
		else
		{
			/*
				填写结构信息，端口自己写（见全局公共头）
				Tips. sockaddr和sockaddr_in是一回事，只不过前者更通用（直接把几个属性凑成字符串了）
			*/
			sockaddr_in srvAddrTmp = *(sockaddr_in *)res->ai_addr;
			srvAddrTmp.sin_port = htons(stoi(getCfgByIndex(port), nullptr, 10));

			//解析域名，把解析出来的IP填入到服务器列表中该服务器信息的相应位置
			inet_ntop(srvAddrTmp.sin_family, &srvAddrTmp.sin_addr, (char *)waitToCnct->srvArgs[hostAddr].data(), 17); 

			/*
				连接服务器。这里是可能超时的，默认超时时间是75秒
				Todo：万一真的阻塞了，这可不行啊，不能让人等很久很久啊
			*/
			SOCKET socketTmp = socket(AF_INET, SOCK_STREAM, 0);
			isConnected = connect(socketTmp, (sockaddr *)&srvAddrTmp, sizeof(srvAddrTmp));

			//在控制台输出相应信息
			if (isConnected == 0)
			{
				cout << "--连接成功" << endl;

				srvAddr = srvAddrTmp;
				displayAddr = serverTmp;
				srvSocket = socketTmp;

				break;
			}
			else
			{
				cout << "--连接失败，Winsock错误代码：" << WSAGetLastError() << endl;

				//切换下一组服务器，直到无备选服务器（指针为空）
				waitToCnct = waitToCnct->next;  
			}
		}
	}

	if (isConnected != 0)
	{
		//如果一组都不成功，就输出无可用服务器
		cout << "无可用服务器." << endl;
	}

	return isConnected;
}

cnctHandler* cnctHandler::getInstance()
{
	return instance;
}

SOCKET cnctHandler::getSocket()
{
	return srvSocket;
}

string cnctHandler::getDisplayAddr()
{
	return displayAddr;
}

int cnctHandler::getStreamPort()
{
	return srvAddr.sin_port;
}

serverList* cnctHandler::getSrvStruct()
{
	return mySrvList;
}

void cnctHandler::showSrvInfo()
{
	//在控制台显示当前使用的服务器信息
	cout << "----当前使用的服务器信息----" << endl;
	cout << "服务器类型：" << getCfgByIndex(srvType) << endl;
	cout << "播放地址：" << getCfgByIndex(displayRoute) << endl;
	cout << "IP地址：" << getCfgByIndex(port) << endl;
	cout << "端口：" << getCfgByIndex(srvType) << endl;
	cout << endl;
}

cnctHandler::~cnctHandler()
{
	closesocket(srvSocket);
}

/*
	读取配置文件，返回值代表是否成功读取
*/
bool cnctHandler::readFile()
{
	//以只读文件流形式打开文件
	fstream srvSettings;
	srvSettings.open(fileName, ios_base::in); 

	if (!srvSettings.is_open())
	{
		//文件未打开或未找到，报错
		cout << "找不到配置文件： " << srvSettingFile << " !" << endl;
		return false;
	}
	else
	{
		string buf;    //输入缓存
		string label;  //标签中的内容

		//读到文件结尾
		//Todo：以后可考虑从特定位置读起/保留当前读取状态/直接存到内存之类的处理方法
		while (!srvSettings.eof())
		{
			//Tips. 这里一次要读一整行的，避免有空格之类坑爹的东西
			getline(srvSettings, buf);

			//1、解析服务器组信息
			if (buf.find("<serverList>") != string::npos)
			{
				//只要是服务器就全部读进来，创建一个服务器链表
				serverList *newServer = mySrvList, *nextServer = NULL;

				while (buf.find("</serverList>") == string::npos)
				{
					getline(srvSettings, buf);

					if (buf.find("<server>") != string::npos)
					{
						//读取单个服务器的信息
						nextServer = new serverList;

						while (buf.find("</server>") == string::npos)
						{
							//取出标签中的内容
							getline(srvSettings, buf);
							label = buf.substr(buf.find('<') + 1, buf.find('>') - buf.find('<') - 1);  

							//根据标签中的内容填充结构体
							getLabelMsg(label, buf);
						}

						//添加新的服务器节点，建立双向链表
						newServer->next = nextServer;
						nextServer->prev = newServer;
						nextServer = nextServer->next;
						newServer = newServer->next;
					}
				}
			}

			//2、解析其它（待添加）
			//Todo：...
		}
	}

	return true;
};

/*
	获取XML标签里的内容，填到相应的变量中

	结构体是一个字符串数组嘛，这里就是根据标签指定数组下标

	这里要根据常量表添加修改了，似乎无法对修改封闭啊
	所以……要遵从设计好的协议！
*/
void cnctHandler::getLabelMsg(string name, string buf)
{
	int index = 0;

	if (name == "type")index = 1;
	if (name == "protocol")index = 2;
	if (name == "hostname")index = 3;
	if (name == "port")index = 4;
	if (name == "display")index = 5;

	//提取标签中的内容
	mySrvList->srvArgs[index] = buf.substr(buf.find('>') + 1, buf.rfind('<') - buf.find('>') - 1);
}

//返回服务器信息结构体中对应下标的属性值
string cnctHandler::getCfgByIndex(int index)
{
	return mySrvList->srvArgs[index];
}