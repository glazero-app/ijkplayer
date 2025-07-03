//
//  ff_download_video.h
//  IJKMediaPlayer
//
//  Created by 张志超 on 2025/7/1.
//  Copyright © 2025 bilibili. All rights reserved.
//

#ifndef FF_DOWNLOAD_VIDEO_H
#define FF_DOWNLOAD_VIDEO_H

#include <stdbool.h>
#include <libavformat/avformat.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FFDownloadList FFDownloadList;

// 下载项结构体
typedef struct FFDownloadItem {
    const char* url;
    const char* output_file_url;
    const char* cookie;
    int download_progress;
    bool is_cancel_download;
    AVFormatContext *input_format_context;
    int64_t total_size;
    int64_t downloaded_size;
    FFDownloadList* list;
    void (*progress_callback)(int progress);
} FFDownloadItem;

// 下载列表结构体
typedef struct FFDownloadList {
    FFDownloadItem* items;
    int size;
    int capacity;
} FFDownloadList;


// 清理下载系统
void ff_cleanup_download_system();

/// 下载视频并转成mp4存储 return == 0下载成功，其它表示失败;  业务层使用时需要放在子线程执行，不然会阻塞UI线程
/// - Parameters:
///   - url: 下载链接
///   - output_file: 下载文件路径需要包含文件名 example: test.mp4
///   - cookie: header内的cookie信息  格式为 Cookie:AWSALB=UdEy1
///   - progress_callback： 下载进度回调 0-100
int ff_start_download(const char* url, const char* output_file, const char* cookie,void (progress_callback)(int progress));

// 停止下载
int ff_stop_download(const char* url);

/// 获取下载进度 0 ~ 100  当进度为-1是均表示下载异常
/// - Parameter url: 下载链接
int ff_get_download_progress(const char* url);

#ifdef __cplusplus
}
#endif

#endif /* ff_download_video_h */
