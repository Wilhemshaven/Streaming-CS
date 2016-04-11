/*--Author������--*/

#include "imageBuffer.h"

//ý�建��ģ�飺�����ͼ���ˣ���ȡ�߲���
HANDLE hsBufferOutput = CreateSemaphore(NULL, 0, BUF_SIZE, syncManager::bufferOutput);

imgBuffer *imgBuffer::instance = new imgBuffer;

imgBuffer *imgBuffer::getInstance()
{
	return instance;
}

imgBuffer::imgBuffer()
{

}

void imgBuffer::pushBuffer(imgHead head, vector<char> img)
{
	myImage image;

	image.head = head;
	image.img = img;

	imageQueue.push(image);

	ReleaseSemaphore(hsBufferOutput, 1, NULL);
}

bool imgBuffer::popBuffer(imgHead &head, vector<char> &img)
{
	if (imageQueue.empty())
	{
		return false;
	}
	
	myImage image = imageQueue.front();

	imageQueue.pop();

	head = image.head;

	img = image.img;

	return true;
}