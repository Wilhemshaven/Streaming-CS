#include "Server.h"
#include "myClient.h"
#include "rtspHandler.h"

// 专门输出前置提示信息
myMessage::myMessage()
{
	cout << "================================================" << endl;
	cout << "=                                              =" << endl;
	cout << "=     交互式流媒体序列帧传输平台，版本v0.1     =" << endl;
	cout << "=            143020085211001 李宏杰            =" << endl;
	cout << "=                                              =" << endl;
	cout << "================================================" << endl;
	cout << endl << "--初始化中..." << endl;
	cout << "--读入配置文件..." << endl;
};
const myMessage myMsg;

UINT rtspHandleThread();              //rtsp交互线程

//这里还要仔细考虑，现在更多的是一次性程序的感觉。
//TODO：增加复位功能
HANDLE hCloseClientEvent = CreateEvent(NULL, TRUE, FALSE, NULL);      //事件：关闭客户端，初值为NULL
HANDLE hRTSPBeginEvent = CreateEvent(NULL, TRUE, FALSE, NULL);        //事件：启动RTSP收发
HANDLE hBeatStartEvent = NULL;                                        //事件：启动RTSP心跳线程，初值为NULL
HANDLE sendRecvMutex = NULL;             //rtsp收发互斥信号量

//================================= MAIN =================================//
int main(int argc, char *argv[])
{
	/*------------------------------建立连接--------------------------*/
	//初始化Winsock
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	//获得服务器实例
	Server *mySrv = Server::getInstance();

	//Connect to server
	if (mySrv->connectServer() == 0)
	{
		/*------------------------------RTSP交互--------------------------*/
		//这个必然是要放在子线程里了
		//所以首先新建两个线程。一个是控制交互，一个是心跳（作为交互的子线程就好），维持连接的
		//线程不需要参数了，单例里面要什么有什么
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)rtspHandleThread, NULL, NULL, NULL);          //rtsp交互线程
	}
	else
	{
		//连接服务器失败
	}

	WaitForSingleObject(hCloseClientEvent, INFINITE);
	system("pause");
	return 0;
}

//RTSP控制线程
UINT rtspHandleThread()
{
	//子线程
	UINT sendMsgThread();       //发送信息
	UINT recvMsgThread();       //接收信息
	UINT rtspHeartBeat();       //心跳
	
	//获取单例
	Server *mySrv = Server::getInstance();
	rtspHandler *rtsp = rtspHandler::getInstance();

	//并用服务器信息设置rtsp处理器
	rtsp->setHandler(mySrv->getDisplayAddr(), MAKEWORD(1, 0), mySrv->getStreamPort(), true);

	//设置Socket接收超时，避免阻塞太久（嗯当然我们现在还是用阻塞模式，客户端么
	int recvTimeMax = 5000;  //5s
	//setsockopt(mySrv->getSocket(), SOL_SOCKET, SO_RCVTIMEO, (char *)recvTimeMax, sizeof(int));

	//启动收发和心跳线程
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)sendMsgThread, NULL, NULL, NULL);
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)recvMsgThread, NULL, NULL, NULL);
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)rtspHeartBeat, NULL, NULL, NULL);     //这个放在Play以后进行
	
	//启动线程
	SetEvent(hRTSPBeginEvent);

	//等待结束
	WaitForSingleObject(hCloseClientEvent, INFINITE);

	CloseHandle(sendMsgThread);
	CloseHandle(recvMsgThread);
	CloseHandle(rtspHeartBeat);

	return 0;
}

//RTSP信令发送线程
void sendMsgInThread(int msgType); //发送信令线程内的函数（一个小封装，编码->发送->错误处理）
UINT sendMsgThread()
{
	//获取单例
	Server *mySrv = Server::getInstance();
	rtspHandler *rtsp = rtspHandler::getInstance();

	sendRecvMutex = CreateMutex(NULL, TRUE, NULL);          //收发可以使用互斥信号量，并设置超时
	WaitForSingleObject(hRTSPBeginEvent,INFINITE);          //等待开始指令    

	string sendBuf, recvBuf;        //收发缓存
	int bytesSent;                  //发送的字节数

	sendMsgInThread(DESCRIBE);      //首先发送DESCRIBE信令

	//互斥量
	ReleaseMutex(sendRecvMutex);
	Sleep(1000);                    //只是为了方便测试，下同
	WaitForSingleObject(sendRecvMutex, INFINITE);

	sendMsgInThread(SETUP);             //然后发送Setup信令

	//互斥量
	ReleaseMutex(sendRecvMutex);
	Sleep(1000);
	WaitForSingleObject(sendRecvMutex, INFINITE);

	sendMsgInThread(PLAY);                                   //然后是Play指令
	hBeatStartEvent = CreateEvent(NULL, TRUE, FALSE, NULL);  //创建心跳事件

	//互斥量
	ReleaseMutex(sendRecvMutex);
	Sleep(1000);
	WaitForSingleObject(sendRecvMutex, INFINITE);

	//等待结束指令，挂起线程（不需要再发什么了）
	WaitForSingleObject(hCloseClientEvent, INFINITE);
	ReleaseMutex(sendRecvMutex);

	sendMsgInThread(TEARDOWN);//TEARDOWN，断开连接

	return 0;
}

