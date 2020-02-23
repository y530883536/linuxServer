#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define DEV_INPUT_event0 "/dev/input/event0"

//type
#define EV_ABS 0x03
#define EV_KEY 0x01
#define EV_SYN 0x00

//code
#define ABS_MT_TRACKING_ID 0x39
#define ABS_MT_POSITION_X 0x35
#define ABS_MT_POSITION_Y 0x36
#define ABS_MT_SLOT 0x2f
#define BTN_TOUCH 0x14a
#define SYN_REPORT 0

//value
#define BTN_KEY_DOWN 1
#define BTN_KEY_UP 0
//BTN_TOUCH的UP事件的ABS_MT_TRACKING_ID总是ffffffff
#define BTN_UP_TRACKING_ID 0xFFFFFFFF
//SYN_REPORT事件的值总是00000000
#define SYN_REPORT_VALUE 0

int PRESS_DOWN = 1;
int PRESS_UP = 0;
int TRACKING_END = 0xFFFFFFFF;

int g_fd = 0;

int connect_dev()
{
	int version=0;
	int g_fd = open(DEV_INPUT_event0, O_RDWR);
	if(g_fd < 0) {
        printf( "could not open %s\n", DEV_INPUT_event0);
        return 0;
    }
	if (ioctl(g_fd, EVIOCGVERSION, &version)) {
        printf( "could not get driver version \n");
        return 0;
    }
	printf("get dev!\n");
	return 1;
}

void sendEvent(int type, int code , int value)
{
	struct input_event event;
	ssize_t ret;
	
	memset(&event, 0, sizeof(event));
    event.type = type;
    event.code = code;
    event.value = value;
    ret = write(g_fd, &event, sizeof(event));
    if(ret < (ssize_t) sizeof(event)) {
        printf( "write event failed\n");
        return ;
    }
    return;

}

//int trackingId = 0;
/*int parseTrackingId() {
	return ++trackingId;
}*/


void swipeStart() {
	printf("start\n");
	//sendEvent(EV_ABS, ABS_MT_TRACKING_ID, parseTrackingId());
	sendEvent(EV_KEY, BTN_TOUCH, PRESS_DOWN);
	sendEvent(EV_KEY, BTN_TOOL_FINGER, PRESS_DOWN);
}

void swipeEnd() {
	printf("end\n");
	sendEvent(EV_ABS, ABS_MT_TRACKING_ID, TRACKING_END);
	sendEvent(EV_KEY, BTN_TOUCH, PRESS_UP);
	sendEvent(EV_KEY, BTN_TOOL_FINGER, PRESS_UP);
	sendEvent(EV_SYN, SYN_REPORT, 0);
}


int SWIPE_RUN_INTERVAL = 50;
void swipeRun(int x1, int y1, int x2, int y2) 
{
	int xStep = (x2 - x1) / SWIPE_RUN_INTERVAL;
	int yStep = (y2 - y1) / SWIPE_RUN_INTERVAL;

	int x = x1, y = y1;
	for (int step = 0; step <= SWIPE_RUN_INTERVAL; ++step) {
		sendEvent(EV_ABS, ABS_MT_POSITION_X, x);
		sendEvent(EV_ABS, ABS_MT_POSITION_Y, y);
		sendEvent(EV_SYN, SYN_REPORT, 0);
		x += xStep;
		y += yStep;
	}
}

  ;
  __u16 code;
  __s32 value;

