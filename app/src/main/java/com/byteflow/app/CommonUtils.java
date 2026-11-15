package com.byteflow.app;

import android.content.Context;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

public class CommonUtils {
    private static final String TAG = "CommonUtils";

    public static void copyAssetsDirToSDCard(Context context, String assetsDirName, String sdCardPath) {
        Log.d(TAG, "copyAssetsDirToSDCard() called with: context = [" + context + "], assetsDirName = [" + assetsDirName + "], sdCardPath = [" + sdCardPath + "]");
        try {
            String list[] = context.getAssets().list(assetsDirName);
            if (list.length == 0) {
                File file = new File(sdCardPath + File.separator
                        + assetsDirName.substring(assetsDirName.lastIndexOf('/') + 1));
                if (file.exists()) {
                    return;
                }
                // 先写临时文件，完整写完再改名：原来直接写目标文件，拷贝中途进程被杀会留下
                // 截断的文件，之后因为“文件已存在”永远不会再修复，native 读到的是残缺数据。
                File tmp = new File(file.getPath() + ".tmp");
                byte[] mByte = new byte[64 * 1024];
                int bt;
                try (InputStream inputStream = context.getAssets().open(assetsDirName);
                     FileOutputStream fos = new FileOutputStream(tmp)) {
                    while ((bt = inputStream.read(mByte)) != -1) {
                        fos.write(mByte, 0, bt);
                    }
                    fos.flush();
                }
                if (!tmp.renameTo(file)) {
                    Log.e(TAG, "copyAssetsDirToSDCard rename fail: " + file);
                    tmp.delete();
                }
            } else {
                String subDirName = assetsDirName;
                if (assetsDirName.contains("/")) {
                    subDirName = assetsDirName.substring(assetsDirName.lastIndexOf('/') + 1);
                }
                sdCardPath = sdCardPath + File.separator + subDirName;
                File file = new File(sdCardPath);
                if (!file.exists())
                    file.mkdirs();
                for (String s : list) {
                    copyAssetsDirToSDCard(context, assetsDirName + File.separator + s, sdCardPath);
                }
            }
        } catch (
                Exception e) {
            e.printStackTrace();
        }
    }
}