//发送信令线程内的函数（一个小封装，编码->发送->错误处理）
void sendMsgInThread(int msgType)
{
	//获取单例
	Server *mySrv = Server::getInstance();
	rtspHandler *rtsp = rtspHandler::getInstance();

	string sendBuf;        //收发缓存
	int bytesSent;         //发送的字节数

	sendBuf = rtsp->encodeMsg(msgType);
	bytesSent = send(mySrv->getSocket(), sendBuf.c_str(), sendBuf.length(), NULL);

	if (bytesSent == 0)
	{
		//发送失败
		cout << "Error in sending msg:" << sendBuf << endl;
		SetEvent(hCloseClientEvent);           //目前采用的方式是直接关闭客户端；TODO：改成切换下一个服务器
	}
	else
	{
		//发送成功，报出相关信息
		cout << "Bytes sent:" << bytesSent << endl;
		cout << "Msg sent:" << endl << sendBuf << endl;
	}
}

//RTSP信令接收线程
UINT recvMsgThread()
{
	//获取单例
	Server *mySrv = Server::getInstance();
	rtspHandler *rtsp = rtspHandler::getInstance();

	string recvBuf;        //收发缓存
	int errCode;           //错误代码
	
	WaitForSingleObject(hRTSPBeginEvent, INFINITE);         //等待线程开始事件

	while (1)
	{
		WaitForSingleObject(sendRecvMutex, INFINITE);       //等待收发互斥量（一收一发）
		WaitForSingleObject(hCloseClientEvent, 0);          //检查结束事件是否已被设置

		recvBuf.clear();
		recvBuf.resize(BUF_SIZE);

		//接收答复（目前通过设置超时避免阻塞过久）
		recv(mySrv->getSocket(), (char *)recvBuf.data(), recvBuf.length(), NULL);
		recvBuf = recvBuf.substr(0, recvBuf.find('\0'));         //紧缩长度
		errCode = rtsp->decodeMsg(recvBuf);
		if (errCode != 200)
		{
			cout << "Error:" << rtsp->getErrMsg(errCode) << endl;
			SetEvent(hCloseClientEvent);   //设置一个结束事件，结束，看是结束所有还是什么
			ReleaseMutex(sendRecvMutex);
			break;
		}
		else
		{
			cout << "Msg recv: " << endl << recvBuf << endl;

			//如果心跳事件被创建（发送了PLAY），这里OK，就设置启动
			if (hBeatStartEvent != NULL)SetEvent(hBeatStartEvent);   
		}

		ReleaseMutex(sendRecvMutex);
		Sleep(1000);               //便于测试
	}

	return 0;
}

//RTSP心跳线程，30秒发送一次GET_PARAMETER信令直到结束客户端事件被设置
//已完成
UINT rtspHeartBeat()
{
	//等待启动信号
	WaitForSingleObject(hBeatStartEvent, INFINITE);

	//获取单例
	Server *mySrv = Server::getInstance();
	rtspHandler *rtsp = rtspHandler::getInstance();

	//计时器相关
	HANDLE heartBeatTimer = CreateWaitableTimer(NULL, FALSE, NULL);     //创建计时器，自动周期
	LARGE_INTEGER timeCnt;
	timeCnt.QuadPart = 0;                                               //0s后启动计时器（值负数）
	SetWaitableTimer(heartBeatTimer, &timeCnt, 30000, NULL, NULL, 0);   //第三个数为周期，单位毫秒.一般的流媒体系统60s超时
	
	string sendBuf;        //收发缓存
	int errCode;           //错误代码

	while (1)
	{
		if (WaitForSingleObject(hCloseClientEvent, 0))break;     //检查是否需要结束	 

		if (WaitForSingleObject(heartBeatTimer, 1000))           //每秒检查一次
		{
			sendBuf = rtsp->encodeMsg(GET_PARAMETER);
			errCode = send(mySrv->getSocket(), sendBuf.c_str(), sendBuf.size(), NULL);

			if (errCode != 200)
			{
				//如果出错
				cout << "心跳发送出错，错误代码：" << errCode << " " << rtsp->getErrMsg(errCode) << endl;
			}

			//不用收取回复了
		}	
	}

	return 0;
}