//触摸屏的event文件
int fd = -999;
void fuckSend( __u16 type, __u16 code , __s32 value)
{
	//int fd;
    ssize_t ret;
    int version;
    struct input_event event;

    //如果是第一次就要打开文件
    if(fd == -999) {
        fd = open("/dev/input/event0", O_RDWR);
    }
    if(fd < 0 && fd != -999) {
        printf("errno=%d\n",errno);
        char * mesg = strerror(errno);
        printf("Mesg:%s\n",mesg);
        printf("open fail\n");
        return;
    }
    if (ioctl(fd, EVIOCGVERSION, &version)) {
        printf("ioctl fail!\n");
        return;
    }
    memset(&event, 0, sizeof(event));
    event.type = type;
    event.code = code;
    event.value = value;
    ret = write(fd, &event, sizeof(event));
    if(ret < (ssize_t) sizeof(event)) {
        printf("write event failed\n");
        //close(fd);
        return ;
    }
    //close(fd);
    return ;
}

//最大支持十指操作
//上个指令的手指x坐标
int lastFingerArrayX[10] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
//上个指令的手指y坐标
int lastFingerArrayY[10] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
int trackingId = 0;
//上个指令的手指触摸数
int lastTouchNum = 0;
//上一个有操作的slotId
int lastOperateSlotId = -1;

//坐标转换的时候要考虑x轴和y轴转换的情况
//Y轴的坐标变成x轴的坐标，x轴的坐标变成720-Y轴的坐标
//正常的屏幕分辨率都是1920*1080，但是模拟器是1080*720的，所以x坐标和y坐标都要*1.5

//一根手指按下事件，i是slotId，第一根手指是0，第二根手指是1，依此类推
void fingerDown(int i,int xAxis,int yAxis) {
    if(lastOperateSlotId != i) {
        fuckSend(EV_ABS,ABS_MT_SLOT,i);
        lastOperateSlotId = i;
    }
    fuckSend(EV_ABS,ABS_MT_TRACKING_ID,trackingId);
    //trackingId每次使用完要加一
    trackingId ++;
    lastFingerArrayX[i] = xAxis;
    lastFingerArrayY[i] = yAxis;
    fuckSend(EV_ABS,ABS_MT_POSITION_X,xAxis);
    fuckSend(EV_ABS,ABS_MT_POSITION_Y,yAxis);
    //第一个要触发DOWN事件
    if(i == 0) {
        fuckSend(EV_KEY,BTN_TOUCH,BTN_KEY_DOWN);
    }
    fuckSend(EV_SYN,SYN_REPORT,SYN_REPORT_VALUE);
}

//所有手指抬起事件（没办法做一根手指抬起事件）
void fingerUp() {
    for(int i=0;i<lastTouchNum;i++) {
        if(lastOperateSlotId != i) {
            fuckSend(EV_ABS,ABS_MT_SLOT,i);
            lastOperateSlotId = -1;
        }
        fuckSend(EV_ABS,ABS_MT_TRACKING_ID,BTN_UP_TRACKING_ID);
        //最后一个要触发UP事件
        if(i == lastTouchNum-1) {
            fuckSend(EV_KEY,BTN_TOUCH,BTN_KEY_UP);
        }
    }
    fuckSend(EV_SYN,SYN_REPORT,SYN_REPORT_VALUE);
    //把x轴坐标，y轴坐标和slotId复位
    for(int i=0;i<10;i++) {
        lastFingerArrayX[i] = -1;
        lastFingerArrayY[i] = -1;
    }
    lastTouchNum = 0;
    lastOperateSlotId = -1;
}

