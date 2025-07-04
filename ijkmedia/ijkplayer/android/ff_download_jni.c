//
// Created by guoshichao on 2025/7/4.
//

#include <string.h>
#include <jni.h>
#include "j4a/class/android/os/Bundle.h"
#include "ijksdl/ijksdl_log.h"
#include "ijksdl/android/ijksdl_android_jni.h"
#include "ijkplayer_android.h"
#include "ijkplayer/ff_download_video.h"

#define JNI_CLASS_FFMPEG_API "tv/danmaku/ijk/media/player/download/FFDownloadManager"

typedef struct ffdownload_fields_t {
    jclass clazz;
} ffdownload_fields_t;
static ffdownload_fields_t g_clazz;


typedef struct {
    JavaVM *jvm;                  // JVM 实例
    jobject progress_listener;    // Java 回调对象
    jmethodID onProgressMethod;   // 回调方法ID
} CallbackContext;

static void callback_context_init(JNIEnv *env, CallbackContext *ctx, jobject listener) {
    (*env)->GetJavaVM(env, &ctx->jvm);  // 获取 JVM
    ctx->progress_listener = (*env)->NewGlobalRef(env, listener);  // 创建全局引用

    // 获取回调方法ID
    jclass cls = (*env)->GetObjectClass(env, listener);
    ctx->onProgressMethod = (*env)->GetMethodID(env, cls, "onProgress", "(I)V");
    (*env)->DeleteLocalRef(env, cls);
}

static void callback_context_release(JNIEnv *env, CallbackContext *ctx) {
    if (ctx->progress_listener) {
        (*env)->DeleteGlobalRef(env, ctx->progress_listener);
    }
    memset(ctx, 0, sizeof(CallbackContext));
}

static void download_progress_callback(int progress, void *user_data) {
    MPTRACE("%s progress=%d\n", __func__, progress);

    CallbackContext *ctx = (CallbackContext *)user_data;

    JNIEnv *env;
    int attached = (*ctx->jvm)->GetEnv(ctx->jvm, (void **)&env, JNI_VERSION_1_6);

    // 如果当前线程未附加到JVM
    if (attached == JNI_EDETACHED) {
        (*ctx->jvm)->AttachCurrentThread(ctx->jvm, &env, NULL);
    }

    if (ctx->onProgressMethod) {
        (*env)->CallVoidMethod(env, ctx->progress_listener, ctx->onProgressMethod, progress);
    }

    // 如果是临时附加的线程，完成后解绑
    if (attached == JNI_EDETACHED) {
        (*ctx->jvm)->DetachCurrentThread(ctx->jvm);
    }

    // 销毁callback
    if (progress < 0 || progress >= 100) {
        callback_context_release(env, ctx);
    }
}

static jint
FFDownloadManager_startDownload(JNIEnv *env, jobject thiz, jstring url, jstring path, jstring cookie, jobject progress_listener)
{
    MPTRACE("%s\n", __func__);

    const char *c_url = NULL;
    c_url = (*env)->GetStringUTFChars(env, url, NULL );
    const char *c_path = NULL;
    c_path = (*env)->GetStringUTFChars(env, path, NULL );
    const char *c_cookie = NULL;
    c_cookie = (*env)->GetStringUTFChars(env, cookie, NULL );
    MPTRACE("url=%s, path=%s, cookie=%s", c_url, c_path, c_cookie);

    // 创建并初始化回调上下文
    CallbackContext ctx;
    callback_context_init(env, &ctx, progress_listener);

    int res = ff_start_download(c_url, c_path, c_cookie, download_progress_callback, &ctx);

    (*env)->ReleaseStringUTFChars(env, url, c_url);
    (*env)->ReleaseStringUTFChars(env, path, c_path);
    (*env)->ReleaseStringUTFChars(env, cookie, c_cookie);

    return res;
}

static jint
FFDownloadManager_stopDownload(JNIEnv *env, jobject thiz, jstring url)
{
    MPTRACE("%s\n", __func__);

    const char *c_url = NULL;
    c_url = (*env)->GetStringUTFChars(env, url, NULL );

    int res = ff_stop_download(c_url);

    (*env)->ReleaseStringUTFChars(env, url, c_url);

    return res;
}

static jint
FFDownloadManager_getDownloadProgress(JNIEnv *env, jobject thiz, jstring url)
{
    MPTRACE("%s\n", __func__);

    const char *c_url = NULL;
    c_url = (*env)->GetStringUTFChars(env, url, NULL );

    int res = ff_get_download_progress(c_url);

    (*env)->ReleaseStringUTFChars(env, url, c_url);

    return res;
}


static JNINativeMethod g_methods[] = {
        { "_startDownload",         "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ltv/danmaku/ijk/media/player/download/DownloadProgressListener;)I",(void *) FFDownloadManager_startDownload },
        { "_stopDownload",          "(Ljava/lang/String;)I",      (void *) FFDownloadManager_stopDownload },
        { "_getDownloadProgress",   "(Ljava/lang/String;)I",      (void *) FFDownloadManager_getDownloadProgress },
};


int FFDownloadManager_global_init(JNIEnv *env)
{
    int ret = 0;

    IJK_FIND_JAVA_CLASS(env, g_clazz.clazz, JNI_CLASS_FFMPEG_API);
    (*env)->RegisterNatives(env, g_clazz.clazz, g_methods, NELEM(g_methods));

    return ret;
}
