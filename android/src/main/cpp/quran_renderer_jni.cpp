/**
 * QuranRenderer JNI wrapper for Android
 */

#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include "quran/renderer.h"

#include <string>
#include <vector>

#define LOG_TAG "QuranRendererJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static QuranRendererHandle g_renderer = nullptr;
static std::vector<uint8_t> g_fontData;

extern "C" {

JNIEXPORT jboolean JNICALL
Java_org_digitalkhatt_quran_renderer_QuranRenderer_nativeInit(
    JNIEnv *env,
    jobject thiz,
    jobject assetManager,
    jstring fontPath
) {
    if (g_renderer) {
        LOGI("Renderer already initialized");
        return JNI_TRUE;
    }

    AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);
    if (!mgr) {
        LOGE("Failed to get asset manager");
        return JNI_FALSE;
    }

    const char *fontPathStr = env->GetStringUTFChars(fontPath, nullptr);
    AAsset *asset = AAssetManager_open(mgr, fontPathStr, AASSET_MODE_BUFFER);
    env->ReleaseStringUTFChars(fontPath, fontPathStr);

    if (!asset) {
        LOGE("Failed to open font asset");
        return JNI_FALSE;
    }

    off_t length = AAsset_getLength(asset);
    g_fontData.resize(length);
    AAsset_read(asset, g_fontData.data(), length);
    AAsset_close(asset);

    QuranFontData fontData;
    fontData.data = g_fontData.data();
    fontData.size = g_fontData.size();

    g_renderer = quran_renderer_create(&fontData);
    if (!g_renderer) {
        LOGE("Failed to create renderer");
        g_fontData.clear();
        return JNI_FALSE;
    }

    LOGI("Renderer initialized successfully");
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_org_digitalkhatt_quran_renderer_QuranRenderer_nativeDestroy(
    JNIEnv *env,
    jobject thiz
) {
    if (g_renderer) {
        quran_renderer_destroy(g_renderer);
        g_renderer = nullptr;
        g_fontData.clear();
        LOGI("Renderer destroyed");
    }
}

JNIEXPORT void JNICALL
Java_org_digitalkhatt_quran_renderer_QuranRenderer_nativeDrawPage(
    JNIEnv *env,
    jobject thiz,
    jobject bitmap,
    jint pageIndex,
    jboolean tajweed,
    jboolean justify
) {
    if (!g_renderer) {
        LOGE("Renderer not initialized");
        return;
    }

    AndroidBitmapInfo info;
    if (AndroidBitmap_getInfo(env, bitmap, &info) != ANDROID_BITMAP_RESULT_SUCCESS) {
        LOGE("Failed to get bitmap info");
        return;
    }

    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGE("Bitmap format must be RGBA_8888");
        return;
    }

    void *pixels;
    if (AndroidBitmap_lockPixels(env, bitmap, &pixels) != ANDROID_BITMAP_RESULT_SUCCESS) {
        LOGE("Failed to lock bitmap pixels");
        return;
    }

    QuranPixelBuffer buffer;
    buffer.pixels = pixels;
    buffer.width = info.width;
    buffer.height = info.height;
    buffer.stride = info.stride;
    buffer.format = QURAN_PIXEL_FORMAT_RGBA8888;

    QuranRenderConfig config;
    config.tajweed = tajweed;
    config.justify = justify;

    quran_renderer_draw_page(g_renderer, &buffer, pageIndex, &config);

    AndroidBitmap_unlockPixels(env, bitmap);
}

JNIEXPORT jint JNICALL
Java_org_digitalkhatt_quran_renderer_QuranRenderer_nativeGetPageCount(
    JNIEnv *env,
    jobject thiz
) {
    return g_renderer ? quran_renderer_get_page_count(g_renderer) : 0;
}

} // extern "C"
