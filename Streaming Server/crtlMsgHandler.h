/*--Author：李宏杰--*/

#pragma once
#include "CommonHeaders.h"

/*
	服务端控制信令处理器，单例模式

	输入待解码的信令，然后解码
	最后把指令交给对应中间件传递给渲染器

	使用：
	
	void decodeMsg(string msg)：解码信令，输入待解码信令即可，不需要关心后续处理

	char getKBCtrlKey()：获取键盘操作情况

	char getMouseCtrlKey();获取鼠标操作情况

*/

class ctrlMsgHandler
{
public:

	static ctrlMsgHandler* getInstance();

	void decodeMsg(int msgType, string msg);

	char getKBCtrlKey();

	char getMouseCtrlKey();

private:

	queue<char> KBQueue;

	/*
		单例模式
	*/

	static ctrlMsgHandler *instance;
	ctrlMsgHandler();

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
static HANDLE hsKBCtrlMsg = CreateSemaphore(NULL, 0, BUF_SIZE, NULL);