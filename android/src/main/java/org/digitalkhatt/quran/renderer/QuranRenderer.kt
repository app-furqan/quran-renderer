package org.digitalkhatt.quran.renderer

import android.content.res.AssetManager
import android.graphics.Bitmap

/**
 * QuranRenderer - High-quality Quran text rendering
 * 
 * Uses custom HarfBuzz (with justification support) and Skia
 * for professional-grade Arabic text layout and tajweed coloring.
 */
class QuranRenderer private constructor() {

    companion object {
        private var instance: QuranRenderer? = null
        
        init {
            System.loadLibrary("quranrenderer")
        }

        /**
         * Get the singleton renderer instance.
         * Call initialize() before using.
         */
        @JvmStatic
        fun getInstance(): QuranRenderer {
            return instance ?: synchronized(this) {
                instance ?: QuranRenderer().also { instance = it }
            }
        }
    }

    private var initialized = false

    /**
     * Initialize the renderer with font asset.
     * 
     * @param assetManager Android AssetManager
     * @param fontPath Path to the font file in assets (e.g., "fonts/quran.otf")
     * @return true if initialization succeeded
     */
    fun initialize(assetManager: AssetManager, fontPath: String): Boolean {
        if (initialized) return true
        initialized = nativeInit(assetManager, fontPath)
        return initialized
    }

    /**
     * Check if the renderer is initialized.
     */
    fun isInitialized(): Boolean = initialized

    /**
     * Render a Quran page to a bitmap.
     * 
     * @param bitmap Bitmap to render into (must be ARGB_8888)
     * @param pageIndex Page index (0-603)
     * @param tajweed Enable tajweed coloring
     * @param justify Enable line justification
     */
    fun drawPage(
        bitmap: Bitmap,
        pageIndex: Int,
        tajweed: Boolean = true,
        justify: Boolean = true
    ) {
        require(initialized) { "QuranRenderer not initialized" }
        require(bitmap.config == Bitmap.Config.ARGB_8888) { "Bitmap must be ARGB_8888" }
        require(pageIndex in 0 until pageCount) { "Invalid page index: $pageIndex" }
        
        nativeDrawPage(bitmap, pageIndex, tajweed, justify)
    }

    /**
     * Create a new bitmap and render a page into it.
     * 
     * @param width Bitmap width in pixels
     * @param height Bitmap height in pixels
     * @param pageIndex Page index (0-603)
     * @param tajweed Enable tajweed coloring
     * @param justify Enable line justification
     * @return Rendered bitmap
     */
    fun renderPage(
        width: Int,
        height: Int,
        pageIndex: Int,
        tajweed: Boolean = true,
        justify: Boolean = true
    ): Bitmap {
        val bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
        drawPage(bitmap, pageIndex, tajweed, justify)
        return bitmap
    }

    /**
     * Get total number of pages.
     */
    val pageCount: Int
        get() = if (initialized) nativeGetPageCount() else 0

    /**
     * Release native resources.
     */
    fun destroy() {
        if (initialized) {
            nativeDestroy()
            initialized = false
        }
    }

    // Native methods
    private external fun nativeInit(assetManager: AssetManager, fontPath: String): Boolean
    private external fun nativeDestroy()
    private external fun nativeDrawPage(bitmap: Bitmap, pageIndex: Int, tajweed: Boolean, justify: Boolean)
    private external fun nativeGetPageCount(): Int
}
