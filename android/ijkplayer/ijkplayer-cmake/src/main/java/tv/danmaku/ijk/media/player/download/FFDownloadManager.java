package tv.danmaku.ijk.media.player.download;

import android.util.Log;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

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

    // 管理下载状态
    private static final Map<String, Boolean> downloadStatusMap = new ConcurrentHashMap<>();

    /**
     * 开始下载指定 URL 的资源
     *
     * @param url 资源 URL
     * @param filePath 保存文件的路径
     * @param cookies HTTP 请求的 cookies（可为 null）
     * @throws IllegalArgumentException 如果 url 或 filePath 为 null
     * @throws IllegalStateException 如果下载启动失败
     */
    public static synchronized int startDownload(String url, String filePath, String cookies) {
        if (url == null || filePath == null) {
            Log.e(TAG, "URL and filePath cannot be null");
            return -1;
        }

        if (downloadStatusMap.containsKey(url) && downloadStatusMap.get(url)) {
            Log.w(TAG, "URL " + url + " is already being downloaded");
            return -1;
        }

        try {
            int res = _startDownload(url, filePath, cookies);
            downloadStatusMap.put(url, true);
            Log.i(TAG, "startDownload: " + res);
            return res;
        } catch (Exception e) {
            downloadStatusMap.put(url, false);
            Log.e(TAG, "Failed to start download: " + e.getMessage(), e);
            return -2;
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
            return -1;
        }

        try {
            int res = _stopDownload(url);
            Log.i(TAG, "stopDownload: " + res);
            return res;
        } catch (Exception e) {
            Log.e(TAG, "Failed to stop download: " + e.getMessage(), e);
            return -2;
        } finally {
            downloadStatusMap.put(url, false);
        }
    }

    /**
     * 获取指定 URL 的下载进度
     *
     * @param url 资源 URL
     * @return 下载进度百分比 (0-100)，-1 表示下载未开始或已完成
     * @throws IllegalArgumentException 如果 url 为 null
     * @throws IllegalStateException 如果获取进度失败
     */
    public static int getDownloadProgress(String url) {
        if (url == null) {
            Log.e(TAG, "URL and filePath cannot be null");
            return -1;
        }

        try {
            int res = _getDownloadProgress(url);
            if (res < 0 || res >= 100) {
                downloadStatusMap.put(url, false);
            }
            Log.i(TAG, "getDownloadProgress: " + res);
            return res;
        } catch (Exception e) {
            Log.e(TAG, "Failed to get download progress: " + e.getMessage(), e);
            return -1;
        }
    }

    private static native int _startDownload(String url, String filePath, String cookies);
    private static native int _stopDownload(String url);
    private static native int _getDownloadProgress(String url);

}
