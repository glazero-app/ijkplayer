package tv.danmaku.ijk.media.player.download;

import android.os.SystemClock;
import android.util.Log;

import java.util.concurrent.atomic.AtomicLong;

import tv.danmaku.ijk.media.player.IjkLoaderLibrary;

/**
 * author : guoshichao
 * date : 2025/7/4 10:22
 * description :
 */
public final class FFDownloadManager {

    private static final String TAG = "FFDownloadManager";

    static {
        // 加载本地库
        IjkLoaderLibrary.loadLibrariesOnce(IjkLoaderLibrary.sLocalLibLoader);
    }

    /**
     * 开始下载指定 URL 的资源
     *
     * @param url 资源 URL
     * @param filePath 保存文件的路径
     * @param cookies HTTP 请求的 cookies（可为 null）
     * @throws IllegalArgumentException 如果 url 或 filePath 为 null
     * @throws IllegalStateException 如果下载启动失败
     */
    public static synchronized int startDownload(String url, String filePath, String cookies, DownloadProgressListener listener) {
        if (url == null || filePath == null) {
            Log.e(TAG, "URL and filePath cannot be null");
            return -101;
        }

        try {
            int res = _startDownload(url, filePath, cookies, listener);
            Log.i(TAG, "startDownload: " + res);
            return res;
        } catch (Exception e) {
            Log.e(TAG, "Failed to start download: " + e.getMessage(), e);
            return -102;
        }
    }

    /**
     * 停止下载指定 URL 的资源
     *
     * @param url 资源 URL
     * @throws IllegalArgumentException 如果 url 为 null
     * @throws IllegalStateException 如果停止下载失败
     */
    public static synchronized int stopDownload(String url) {
        if (url == null) {
            Log.e(TAG, "URL and filePath cannot be null");
            return -101;
        }

        try {
            int res = _stopDownload(url);
            Log.i(TAG, "stopDownload: " + res);
            return res;
        } catch (Exception e) {
            Log.e(TAG, "Failed to stop download: " + e.getMessage(), e);
            return -102;
        }
    }

    private static native int _startDownload(String url, String filePath, String cookies, DownloadProgressListener listener);
    private static native int _stopDownload(String url);

}