void changeToAndroidInstruct(char *originInst){
    //200000us是200ms也就是0.2s
    //usleep(20000);

    char temp[100];
    for(int i=0;i<100;i++) {
        temp[i] = *(originInst + i);
        if(temp[i] == '\0') {
            break;
        }
    }

    char *originInstArray[100];
    char *buf;
    buf = temp;
    char *token;
    int index = 0;
    while((token = strsep(&buf,":")) != NULL) {
        originInstArray[index] = token;
        index ++;
    }
    //触屏数量
    int touchNum = atoi(originInstArray[1]);
    //触屏类型：0（按下）  1（抬起）  2（移动）
    int touchType = atoi(originInstArray[2]);
    //按下事件
    if(touchType == 0) {
        for(int i=0;i<touchNum;i++) {
            //int xAxis = (int)(atoi(originInstArray[i*2+3]) * 1.5);
            //int yAxis = (int)(atoi(originInstArray[i*2+4]) * 1.5);
            int xAxis = (int)((720-atoi(originInstArray[i*2+4])) * 1.5);
            int yAxis = (int)(atoi(originInstArray[i*2+3]) * 1.5);
            fingerDown(i,xAxis,yAxis);
        }
        lastTouchNum = touchNum;
    }
    //抬起事件
    if(touchType == 1) {
        fingerUp();
    }
    //移动事件
    if(touchType == 2) {
        //如果当前指令的触屏数量不等于上个指令的触屏数量，重新初始化一次
        if(touchNum >= lastTouchNum) {
            //手指增加的情况，原来的手指继续移动
            for(int i=0;i<lastTouchNum;i++) {
                //int xAxis = (int)(atoi(originInstArray[i*2+3]) * 1.5);
                //int yAxis = (int)(atoi(originInstArray[i*2+4]) * 1.5);
                int xAxis = (int)((720-atoi(originInstArray[i*2+4])) * 1.5);
                int yAxis = (int)(atoi(originInstArray[i*2+3]) * 1.5);
                int lastFingerX = lastFingerArrayX[i];
                int lastFingerY = lastFingerArrayY[i];
                //如果和上次手指位置是一样的，不需要触发任何事件
                if(lastFingerX == xAxis && lastFingerY == yAxis) {
                    continue;
                }
                //如果上次操作的slotId不是当前slotId，要加上一句切换slotId的指令
                if(lastOperateSlotId != i) {
                    fuckSend(EV_ABS,ABS_MT_SLOT,i);
                    lastOperateSlotId = i;
                }
                if(lastFingerX != xAxis) {
                    fuckSend(EV_ABS,ABS_MT_POSITION_X,xAxis);
                }
                if(lastFingerY != yAxis) {
                    fuckSend(EV_ABS,ABS_MT_POSITION_Y,yAxis);
                }
            }
            //新增加的手指逻辑
            for(int i=lastTouchNum;i<touchNum;i++){
                //int xAxis = (int)(atoi(originInstArray[i*2+3]) * 1.5);
                //int yAxis = (int)(atoi(originInstArray[i*2+4]) * 1.5);
                int xAxis = (int)((720-atoi(originInstArray[i*2+4])) * 1.5);
                int yAxis = (int)(atoi(originInstArray[i*2+3]) * 1.5);
                fuckSend(EV_ABS,ABS_MT_SLOT,i);
                lastOperateSlotId = i;
                fuckSend(EV_ABS,ABS_MT_TRACKING_ID,trackingId);
                trackingId ++;
                lastFingerArrayX[i] = xAxis;
                lastFingerArrayY[i] = yAxis;
                fuckSend(EV_ABS,ABS_MT_POSITION_X,xAxis);
                fuckSend(EV_ABS,ABS_MT_POSITION_Y,yAxis);
            }
            fuckSend(EV_SYN,SYN_REPORT,SYN_REPORT_VALUE);
        }
        //如果抬起了手指，就要触发一次手指抬起和触摸事件
        else{
            fingerUp();
            for(int i=0;i<touchNum;i++) {
                //int xAxis = (int)(atoi(originInstArray[i*2+3]) * 1.5);
                //int yAxis = (int)(atoi(originInstArray[i*2+4]) * 1.5);
                int xAxis = (int)((720-atoi(originInstArray[i*2+4])) * 1.5);
                int yAxis = (int)(atoi(originInstArray[i*2+3]) * 1.5);
                fingerDown(i,xAxis,yAxis);
            }
        }
        //记录一下触屏数量
        lastTouchNum = touchNum;
    }
    //如果trackingId太大的话，把trackingId归0
    if(trackingId >= 0xfff) {
        trackingId = 0;
    }
    return;
}
