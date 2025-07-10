#include "ff_download_video.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libavutil/avutil.h>

// 全局下载列表
static FFDownloadList* g_downloadList = NULL;

// 内部函数声明
static FFDownloadList* ff_create_download_list(int initialCapacity);
static void ff_free_download_list(FFDownloadList* list);
static int ff_find_download_item_index(const char* url);
static void ff_update_download_progress(const char* url, int progress);
static int ff_download(const FFDownloadItem* item);

// 初始化下载系统
void ff_init_download_system() {
    if (g_downloadList == NULL) {
        g_downloadList = ff_create_download_list(4);
    }
}

// 清理下载系统
void ff_cleanup_download_system() {
    if (g_downloadList) {
        // 先停止所有下载
        for (int i = 0; i < g_downloadList->size; i++) {
            g_downloadList->items[i].is_cancel_download = true;
            if (g_downloadList->items[i].input_format_context) {
                avformat_close_input(&g_downloadList->items[i].input_format_context);
            }
        }
        
        ff_free_download_list(g_downloadList);
        g_downloadList = NULL;
    }
}

// 创建下载列表
static FFDownloadList* ff_create_download_list(int initialCapacity) {
    FFDownloadList* list = (FFDownloadList*)malloc(sizeof(FFDownloadList));
    list->items = (FFDownloadItem*)malloc(initialCapacity * sizeof(FFDownloadItem));
    list->size = 0;
    list->capacity = initialCapacity;
    return list;
}

// 释放下载列表
static void ff_free_download_list(FFDownloadList* list) {
    for (int i = 0; i < list->size; i++) {
        if (list->items[i].input_format_context) {
            avformat_close_input(&list->items[i].input_format_context);
        }
    }
    free(list->items);
    free(list);
}

// 根据URL查找下载项索引
static int ff_find_download_item_index(const char* url) {
    int index = -1;
    if (g_downloadList) {
        for (int i = 0; i < g_downloadList->size; i++) {
            if (strcmp(g_downloadList->items[i].url, url) == 0) {
                index = i;
                break;
            }
        }
    }
    return index;
}

// 更新下载进度
static void ff_update_download_progress(const char* url, int progress) {
    if (g_downloadList) {
        int index = ff_find_download_item_index(url);
        if (index != -1) {
            g_downloadList->items[index].download_progress = progress;
            if (g_downloadList->items[index].progress_callback) {
                g_downloadList->items[index].progress_callback(progress, g_downloadList->items[index].user_data);
            }
        }
    }
}

