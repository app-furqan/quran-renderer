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
static std::vector<uint8_t> g_surahHeaderFontData;

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

    // Load surah header font (optional)
    AAsset *surahHeaderAsset = AAssetManager_open(mgr, "fonts/QCF_SurahHeader_COLOR-Regular.ttf", AASSET_MODE_BUFFER);
    if (surahHeaderAsset) {
        off_t headerLength = AAsset_getLength(surahHeaderAsset);
        g_surahHeaderFontData.resize(headerLength);
        AAsset_read(surahHeaderAsset, g_surahHeaderFontData.data(), headerLength);
        AAsset_close(surahHeaderAsset);
        
        QuranFontData surahHeaderFontData;
        surahHeaderFontData.data = g_surahHeaderFontData.data();
        surahHeaderFontData.size = g_surahHeaderFontData.size();
        
        if (quran_renderer_load_surah_header_font(g_renderer, &surahHeaderFontData)) {
            LOGI("Surah header font loaded successfully");
        } else {
            LOGE("Failed to load surah header font");
            g_surahHeaderFontData.clear();
        }
    } else {
        LOGE("Surah header font not found in assets");
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
    jboolean justify,
    jfloat fontScale
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
    config.fontScale = fontScale;

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

// Surah/Ayah API - these don't require renderer initialization

JNIEXPORT jint JNICALL
Java_org_digitalkhatt_quran_renderer_QuranRenderer_nativeGetSurahCount(
    JNIEnv *env,
    jobject thiz
) {
    return quran_renderer_get_surah_count();
}

JNIEXPORT jint JNICALL
Java_org_digitalkhatt_quran_renderer_QuranRenderer_nativeGetTotalAyahCount(
    JNIEnv *env,
    jobject thiz
) {
    return quran_renderer_get_total_ayah_count();
}

JNIEXPORT jint JNICALL
Java_org_digitalkhatt_quran_renderer_QuranRenderer_nativeGetAyahCount(
    JNIEnv *env,
    jobject thiz,
    jint surahNumber
) {
    return quran_renderer_get_ayah_count(surahNumber);
}

JNIEXPORT jint JNICALL
Java_org_digitalkhatt_quran_renderer_QuranRenderer_nativeGetSurahStartPage(
    JNIEnv *env,
    jobject thiz,
    jint surahNumber
) {
    return quran_renderer_get_surah_start_page(surahNumber);
}

JNIEXPORT jint JNICALL
Java_org_digitalkhatt_quran_renderer_QuranRenderer_nativeGetAyahPage(
    JNIEnv *env,
    jobject thiz,
    jint surahNumber,
    jint ayahNumber
) {
    return quran_renderer_get_ayah_page(surahNumber, ayahNumber);
}

JNIEXPORT jobject JNICALL
Java_org_digitalkhatt_quran_renderer_QuranRenderer_nativeGetPageLocation(
    JNIEnv *env,
    jobject thiz,
    jint pageIndex
) {
    QuranAyahLocation loc;
    if (!quran_renderer_get_page_location(pageIndex, &loc)) {
        return nullptr;
    }
    
    jclass cls = env->FindClass("org/digitalkhatt/quran/renderer/AyahLocation");
    if (!cls) return nullptr;
    
    jmethodID constructor = env->GetMethodID(cls, "<init>", "(III)V");
    if (!constructor) return nullptr;
    
    return env->NewObject(cls, constructor, loc.surahNumber, loc.ayahNumber, loc.pageIndex);
}

JNIEXPORT jobject JNICALL
Java_org_digitalkhatt_quran_renderer_QuranRenderer_nativeGetSurahInfo(
    JNIEnv *env,
    jobject thiz,
    jint surahNumber
) {
    QuranSurahInfo info;
    if (!quran_renderer_get_surah_info(surahNumber, &info)) {
        return nullptr;
    }
    
    jclass cls = env->FindClass("org/digitalkhatt/quran/renderer/SurahInfo");
    if (!cls) return nullptr;
    
    jmethodID constructor = env->GetMethodID(cls, "<init>", 
        "(IIILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;II)V");
    if (!constructor) return nullptr;
    
    jstring nameArabic = env->NewStringUTF(info.nameArabic);
    jstring nameTrans = env->NewStringUTF(info.nameTrans);
    jstring nameEnglish = env->NewStringUTF(info.nameEnglish);
    jstring type = env->NewStringUTF(info.type);
    
    return env->NewObject(cls, constructor,
        info.number, info.ayahCount, info.startAyah,
        nameArabic, nameTrans, nameEnglish, type,
        info.revelationOrder, info.rukuCount);
}

} // extern "C"
