package org.digitalkhatt.quran.renderer

import android.content.res.AssetManager
import android.graphics.Bitmap

/**
 * Available DigitalKhatt font styles.
 * 
 * Note: Only MADINA_QURANIC includes tajweed coloring (COLR/CPAL tables).
 * Other fonts render in black without tajweed highlighting.
 */
enum class QuranFont(val assetPath: String, val displayName: String, val hasTajweed: Boolean) {
    /** Madina Quranic style with tajweed coloring (default) */
    MADINA_QURANIC("fonts/digitalkhatt.otf", "Madina Quranic", true),
    /** New Madina style (no tajweed coloring) */
    MADINA("fonts/madina.otf", "New Madina", false),
    /** Old Madina Mushaf style (no tajweed coloring) */
    OLD_MADINA("fonts/oldmadina.otf", "Old Madina", false),
    /** IndoPak 13-line Mushaf style (no tajweed coloring) */
    INDOPAK("fonts/indopak.otf", "IndoPak", false)
}

/**
 * QuranRenderer - High-quality Quran text rendering
 * 
 * Uses custom HarfBuzz (with justification support) and Skia
 * for professional-grade Arabic text layout and tajweed coloring.
 * 
 * Supports four font styles:
 * - MADINA_QURANIC: Madina style with tajweed coloring (default, recommended)
 * - MADINA: New Madina style (no tajweed)
 * - OLD_MADINA: Old Madina Mushaf style (no tajweed)
 * - INDOPAK: IndoPak 13-line Mushaf style (no tajweed)
 * 
 * Example usage:
 * ```kotlin
 * val renderer = QuranRenderer.getInstance()
 * renderer.initialize(assets, QuranFont.MADINA_QURANIC)
 * 
 * // Switch to IndoPak style (no tajweed colors)
 * renderer.setFont(assets, QuranFont.INDOPAK)
 * ```
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
    private var currentFont: QuranFont? = null

    /**
     * Initialize the renderer with a font style.
     * 
     * @param assetManager Android AssetManager
     * @param font QuranFont style to use (default: MADINA_QURANIC with tajweed)
     * @return true if initialization succeeded
     */
    fun initialize(assetManager: AssetManager, font: QuranFont = QuranFont.MADINA_QURANIC): Boolean {
        if (initialized) return true
        initialized = nativeInit(assetManager, font.assetPath)
        if (initialized) {
            currentFont = font
        }
        return initialized
    }

    /**
     * Initialize the renderer with a custom font path.
     * 
     * @param assetManager Android AssetManager
     * @param fontPath Path to the font file in assets (e.g., "fonts/custom.otf")
     * @return true if initialization succeeded
     */
    fun initialize(assetManager: AssetManager, fontPath: String): Boolean {
        if (initialized) return true
        initialized = nativeInit(assetManager, fontPath)
        if (initialized) {
            currentFont = null // Custom font
        }
        return initialized
    }

    /**
     * Switch to a different font style.
     * Destroys current renderer and reinitializes with new font.
     * 
     * @param assetManager Android AssetManager
     * @param font QuranFont style to switch to
     * @return true if switch succeeded
     */
    fun setFont(assetManager: AssetManager, font: QuranFont): Boolean {
        if (currentFont == font) return true
        
        if (initialized) {
            nativeDestroy()
            initialized = false
        }
        
        initialized = nativeInit(assetManager, font.assetPath)
        if (initialized) {
            currentFont = font
        }
        return initialized
    }

    /**
     * Get the currently active font style.
     * @return Current QuranFont or null if using custom font
     */
    fun getCurrentFont(): QuranFont? = currentFont

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
            currentFont = null
        }
    }

    // Native methods
    private external fun nativeInit(assetManager: AssetManager, fontPath: String): Boolean
    private external fun nativeDestroy()
    private external fun nativeDrawPage(bitmap: Bitmap, pageIndex: Int, tajweed: Boolean, justify: Boolean)
    private external fun nativeGetPageCount(): Int
}
