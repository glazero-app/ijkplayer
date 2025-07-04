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


static void progress_callback(int progress) {
    printf("ffDownload progress: %d%%\n", progress);
}

static jint
FFDownloadManager_startDownload(JNIEnv *env, jobject thiz, jstring url, jstring path, jstring cookie)
{
    MPTRACE("%s\n", __func__);

    const char *c_url = NULL;
    c_url = (*env)->GetStringUTFChars(env, url, NULL );
    const char *c_path = NULL;
    c_path = (*env)->GetStringUTFChars(env, path, NULL );
    const char *c_cookie = NULL;
    c_cookie = (*env)->GetStringUTFChars(env, cookie, NULL );
    MPTRACE("url=%s, path=%s, cookie=%s", c_url, c_path, c_cookie);

    return ff_start_download(c_url, c_path, c_cookie, progress_callback);
}

static jint
FFDownloadManager_stopDownload(JNIEnv *env, jobject thiz, jstring url)
{
    MPTRACE("%s\n", __func__);

    const char *c_url = NULL;
    c_url = (*env)->GetStringUTFChars(env, url, NULL );

    return ff_stop_download(c_url);
}

static jint
FFDownloadManager_getDownloadProgress(JNIEnv *env, jobject thiz, jstring url)
{
    MPTRACE("%s\n", __func__);

    const char *c_url = NULL;
    c_url = (*env)->GetStringUTFChars(env, url, NULL );

    return ff_get_download_progress(c_url);
}


static JNINativeMethod g_methods[] = {
        { "_startDownload",         "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I",(void *) FFDownloadManager_startDownload },
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