// 下载函数
static int ff_download(const FFDownloadItem* item) {
    AVFormatContext *input_format_context = NULL;
    AVFormatContext *output_format_context = NULL;
    AVOutputFormat *output_format = NULL;
    AVStream *in_stream = NULL;
    AVStream *out_stream = NULL;
    int stream_index = 0;
    int *stream_mapping = NULL;
    int stream_mapping_size = 0;
    int ret;
    int oldProgress = 0;

    // 初始化 FFmpeg 库
    avformat_network_init();
    
    // 优化网络缓存和超时设置
    AVDictionary *headers = NULL;
    AVDictionary *options = NULL;
    av_dict_set(&options, "buffer_size", "4194304", 0); // 4MB 输入缓存
    av_dict_set(&options, "max_delay", "5000000", 0);  // 5秒最大延迟
    av_dict_set(&options, "stimeout", "10000000", 0);  // 10秒超时
    av_dict_set(&options, "rw_timeout", "10000000", 0); // 10秒读写超时
    
    if (item->cookie) {
        av_dict_set(&options, "headers", item->cookie, 0);
    }
    
    // 打开输入网络流
    ret = avformat_open_input(&input_format_context, item->url, NULL, &options);
    if (ret < 0) {
        fprintf(stderr, "无法打开输入流: %s\n", av_err2str(ret));
        ff_update_download_progress(item->url, -1); // 错误状态
        goto cleanup;
    }
    
    // 保存输入上下文供停止函数使用
    ((FFDownloadItem*)item)->input_format_context = input_format_context;

    // 查找流信息
    ret = avformat_find_stream_info(input_format_context, NULL);
    if (ret < 0) {
        fprintf(stderr, "无法查找流信息: %s\n", av_err2str(ret));
        ff_update_download_progress(item->url, -2);
        goto cleanup;
    }
    
    // 估算总大小
    if (input_format_context->duration != AV_NOPTS_VALUE) {
        ((FFDownloadItem*)item)->total_size = input_format_context->duration / 100;
    } else {
        ((FFDownloadItem*)item)->total_size = 1000000000; // 默认值
    }
    
    // 分配输出格式上下文
    avformat_alloc_output_context2(&output_format_context, NULL, NULL, item->output_file_url);
    if (!output_format_context) {
        fprintf(stderr, "无法创建输出上下文\n");
        ff_update_download_progress(item->url, -3);
        goto cleanup;
    }
    output_format = output_format_context->oformat;

    stream_mapping_size = input_format_context->nb_streams;
    stream_mapping = av_mallocz_array(stream_mapping_size, sizeof(*stream_mapping));
    if (!stream_mapping) {
        fprintf(stderr, "无法分配流映射数组\n");
        ff_update_download_progress(item->url, -4);
        goto cleanup;
    }

    // 初始化流映射
    for (unsigned int i = 0; i < input_format_context->nb_streams; i++) {
        in_stream = input_format_context->streams[i];
        AVCodecParameters *in_codecpar = in_stream->codecpar;

        if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            stream_mapping[i] = -1;
            continue;
        }

        stream_mapping[i] = stream_index++;

        // 创建输出流
        out_stream = avformat_new_stream(output_format_context, NULL);
        if (!out_stream) {
            fprintf(stderr, "无法分配输出流\n");
            ff_update_download_progress(item->url, -5);
            goto cleanup;
        }

        // 复制输入流的编解码器参数到输出流
        if (avcodec_parameters_copy(out_stream->codecpar, in_codecpar) < 0) {
            fprintf(stderr, "无法复制编解码器参数\n");
            ff_update_download_progress(item->url, -6);
            goto cleanup;
        }
        out_stream->codecpar->codec_tag = 0;

        // 复制元数据
        av_dict_copy(&out_stream->metadata, in_stream->metadata, 0);

        // 确保帧率和时间基一致
        out_stream->time_base = in_stream->time_base;
        out_stream->avg_frame_rate = in_stream->avg_frame_rate;
        
        // 优化视频编码参数
        if (in_codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            // 设置适当的缓冲大小
//            out_stream->codecpar->buf_size = 2000000; // 2MB缓冲区
            
            // 如果是实时流，设置低延迟选项
            if (av_dict_get(input_format_context->metadata, "live", NULL, 0)) {
                av_dict_set(&options, "tune", "zerolatency", 0);
                av_dict_set(&options, "preset", "ultrafast", 0);
            }
        }
    }

    // 打开输出文件
    if (!(output_format->flags & AVFMT_NOFILE)) {
        if (avio_open(&output_format_context->pb, item->output_file_url, AVIO_FLAG_WRITE) < 0) {
            fprintf(stderr, "无法打开输出文件: %s\n", av_err2str(ret));
            ff_update_download_progress(item->url, -7);
            goto cleanup;
        }
    }

    // 写入文件头
    if (avformat_write_header(output_format_context, &options) < 0) {
        fprintf(stderr, "打开输出文件时出错\n");
        ff_update_download_progress(item->url, -8);
        goto cleanup;
    }

    AVPacket packet;
    int64_t first_pts = AV_NOPTS_VALUE;
    int64_t last_pts = AV_NOPTS_VALUE;
    int64_t last_dts = AV_NOPTS_VALUE;
    int consecutive_errors = 0;
    
    // 主下载循环
    while (1) {
        if (item->is_cancel_download) {
            fprintf(stderr, "下载已取消: %s\n", item->url);
            ff_update_download_progress(item->url, -9); // 取消状态
            break;
        }
        
        // 重置packet
        av_init_packet(&packet);
        packet.data = NULL;
        packet.size = 0;
        
        ret = av_read_frame(input_format_context, &packet);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                break;
            } else if (ret == AVERROR(EAGAIN)) {
                // 资源暂时不可用，重试
//                av_usleep(100000); // 100ms
                consecutive_errors++;
                if (consecutive_errors > 50) { // 连续多次失败则退出
                    fprintf(stderr, "连续读取失败: %s\n", av_err2str(ret));
                    ff_update_download_progress(item->url, -10);
                    break;
                }
                continue;
            } else {
                fprintf(stderr, "读取帧时出错: %s\n", av_err2str(ret));
                ff_update_download_progress(item->url, -10);
                
                // 尝试恢复
                consecutive_errors++;
                if (consecutive_errors > 10) {
                    break;
                } else {
                    av_packet_unref(&packet);
                    continue;
                }
            }
        }
        
        consecutive_errors = 0;

        in_stream = input_format_context->streams[packet.stream_index];
        if (packet.stream_index >= stream_mapping_size ||
            stream_mapping[packet.stream_index] < 0) {
            av_packet_unref(&packet);
            continue;
        }
        
        // 恢复原来的进度计算方式
        ((FFDownloadItem*)item)->downloaded_size += packet.duration / 16.0;
        if (item->total_size > 0) {
            int progress = ((double)item->downloaded_size / item->total_size) * 100;
            if (progress > 99) progress = 99;
            if (progress > oldProgress) {
                ff_update_download_progress(item->url, progress);
                oldProgress = progress;
            }
        }

        packet.stream_index = stream_mapping[packet.stream_index];
        out_stream = output_format_context->streams[packet.stream_index];

        // 处理起始偏移
        if (first_pts == AV_NOPTS_VALUE) {
            first_pts = packet.pts;
        }
        
        // 时间戳校正，防止时间戳回退
        if (last_pts != AV_NOPTS_VALUE && packet.pts < last_pts) {
            packet.pts = last_pts + av_rescale_q(40, (AVRational){1, 1000}, in_stream->time_base);
        }
        if (last_dts != AV_NOPTS_VALUE && packet.dts < last_dts) {
            packet.dts = last_dts + av_rescale_q(40, (AVRational){1, 1000}, in_stream->time_base);
        }
        
        last_pts = packet.pts;
        last_dts = packet.dts;

        packet.pts -= first_pts;
        packet.dts -= first_pts;

        // 转换时间戳
        av_packet_rescale_ts(&packet, in_stream->time_base, out_stream->time_base);
        packet.pos = -1;

        // 写入数据包
        ret = av_interleaved_write_frame(output_format_context, &packet);
        if (ret < 0) {
            fprintf(stderr, "混合数据包时出错: %s\n", av_err2str(ret));
            ff_update_download_progress(item->url, -11);
            av_packet_unref(&packet);
            consecutive_errors++;
            if (consecutive_errors > 10) {
                break;
            }
            continue;
        }
        consecutive_errors = 0;
        av_packet_unref(&packet);
    }

    // 写入文件尾
    if (av_write_trailer(output_format_context) < 0) {
        fprintf(stderr, "写入文件尾时出错\n");
    } else {
        ff_update_download_progress(item->url, 100); // 完成状态
        ret = 0;
    }

