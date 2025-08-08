#include "ffplayer_record_thread.h"
#include "ff_ffplay.h"
#include <libavutil/error.h>
#include <string.h>

static void *record_thread(void *arg);
// 初始化录制线程
RecordThreadArgs *ffp_record_thread_init(FFPlayer *ffp) {
    av_log(ffp, AV_LOG_INFO, "Record thread: record thread init\n");
    RecordThreadArgs *args = av_mallocz(sizeof(RecordThreadArgs));
    if (!args) {
        av_log(ffp, AV_LOG_ERROR, "Record thread: Failed to allocate memory for record thread args\n");
        return NULL;
    }

    // 初始化互斥锁和条件变量
    if (pthread_mutex_init(&args->mutex, NULL) != 0 ||
        pthread_cond_init(&args->cond, NULL) != 0) {
        av_log(ffp, AV_LOG_ERROR, "Record thread: Failed to initialize mutex or condition variable\n");
        av_free(args);
        return NULL;
    }
    
    return args;
}

// 启动录制线程
int ffp_record_thread_start(FFPlayer *ffp) {
    av_log(ffp, AV_LOG_INFO, "Record thread: record thread start\n");
    if (!ffp->record_thread_args) return AVERROR(EINVAL);

    ffp->record_thread_args->ffp = ffp;
    ffp->record_thread_args->packet = NULL;
    ffp->record_thread_args->running = 1;
    ffp->record_thread_id = 0;

    int ret = pthread_create(&ffp->record_thread_id, NULL, record_thread, ffp->record_thread_args);
    if (ret != 0) {
        av_log(ffp, AV_LOG_ERROR, "Record thread: Failed to create record thread: %s\n", strerror(ret));
        return AVERROR(ret);
    }
    return 0;
}

// 向录制线程发送数据包进行处理
int ffp_record_thread_send_packet(FFPlayer *ffp, AVPacket *packet) {
    RecordThreadArgs *args = ffp->record_thread_args;
    if (!args || !packet || !args->running) {
        return AVERROR(EINVAL);
    }
    
    pthread_mutex_lock(&args->mutex);
    
    // 释放之前的数据包
    if (args->packet) {
        av_packet_unref(args->packet);
        av_packet_free(&args->packet);
    }
    
    // 引用新的数据包
    args->packet = av_packet_alloc();
    if (!args->packet) {
        pthread_mutex_unlock(&args->mutex);
        return AVERROR(ENOMEM);
    }
    
    if (av_packet_ref(args->packet, packet) < 0) {
        av_packet_free(&args->packet);
        args->packet = NULL;
        pthread_mutex_unlock(&args->mutex);
        return AVERROR(EINVAL);
    }
    
    // 唤醒线程处理数据包
    pthread_cond_signal(&args->cond);
    pthread_mutex_unlock(&args->mutex);
    
    return 0;
}

// 停止录制线程
void ffp_record_thread_stop(FFPlayer *ffp) {
    RecordThreadArgs *args = ffp->record_thread_args;
    if (!args) return;

    // 1. 发送停止信号
    pthread_mutex_lock(&args->mutex);
    if (args->running) { // 避免重复停止
        args->running = 0;
        pthread_cond_signal(&args->cond); // 唤醒线程使其退出
    }
    pthread_mutex_unlock(&args->mutex);

    // 2. 【关键步骤】等待线程自然结束
    // pthread_join 会阻塞当前线程（主线程），直到 thread_id 代表的线程完全退出。
    // 这确保了 record_thread 已经执行完它所有的代码，不再需要 args 中的任何资源。
    ALOGD("ffp_record, ffp->record_thread_id: %ld", ffp->record_thread_id);
    if (ffp->record_thread_id != 0) { // 检查线程ID是否有效
        void *thread_return_value;
        int ret = pthread_join(ffp->record_thread_id, &thread_return_value);
        if (ret != 0) {
            // 打印日志，join 失败可能是个问题
            ALOGE("ffp_record, Failed to join record thread: %s", strerror(ret));
        }
    }

    // 3. 在确认线程已经结束后，才安全地销毁资源
    pthread_mutex_destroy(&args->mutex);
    pthread_cond_destroy(&args->cond);

    // 4. 释放剩余的资源
    if (args->packet) {
        av_packet_unref(args->packet);
        // 注意：这里 packet 是一个指针，应该释放指针本身
        av_packet_free(&args->packet);
    }
    av_free(args);
}

