/*--Author：李宏杰--*/

#pragma once
#include "CommonHeaders.h"

/*
	服务端控制信令处理器，单例模式

	输入待解码的信令，然后解码
	最后把指令交给对应中间件传递给渲染器

	使用：
	
	void decodeMsg(string msg)：解码信令，返回会话号

	string encodeMsg(imgHead head, int imgSize, int session)：编码信令（图像头）

	char getCtrlKey()：获取操作情况

*/

class ctrlMsgHandler
{
public:

	static ctrlMsgHandler* getInstance();

	int decodeMsg(string msg);

	string encodeMsg(imgHead head, int imgSize, int session);

	char getCtrlKey();

private:

	//编码公共头
	string encodePublicHead(int payloadType, int session, int size);

	queue<char> ctrlKeyQueue;

	/*
		单例模式
	*/

	ctrlMsgHandler();

	static ctrlMsgHandler *instance;
	

	ctrlMsgHandler(const ctrlMsgHandler&);
	ctrlMsgHandler &operator=(const ctrlMsgHandler&);
	class CGarbo
	{
	public:
		~CGarbo()
		{
			if (instance)delete instance;
		}
	};
	static CGarbo garbo;
};

//控制信令处理模块：标记信令解码完毕，请中间件拿走转给渲染器
static HANDLE hsCtrlMsg = CreateSemaphore(NULL, 0, BUF_SIZE, NULL);