cleanup:
    // 释放资源
    if (input_format_context) {
        avformat_close_input(&input_format_context);
        ((FFDownloadItem*)item)->input_format_context = NULL;
    }
    if (output_format_context && !(output_format->flags & AVFMT_NOFILE)) {
        avio_closep(&output_format_context->pb);
    }
    if (output_format_context) {
        avformat_free_context(output_format_context);
    }
    if (stream_mapping) {
        av_freep(&stream_mapping);
    }
    if (headers) {
        av_dict_free(&headers);
    }
    if (options) {
        av_dict_free(&options);
    }
    
    // 从列表中移除已完成的下载项
    for (int i = 0; i < item->list->size; i++) {
        if (strcmp(item->list->items[i].url, item->url) == 0) {
            // 将后面的元素前移
            for (int j = i; j < item->list->size - 1; j++) {
                item->list->items[j] = item->list->items[j + 1];
            }
            item->list->size--;
            break;
        }
    }
    
    return ret;
}

// 启动下载
int ff_start_download(const char* url, const char* output_file, const char* cookie, void (*progress_callback)(int, void*), void *user_data){
    if (!g_downloadList) {
        ff_init_download_system();
    }
    
    if (!url || !output_file) return -1;
    
    // 检查是否已存在相同URL的下载
    int index = ff_find_download_item_index(url);
    if (index != -1) {
        return -2; // 已存在下载
    }
    
    // 扩容检查
    if (g_downloadList->size >= g_downloadList->capacity) {
        g_downloadList->capacity *= 2;
        g_downloadList->items = (FFDownloadItem*)realloc(g_downloadList->items, g_downloadList->capacity * sizeof(FFDownloadItem));
    }

    // 初始化新下载项
    FFDownloadItem* item = &g_downloadList->items[g_downloadList->size];
    item->url = url;
    item->output_file_url = output_file;
    item->cookie = cookie;
    item->download_progress = 0;
    item->is_cancel_download = false;
    item->total_size = 0;
    item->downloaded_size = 0;
    item->progress_callback = progress_callback;
    item->user_data = user_data;
    item->input_format_context = NULL;
    item->list = g_downloadList; // 保存列表指针
    g_downloadList->size++;

    // 开始下载
    ff_download(item);
    int ret = 0;
    return ret;
}


// 停止下载
int ff_stop_download(const char* url) {
    if (!g_downloadList || !url) return -1;
    
    int index = ff_find_download_item_index(url);
    if (index == -12) {
        return -13; // 未找到下载项
    }
    
    // 标记为取消
    g_downloadList->items[index].is_cancel_download = true;
    
    // 如果有输入上下文，尝试中断读取
    if (g_downloadList->items[index].input_format_context) {
        avformat_close_input(&g_downloadList->items[index].input_format_context);
        g_downloadList->items[index].input_format_context = NULL;
    }
    
    return 0;
}

// 获取下载进度
int ff_get_download_progress(const char* url) {
    if (!g_downloadList || !url) return -14;
    
    int progress = -15;
    int index = ff_find_download_item_index(url);
    if (index != -1) {
        progress = g_downloadList->items[index].download_progress;
    }
    
    return progress;
}
