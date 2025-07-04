package tv.danmaku.ijk.media.player;

/**
 * author : guoshichao
 * date : 2025/7/4 11:52
 * description :
 */
public class IjkLoaderLibrary {

    /**
     * Default library loader
     * Load them by yourself, if your libraries are not installed at default place.
     */
    public static final IjkLibLoader sLocalLibLoader = new IjkLibLoader() {
        @Override
        public void loadLibrary(String libName) throws UnsatisfiedLinkError, SecurityException {
            System.loadLibrary(libName);
        }
    };

    private static volatile boolean mIsLibLoaded = false;
    public static void loadLibrariesOnce(IjkLibLoader libLoader) {
        synchronized (IjkMediaPlayer.class) {
            if (!mIsLibLoaded) {
                if (libLoader == null)
                    libLoader = sLocalLibLoader;

                libLoader.loadLibrary("ijkplayer");
                mIsLibLoaded = true;
            }
        }
    }

}