// 原有的录制函数保持不变
int ffp_record_file(FFPlayer *ffp, AVPacket *packet) {
    assert(ffp);
    VideoState *is = ffp->is;
    int ret = 0;
    AVStream *in_stream = NULL;
    AVStream *out_stream = NULL;
    
    if (ffp->is_record) {
        if (packet == NULL) {
            ffp->record_error = 1;
            av_log(ffp, AV_LOG_ERROR, "ffp_record, packet == NULL");
            return -1;
        }
        
        // 检查输出上下文是否正确初始化
        if (!ffp->m_ofmt_ctx || !ffp->m_ofmt_ctx->pb) {
            av_log(ffp, AV_LOG_ERROR, "ffp_record, Output context not initialized");
            return AVERROR(EINVAL);
        }
        
        AVPacket *pkt = av_packet_alloc();
        if (!pkt) {
            av_log(ffp, AV_LOG_ERROR, "ffp_record, Failed to allocate packet");
            return AVERROR(ENOMEM);
        }
        
        if (av_packet_ref(pkt, packet) < 0) {
            av_log(ffp, AV_LOG_ERROR, "ffp_record, av_packet_ref failed");
            av_packet_free(&pkt);
            return -1;
        }
                
        // 验证流索引是否有效
        if (pkt->stream_index < 0 || pkt->stream_index >= is->ic->nb_streams) {
            av_log(ffp, AV_LOG_ERROR, "ffp_record, Invalid stream index: %d", pkt->stream_index);
            ret = AVERROR(EINVAL);
            goto exit;
        }
        
        in_stream  = is->ic->streams[pkt->stream_index];
        
        // 确保输出流存在且索引匹配
        if (pkt->stream_index >= ffp->m_ofmt_ctx->nb_streams) {
            av_log(ffp, AV_LOG_ERROR, "ffp_record, Output stream index out of range: %d", pkt->stream_index);
            ret = AVERROR(EINVAL);
            goto exit;
        }
        
        out_stream = ffp->m_ofmt_ctx->streams[pkt->stream_index];
        
        // 验证输入和输出流类型匹配
        if (in_stream->codecpar->codec_type != out_stream->codecpar->codec_type) {
            av_log(ffp, AV_LOG_ERROR, "ffp_record, Stream type mismatch: input=%d, output=%d",
                  in_stream->codecpar->codec_type, out_stream->codecpar->codec_type);
            ret = AVERROR(EINVAL);
            goto exit;
        }
        
        // 确保输出流time_base已正确初始化
        if (out_stream->time_base.num <= 0 || out_stream->time_base.den <= 0) {
            // 如果time_base无效，使用输入流的time_base
            out_stream->time_base = in_stream->time_base;
            av_log(ffp, AV_LOG_WARNING, "ffp_record, Output stream time_base was invalid, set to %d/%d",
                  out_stream->time_base.num, out_stream->time_base.den);
        }
        
        // 处理第一帧的基准时间
        if (!ffp->is_first) {
            ffp->is_first = 1;
            
            // 初始化基准时间（如果原始值无效，则使用0）
            ffp->start_pts = (pkt->pts != AV_NOPTS_VALUE) ? pkt->pts : 0;
            ffp->start_dts = (pkt->dts != AV_NOPTS_VALUE) ? pkt->dts : 0;
            
            // 对第一帧应用相对时间戳
            pkt->pts = 0;
            pkt->dts = 0;
            
            av_log(ffp, AV_LOG_INFO, "ffp_record, Recording started: stream_index=%d, start_pts=%lld, start_dts=%lld",
                  pkt->stream_index, ffp->start_pts, ffp->start_dts);
        } else {
            // 计算相对时间戳（处理AV_NOPTS_VALUE）
            if (pkt->pts != AV_NOPTS_VALUE) {
                pkt->pts = pkt->pts - ffp->start_pts;
            } else {
                // 生成连续的时间戳
                static int64_t generated_pts[AVMEDIA_TYPE_NB] = {0}; // 按媒体类型生成
                pkt->pts = generated_pts[pkt->stream_index]++;
                av_log(ffp, AV_LOG_WARNING, "ffp_record, Generated PTS for packet without timestamp: %lld", pkt->pts);
            }
            
            if (pkt->dts != AV_NOPTS_VALUE) {
                pkt->dts = pkt->dts - ffp->start_dts;
            } else {
                // 没有有效dts时，使用pts或生成值
                pkt->dts = (pkt->pts != AV_NOPTS_VALUE) ? pkt->pts : 0;
            }
            
            av_log(ffp, AV_LOG_DEBUG, "ffp_record, Before rescale: stream=%d, pts=%lld, dts=%lld",
                  pkt->stream_index, pkt->pts, pkt->dts);
        }
        
        // 时间基转换
        if (pkt->pts != AV_NOPTS_VALUE)
            pkt->pts = av_rescale_q_rnd(pkt->pts, in_stream->time_base, out_stream->time_base,
                                      (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                                      
        if (pkt->dts != AV_NOPTS_VALUE)
            pkt->dts = av_rescale_q_rnd(pkt->dts, in_stream->time_base, out_stream->time_base,
                                      (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                                      
        if (pkt->duration > 0)
            pkt->duration = av_rescale_q(pkt->duration, in_stream->time_base, out_stream->time_base);
            
        pkt->pos = -1;
        
        // 确保DTS <= PTS
        if (pkt->pts != AV_NOPTS_VALUE && pkt->dts != AV_NOPTS_VALUE && pkt->dts > pkt->pts) {
            av_log(ffp, AV_LOG_WARNING, "ffp_record, DTS > PTS corrected: dts=%lld, pts=%lld", pkt->dts, pkt->pts);
            pkt->dts = pkt->pts;
        }
        
        // 确保时间戳非负（某些格式不允许负数）
        if (pkt->pts != AV_NOPTS_VALUE && pkt->pts < 0) {
            av_log(ffp, AV_LOG_WARNING, "ffp_record, Negative PTS corrected to 0: %lld", pkt->pts);
            pkt->pts = 0;
        }
        
        if (pkt->dts != AV_NOPTS_VALUE && pkt->dts < 0) {
            av_log(ffp, AV_LOG_WARNING, "ffp_record, Negative DTS corrected to 0: %lld", pkt->dts);
            pkt->dts = 0;
        }
        
        // 打印转换后的时间戳，用于调试
        av_log(ffp, AV_LOG_DEBUG, "ffp_record, After rescale: stream=%d, pts=%lld, dts=%lld, duration=%lld",
              pkt->stream_index, pkt->pts, pkt->dts, pkt->duration);
        
        // 写入数据包
        if ((ret = av_interleaved_write_frame(ffp->m_ofmt_ctx, pkt)) < 0) {
            if (s_record_fail_callback) {
                s_record_fail_callback(ffp->inject_opaque, ret);
            }
            av_log(ffp, AV_LOG_ERROR, "ffp_record, Error muxing packet: %s", av_err2str(ret));
            
            // 输出更多上下文信息用于调试
            av_log(ffp, AV_LOG_ERROR, "ffp_record, Packet info: stream=%d, pts=%lld, dts=%lld, size=%d, flags=0x%x",
                  pkt->stream_index, pkt->pts, pkt->dts, pkt->size, pkt->flags);
            
            av_log(ffp, AV_LOG_ERROR, "ffp_record, Input stream time_base: %d/%d",
                  in_stream->time_base.num, in_stream->time_base.den);
                  
            av_log(ffp, AV_LOG_ERROR, "ffp_record, Output stream time_base: %d/%d",
                  out_stream->time_base.num, out_stream->time_base.den);
            
            // 额外调试：检查输出上下文状态
            av_log(ffp, AV_LOG_ERROR, "ffp_record, Output format: %s, nb_streams: %d",
                  ffp->m_ofmt_ctx->oformat->name, ffp->m_ofmt_ctx->nb_streams);
        }
        
exit:
        av_packet_unref(pkt);
        av_packet_free(&pkt);
    }
    return ret;
}
    

// 录制线程的主函数
static void *record_thread(void *arg) {
    RecordThreadArgs *args = (RecordThreadArgs *)arg;
    FFPlayer *ffp = args->ffp;

    av_log(ffp, AV_LOG_INFO, "Record thread: started running=%d\n", args->running);

    while (args->running) {
        int ret = pthread_mutex_lock(&args->mutex);
        if (ret != 0) {
            // 如果加锁失败，打印错误并退出
            av_log(ffp, AV_LOG_ERROR, "Record thread: failed to lock mutex: %s\n", strerror(ret));
            break;
        }
        
        // 等待新的数据包或退出信号
        while (args->running && !args->packet) {
            // 打印日志，确认即将进入 wait
            pthread_cond_wait(&args->cond, &args->mutex);
        }
        
        // 检查是否需要退出
        if (!args->running) {
            pthread_mutex_unlock(&args->mutex);
            break;
        }
        
        // 处理数据包
        if (args->packet) {
            // 调用原有的录制函数
            ffp_record_file(ffp, args->packet);
            args->packet = NULL;
        }
        
        pthread_mutex_unlock(&args->mutex);
    }
    
    av_log(ffp, AV_LOG_INFO, "Record thread: exited\n");
    return NULL;
}

