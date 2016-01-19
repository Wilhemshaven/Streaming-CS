#pragma once
#include "CommonHeaders.h"

//RTSP������Ϣ������
class rtspErrHandler
{
public:
	rtspErrHandler();

	string getErrMsg(int code);      //���������Ϣ

private:
	string settingFile;
	void buildErrList();             //�����������ӱ����������ļ������ڻ������ݲ��ϸ�ʽ�ˣ�ֻ��Ϊ�˼��ٴ��������ѣ�Ҳ����

	map<int, string> errCodeList;    //�������ӱ�
};

//NTPʱ���ȡ��
class NTPTime
{
public:
	unsigned long getNTPTime();
	unsigned long getNTPTimeUnix();
};

//SDP������
class sdpEncoder
{
public:
	string encodeMsg();

	sdpEncoder();

private:
	NTPTime ntpTime;
	string sdpMsg;

	//--Session description
	//Require
	string proVer;
	string owner;
	string sessionName;
	//Option
	string sessionInfo;
	string URI;
	string emailAddr;
	string phoneNum;
	string cnctInfo;
	string bandwidthInfo;
	string timeZoneAdjust;
	string encryptKey;
	string moreSessionAttr;

	//Time description
	string timeActive;     //require
	string repeatTime;     //option

	//Media description
	string mediaNameAndAddr;     //require
	string mediaTitle;
	//string cnctInfo;             //optional if included at session-level
	//string bandwidthInfo;
	//string encryptKey;
	//string moreSessionAttr;
};

//һ�����Session��������,�������Ҿ���ֱ����ʱ����ͺ��ˡ�������һ��FFFFFFFF���㹻��
class sessionGenerator
{
private:
	string session;
	
	NTPTime ntpTime;

public:
	sessionGenerator();

	string getSessionID();
};

//RTSP��Ϣ�Ĵ���
//��������������������ͻ������űȽϴ������
//ÿ���ͻ��˵����󶼲�һ��������Ҫ�����һ�壨�������ɣ���Ȼ�Ҿ��û��ǿ��Բ𿪵ģ�
class rtspHandler
{
public:
	static rtspHandler *getInstance();

	string getErrMsg(int code);      //���ش����������ʾ����Ϣ
	string getHandlerInfo();         //��ȡrtsp�������������Ϣ

	//ֻ�ܱ���룬��������
	string msgCodec(string msg);     //�����һ�壬�����������Ϣ�����ر���õĴ�

private:
	static rtspHandler *instance;              //����
	rtspHandler();                             //���캯��

	string URI;                                //��ý���ַ
	string rtspVersion;                        //RTSP�汾

	rtspErrHandler errHandler;                 //������Ϣ������
	sdpEncoder sdpHandler;                     //SDP������

	vector<string> rtspMethod;                 //������rtsp����
	vector<bool> availableMethod;              //���õķ�����rtsp����

	//��ֹ���������Լ���ֵ
	rtspHandler(const rtspHandler &);
	rtspHandler & operator = (const rtspHandler &);

	//��������
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
