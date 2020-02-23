#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

//todo 这里之后动态确定触摸屏的event
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
        return ;
    }
    return ;
}

void clearArray(int array[],int size) {
    for(int i=0;i<size;i++) {
        array[i] = -1;
    }
}

//最大支持十指操作
int prevSlotIdArray[10] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
int prevArrayX[10] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
int prevArrayY[10] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
//trackingId，每次使用之后都加1
int trackingId = 0;
//上个指令的手指触摸数
int prevTouchNum = 0;
//上一个有操作的slotId
int prevOperateSlotId = -1;
//todo x轴坐标的放大倍率和y轴坐标的放大倍率读取实机的分辨率，这里先写死，之后再做动态匹配
float xMag = 1.5;
float yMag = 1.5;

//坐标转换的时候要考虑x轴和y轴转换的情况
//Y轴的坐标变成x轴的坐标，x轴的坐标变成720-Y轴的坐标
//正常的屏幕分辨率都是1920*1080，但是模拟器是1080*720的，所以x坐标和y坐标都要乘以倍率

void changeToAndroidInstruct(char *originInst){

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
    //x轴坐标数组
    int xPointArray[touchNum] = {};
    //y轴坐标数组
    int yPointArray[touchNum] = {};
    //本次指令最终的slotId数组
    int currSlotIdArray[10] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
    //本次指令最终的x坐标数组
    int currArrayX[10] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
    //本次指令最终的y坐标数组
    int currArrayY[10] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

    //初始化x轴坐标数组和y轴坐标数组
    //todo 在这里就要做坐标转换了，目前制作了手机往一边横屏的情况，之后可能要看情况加上自动识别横屏的逻辑
    //todo 而且这里的720是写死的，之后还要考虑怎么做坐标转换
    for(int i=0;i<touchNum;i++) {
        xPointArray[i] = (int)((720-atoi(originInstArray[i*2+4])) * 1.5);
        yPointArray[i] = (int)(atoi(originInstArray[i*2+3]) * 1.5);
    }

    //按下事件
    if(touchType == 0) {
        for(int i=0;i<touchNum;i++) {
            prevSlotIdArray[i] = i;
            prevArrayX[i] = xPointArray[i];
            prevArrayY[i] = yPointArray[i];
            currSlotIdArray[i] = i;
            currArrayX[i] = xPointArray[i];
            currArrayY[i] = yPointArray[i];
            //fingerDown(i,xAxis,yAxis);
            if(prevOperateSlotId != currSlotIdArray[i]) {
                fuckSend(EV_ABS,ABS_MT_SLOT,currSlotIdArray[i]);
                prevOperateSlotId = currSlotIdArray[i];
            }
            fuckSend(EV_ABS,ABS_MT_TRACKING_ID,trackingId);
            //trackingId每次使用完要加一
            trackingId ++;
            fuckSend(EV_ABS,ABS_MT_POSITION_X,currArrayX[i]);
            fuckSend(EV_ABS,ABS_MT_POSITION_Y,currArrayX[i]);
            //第一个要触发DOWN事件
            if(currSlotIdArray[i] == 0) {
                fuckSend(EV_KEY,BTN_TOUCH,BTN_KEY_DOWN);
            }
            fuckSend(EV_SYN,SYN_REPORT,SYN_REPORT_VALUE);
        }
        //prevTouchNum = touchNum;
    }
    //抬起事件
    if(touchType == 1) {
        for(int i=0;i<10;i++) {
            if(prevSlotIdArray[i] == -1) {
                continue;
            }
            if(prevOperateSlotId != prevSlotIdArray[i]) {
                fuckSend(EV_ABS,ABS_MT_SLOT,prevSlotIdArray[i]);
                prevOperateSlotId = prevSlotIdArray[i];
            }
            fuckSend(EV_ABS,ABS_MT_TRACKING_ID,BTN_UP_TRACKING_ID);
            //最后一个要触发UP事件
            /*if(i == prevTouchNum-1) {
                fuckSend(EV_KEY,BTN_TOUCH,BTN_KEY_UP);
            }*/
        }
        fuckSend(EV_KEY,BTN_TOUCH,BTN_KEY_UP);
        fuckSend(EV_SYN,SYN_REPORT,SYN_REPORT_VALUE);
        for(int i=0;i<10;i++) {
            prevSlotIdArray[i] = -1;
            prevArrayX[i] = -1;
            prevArrayY[i] = -1;
        }
        prevOperateSlotId = -1;
    }
    //移动事件
    if(touchType == 2) {
        //本次的手指触摸数量<=上次的手指触摸数量
        if(touchNum <= prevTouchNum) {
            //最外层循环是本次的指令坐标
            for(int i=0;i<touchNum;i++) {
                int isFirst = 1;
                int lastDifference = -1;
                int surviveSlotId = -1;
                //这一层的循环是上次的指令坐标
                for(int j=0;j<10;j++) {
                    if(prevSlotIdArray[j] == -1) {
                        continue;
                    }
                    int xDifference = abs(prevArrayX[j] - xPointArray[i]);
                    int yDifference = abs(prevArrayY[j] - yPointArray[i]);
                    int sumDifference = xDifference + yDifference;
                    if(isFirst == 1 || sumDifference <= lastDifference) {
                        lastDifference = sumDifference;
                        surviveSlotId = prevSlotIdArray[j];
                        isFirst = 0;
                    }
                }
                currSlotIdArray[surviveSlotId] = prevSlotIdArray[surviveSlotId];
                currArrayX[surviveSlotId] = xPointArray[i];
                currArrayY[surviveSlotId] = yPointArray[i];
            }
        }
        //本次的手指触摸数量>上次的手指触摸数量
        else{
            int isFirst = 1;
            int lastDifference = -1;
            for(int i=0;i<10;i++) {
                if(prevSlotIdArray[i] == -1) {
                    continue;
                }
                currSlotIdArray[i] = prevSlotIdArray[i];
                for(int j=0;j<touchNum;j++) {
                    int xDifference = abs(prevArrayX[i] - xPointArray[j]);
                    int yDifference = abs(prevArrayY[i] - yPointArray[j]);
                    int sumDifference = xDifference + yDifference;
                    if(isFirst == 1 || sumDifference <= lastDifference) {
                        lastDifference = sumDifference;
                        currArrayX[i] = xPointArray[j];
                        currArrayY[i] = yPointArray[j];
                        isFirst = 0;
                    }
                }
            }
            for(int k=0;k<10;k++) {
                if(currSlotIdArray[k] == -1) {
                    continue;
                }
                int currX = currArrayX[k];
                int currY = currArrayY[k];
                for(int l=0;l<touchNum;l++) {
                    if(xPointArray[l] == currX && yPointArray[l] == currY) {
                        xPointArray[l] = -1;
                        yPointArray[l] = -1;
                        break;
                    }
                }
            }
            for(int m=0;m<touchNum;m++) {
                if(xPointArray[m] == -1 && yPointArray[m] == -1) {
                    continue;
                }
                for(int n=0;n<10;n++) {
                    if(currSlotIdArray[n] == -1) {
                        currSlotIdArray[n] = n;
                        currArrayX[n] = xPointArray[m];
                        currArrayY[n] = yPointArray[m];
                        break;
                    }
                }
            }
        }
        //算出上次指令和这次指令之后，对比指令然后进行处理
        for(int i=0;i<10;i++) {
            //上次指令有坐标，这次指令有坐标，那么手指是按住了或者是移动中
            if(prevSlotIdArray[i] != -1 && currSlotIdArray[i] != -1) {
                //如果和上次手指位置是一样的，不需要触发任何事件
                if(prevArrayX[i] == currArrayX[i] && prevArrayY[i] == currArrayY[i]) {
                    continue;
                }
                //如果上次操作的slotId不是当前slotId，要加上一句切换slotId的指令
                if(prevOperateSlotId != currSlotIdArray[i]) {
                    fuckSend(EV_ABS,ABS_MT_SLOT,currSlotIdArray[i]);
                    prevOperateSlotId = currSlotIdArray[i];
                }
                if(prevArrayX[i] != currArrayX[i]) {
                    fuckSend(EV_ABS,ABS_MT_POSITION_X,currArrayX[i]);
                }
                if(prevArrayY[i] != currArrayY[i]) {
                    fuckSend(EV_ABS,ABS_MT_POSITION_Y,currArrayY[i]);
                }
            }
            //上次指令有坐标，这次指令没有坐标，那么就是手指离开了
            if(prevSlotIdArray[i] != -1 && currSlotIdArray[i] == -1) {
                if(prevOperateSlotId != prevSlotIdArray[i]) {
                    fuckSend(EV_ABS,ABS_MT_SLOT,prevSlotIdArray[i]);
                    prevOperateSlotId = prevSlotIdArray[i];
                }
                fuckSend(EV_ABS,ABS_MT_TRACKING_ID,BTN_UP_TRACKING_ID);
            }
            //上次指令没坐标，这次指令有坐标，那么就是增加手指了
            if(prevSlotIdArray[i] == -1 && currSlotIdArray[i] != -1) {
                fuckSend(EV_ABS,ABS_MT_SLOT,currSlotIdArray[i]);
                lastOperateSlotId = currSlotIdArray[i];
                fuckSend(EV_ABS,ABS_MT_TRACKING_ID,trackingId);
                trackingId ++;
                fuckSend(EV_ABS,ABS_MT_POSITION_X,currArrayX[i]);
                fuckSend(EV_ABS,ABS_MT_POSITION_Y,currArrayY[i]);
            }
        }
        fuckSend(EV_SYN,SYN_REPORT,SYN_REPORT_VALUE);
        //最后记录本次的手指指令
        for(int i=0;i<10;i++) {
            prevSlotIdArray[i] = currSlotIdArray[i];
            prevArrayX[i] = currArrayX[i];
            prevArrayY[i] = currArrayY[i];
        }
    }
    //记录一下触屏数量
    prevTouchNum = touchNum;
    //如果trackingId太大的话，把trackingId归0
    if(trackingId >= 0xfff) {
        trackingId = 0;
    }
    return;
}
