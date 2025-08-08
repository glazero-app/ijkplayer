#ifndef FFPLAYER_RECORD_THREAD_H
#define FFPLAYER_RECORD_THREAD_H

#include <pthread.h>
#include <libavformat/avformat.h>

// 向前声明FFPlayer结构体
typedef struct FFPlayer FFPlayer;

// 录制线程的参数结构
typedef struct {
    FFPlayer *ffp;
    AVPacket *packet;
    int running;           // 线程运行标志
    pthread_mutex_t mutex; // 互斥锁，用于同步数据包
    pthread_cond_t cond;   // 条件变量，用于唤醒线程处理新数据
} RecordThreadArgs;

// 初始化录制线程
RecordThreadArgs *ffp_record_thread_init(FFPlayer *ffp);

// 启动录制线程
int ffp_record_thread_start(FFPlayer *ffp);

// 向录制线程发送数据包进行处理
int ffp_record_thread_send_packet(FFPlayer *ffp, AVPacket *packet);

// 停止录制线程
void ffp_record_thread_stop(FFPlayer *ffp);


#endif // FFPLAYER_RECORD_THREAD_H
    
