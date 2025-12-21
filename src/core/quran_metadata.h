/**
 * Quran Metadata - Surah and Page information
 * Auto-generated from quranmetadata.json and pages.json
 */

#ifndef QURAN_METADATA_H
#define QURAN_METADATA_H

#include <cstdint>

struct SurahData {
    int number;
    int ayahCount;
    int startAyah;          // Cumulative starting ayah (0-based)
    const char* nameArabic;
    const char* nameTrans;
    const char* nameEnglish;
    const char* type;
    int revelationOrder;
    int rukuCount;
};

// Total counts
static constexpr int QURAN_SURAH_COUNT = 114;
static constexpr int QURAN_TOTAL_AYAHS = 6236;
static constexpr int QURAN_PAGE_COUNT = 604;

// Surah metadata (1-indexed, index 0 is unused)
static const SurahData SURAH_DATA[115] = {
    // Index 0 - unused placeholder
    {0, 0, 0, "", "", "", "", 0, 0},
    // 1. Al-Faatiha
    {1, 7, 0, "الفاتحة", "Al-Faatiha", "The Opening", "Meccan", 5, 1},
    // 2. Al-Baqara
    {2, 286, 7, "البقرة", "Al-Baqara", "The Cow", "Medinan", 87, 40},
    // 3. Aal-i-Imraan
    {3, 200, 293, "آل عمران", "Aal-i-Imraan", "The Family of Imraan", "Medinan", 89, 20},
    // 4. An-Nisaa
    {4, 176, 493, "النساء", "An-Nisaa", "The Women", "Medinan", 92, 24},
    // 5. Al-Maaida
    {5, 120, 669, "المائدة", "Al-Maaida", "The Table", "Medinan", 112, 16},
    // 6. Al-An'aam
    {6, 165, 789, "الأنعام", "Al-An'aam", "The Cattle", "Meccan", 55, 20},
    // 7. Al-A'raaf
    {7, 206, 954, "الأعراف", "Al-A'raaf", "The Heights", "Meccan", 39, 24},
    // 8. Al-Anfaal
    {8, 75, 1160, "الأنفال", "Al-Anfaal", "The Spoils of War", "Medinan", 88, 10},
    // 9. At-Tawba
    {9, 129, 1235, "التوبة", "At-Tawba", "The Repentance", "Medinan", 113, 16},
    // 10. Yunus
    {10, 109, 1364, "يونس", "Yunus", "Jonas", "Meccan", 51, 11},
    // 11. Hud
    {11, 123, 1473, "هود", "Hud", "Hud", "Meccan", 52, 10},
    // 12. Yusuf
    {12, 111, 1596, "يوسف", "Yusuf", "Joseph", "Meccan", 53, 12},
    // 13. Ar-Ra'd
    {13, 43, 1707, "الرعد", "Ar-Ra'd", "The Thunder", "Medinan", 96, 6},
    // 14. Ibrahim
    {14, 52, 1750, "ابراهيم", "Ibrahim", "Abraham", "Meccan", 72, 7},
    // 15. Al-Hijr
    {15, 99, 1802, "الحجر", "Al-Hijr", "The Rock", "Meccan", 54, 6},
    // 16. An-Nahl
    {16, 128, 1901, "النحل", "An-Nahl", "The Bee", "Meccan", 70, 16},
    // 17. Al-Israa
    {17, 111, 2029, "الإسراء", "Al-Israa", "The Night Journey", "Meccan", 50, 12},
    // 18. Al-Kahf
    {18, 110, 2140, "الكهف", "Al-Kahf", "The Cave", "Meccan", 69, 12},
    // 19. Maryam
    {19, 98, 2250, "مريم", "Maryam", "Mary", "Meccan", 44, 6},
    // 20. Taa-Haa
    {20, 135, 2348, "طه", "Taa-Haa", "Taa-Haa", "Meccan", 45, 8},
    // 21. Al-Anbiyaa
    {21, 112, 2483, "الأنبياء", "Al-Anbiyaa", "The Prophets", "Meccan", 73, 7},
    // 22. Al-Hajj
    {22, 78, 2595, "الحج", "Al-Hajj", "The Pilgrimage", "Medinan", 103, 10},
    // 23. Al-Muminoon
    {23, 118, 2673, "المؤمنون", "Al-Muminoon", "The Believers", "Meccan", 74, 6},
    // 24. An-Noor
    {24, 64, 2791, "النور", "An-Noor", "The Light", "Medinan", 102, 9},
    // 25. Al-Furqaan
    {25, 77, 2855, "الفرقان", "Al-Furqaan", "The Criterion", "Meccan", 42, 6},
    // 26. Ash-Shu'araa
    {26, 227, 2932, "الشعراء", "Ash-Shu'araa", "The Poets", "Meccan", 47, 11},
    // 27. An-Naml
    {27, 93, 3159, "النمل", "An-Naml", "The Ant", "Meccan", 48, 7},
    // 28. Al-Qasas
    {28, 88, 3252, "القصص", "Al-Qasas", "The Stories", "Meccan", 49, 8},
    // 29. Al-Ankaboot
    {29, 69, 3340, "العنكبوت", "Al-Ankaboot", "The Spider", "Meccan", 85, 7},
    // 30. Ar-Room
    {30, 60, 3409, "الروم", "Ar-Room", "The Romans", "Meccan", 84, 6},
    // 31. Luqman
    {31, 34, 3469, "لقمان", "Luqman", "Luqman", "Meccan", 57, 3},
    // 32. As-Sajda
    {32, 30, 3503, "السجدة", "As-Sajda", "The Prostration", "Meccan", 75, 3},
    // 33. Al-Ahzaab
    {33, 73, 3533, "الأحزاب", "Al-Ahzaab", "The Clans", "Medinan", 90, 9},
    // 34. Saba
    {34, 54, 3606, "سبإ", "Saba", "Sheba", "Meccan", 58, 6},
    // 35. Faatir
    {35, 45, 3660, "فاطر", "Faatir", "The Originator", "Meccan", 43, 5},
    // 36. Yaseen
    {36, 83, 3705, "يس", "Yaseen", "Yaseen", "Meccan", 41, 5},
    // 37. As-Saaffaat
    {37, 182, 3788, "الصافات", "As-Saaffaat", "Those drawn up in Ranks", "Meccan", 56, 5},
    // 38. Saad
    {38, 88, 3970, "ص", "Saad", "The letter Saad", "Meccan", 38, 5},
    // 39. Az-Zumar
    {39, 75, 4058, "الزمر", "Az-Zumar", "The Groups", "Meccan", 59, 8},
    // 40. Al-Ghaafir
    {40, 85, 4133, "غافر", "Al-Ghaafir", "The Forgiver", "Meccan", 60, 9},
    // 41. Fussilat
    {41, 54, 4218, "فصلت", "Fussilat", "Explained in detail", "Meccan", 61, 6},
    // 42. Ash-Shura
    {42, 53, 4272, "الشورى", "Ash-Shura", "Consultation", "Meccan", 62, 5},
    // 43. Az-Zukhruf
    {43, 89, 4325, "الزخرف", "Az-Zukhruf", "Ornaments of gold", "Meccan", 63, 7},
    // 44. Ad-Dukhaan
    {44, 59, 4414, "الدخان", "Ad-Dukhaan", "The Smoke", "Meccan", 64, 3},
    // 45. Al-Jaathiya
    {45, 37, 4473, "الجاثية", "Al-Jaathiya", "Crouching", "Meccan", 65, 4},
    // 46. Al-Ahqaf
    {46, 35, 4510, "الأحقاف", "Al-Ahqaf", "The Dunes", "Meccan", 66, 4},
    // 47. Muhammad
    {47, 38, 4545, "محمد", "Muhammad", "Muhammad", "Medinan", 95, 4},
    // 48. Al-Fath
    {48, 29, 4583, "الفتح", "Al-Fath", "The Victory", "Medinan", 111, 4},
    // 49. Al-Hujuraat
    {49, 18, 4612, "الحجرات", "Al-Hujuraat", "The Inner Apartments", "Medinan", 106, 2},
    // 50. Qaaf
    {50, 45, 4630, "ق", "Qaaf", "The letter Qaaf", "Meccan", 34, 3},
    // 51. Adh-Dhaariyat
    {51, 60, 4675, "الذاريات", "Adh-Dhaariyat", "The Winnowing Winds", "Meccan", 67, 3},
    // 52. At-Tur
    {52, 49, 4735, "الطور", "At-Tur", "The Mount", "Meccan", 76, 2},
    // 53. An-Najm
    {53, 62, 4784, "النجم", "An-Najm", "The Star", "Meccan", 23, 3},
    // 54. Al-Qamar
    {54, 55, 4846, "القمر", "Al-Qamar", "The Moon", "Meccan", 37, 3},
    // 55. Ar-Rahmaan
    {55, 78, 4901, "الرحمن", "Ar-Rahmaan", "The Beneficent", "Medinan", 97, 3},
    // 56. Al-Waaqia
    {56, 96, 4979, "الواقعة", "Al-Waaqia", "The Inevitable", "Meccan", 46, 3},
    // 57. Al-Hadid
    {57, 29, 5075, "الحديد", "Al-Hadid", "The Iron", "Medinan", 94, 4},
    // 58. Al-Mujaadila
    {58, 22, 5104, "المجادلة", "Al-Mujaadila", "The Pleading Woman", "Medinan", 105, 3},
    // 59. Al-Hashr
    {59, 24, 5126, "الحشر", "Al-Hashr", "The Exile", "Medinan", 101, 3},
    // 60. Al-Mumtahana
    {60, 13, 5150, "الممتحنة", "Al-Mumtahana", "She that is to be examined", "Medinan", 91, 2},
    // 61. As-Saff
    {61, 14, 5163, "الصف", "As-Saff", "The Ranks", "Medinan", 109, 2},
    // 62. Al-Jumu'a
    {62, 11, 5177, "الجمعة", "Al-Jumu'a", "Friday", "Medinan", 110, 2},
    // 63. Al-Munaafiqoon
    {63, 11, 5188, "المنافقون", "Al-Munaafiqoon", "The Hypocrites", "Medinan", 104, 2},
    // 64. At-Taghaabun
    {64, 18, 5199, "التغابن", "At-Taghaabun", "Mutual Disillusion", "Medinan", 108, 2},
    // 65. At-Talaaq
    {65, 12, 5217, "الطلاق", "At-Talaaq", "Divorce", "Medinan", 99, 2},
    // 66. At-Tahrim
    {66, 12, 5229, "التحريم", "At-Tahrim", "The Prohibition", "Medinan", 107, 2},
    // 67. Al-Mulk
    {67, 30, 5241, "الملك", "Al-Mulk", "The Sovereignty", "Meccan", 77, 2},
    // 68. Al-Qalam
    {68, 52, 5271, "القلم", "Al-Qalam", "The Pen", "Meccan", 2, 2},
    // 69. Al-Haaqqa
    {69, 52, 5323, "الحاقة", "Al-Haaqqa", "The Reality", "Meccan", 78, 2},
    // 70. Al-Ma'aarij
    {70, 44, 5375, "المعارج", "Al-Ma'aarij", "The Ascending Stairways", "Meccan", 79, 2},
    // 71. Nooh
    {71, 28, 5419, "نوح", "Nooh", "Noah", "Meccan", 71, 2},
    // 72. Al-Jinn
    {72, 28, 5447, "الجن", "Al-Jinn", "The Jinn", "Meccan", 40, 2},
    // 73. Al-Muzzammil
    {73, 20, 5475, "المزمل", "Al-Muzzammil", "The Enshrouded One", "Meccan", 3, 2},
    // 74. Al-Muddaththir
    {74, 56, 5495, "المدثر", "Al-Muddaththir", "The Cloaked One", "Meccan", 4, 2},
    // 75. Al-Qiyaama
    {75, 40, 5551, "القيامة", "Al-Qiyaama", "The Resurrection", "Meccan", 31, 2},
    // 76. Al-Insaan
    {76, 31, 5591, "الانسان", "Al-Insaan", "Man", "Medinan", 98, 2},
    // 77. Al-Mursalaat
    {77, 50, 5622, "المرسلات", "Al-Mursalaat", "The Emissaries", "Meccan", 33, 2},
    // 78. An-Naba
    {78, 40, 5672, "النبإ", "An-Naba", "The Announcement", "Meccan", 80, 2},
    // 79. An-Naazi'aat
    {79, 46, 5712, "النازعات", "An-Naazi'aat", "Those who drag forth", "Meccan", 81, 2},
    // 80. Abasa
    {80, 42, 5758, "عبس", "Abasa", "He frowned", "Meccan", 24, 1},
    // 81. At-Takwir
    {81, 29, 5800, "التكوير", "At-Takwir", "The Overthrowing", "Meccan", 7, 1},
    // 82. Al-Infitaar
    {82, 19, 5829, "الإنفطار", "Al-Infitaar", "The Cleaving", "Meccan", 82, 1},
    // 83. Al-Mutaffifin
    {83, 36, 5848, "المطففين", "Al-Mutaffifin", "Defrauding", "Meccan", 86, 1},
    // 84. Al-Inshiqaaq
    {84, 25, 5884, "الإنشقاق", "Al-Inshiqaaq", "The Splitting Open", "Meccan", 83, 1},
    // 85. Al-Burooj
    {85, 22, 5909, "البروج", "Al-Burooj", "The Constellations", "Meccan", 27, 1},
    // 86. At-Taariq
    {86, 17, 5931, "الطارق", "At-Taariq", "The Morning Star", "Meccan", 36, 1},
    // 87. Al-A'laa
    {87, 19, 5948, "الأعلى", "Al-A'laa", "The Most High", "Meccan", 8, 1},
    // 88. Al-Ghaashiya
    {88, 26, 5967, "الغاشية", "Al-Ghaashiya", "The Overwhelming", "Meccan", 68, 1},
    // 89. Al-Fajr
    {89, 30, 5993, "الفجر", "Al-Fajr", "The Dawn", "Meccan", 10, 1},
    // 90. Al-Balad
    {90, 20, 6023, "البلد", "Al-Balad", "The City", "Meccan", 35, 1},
    // 91. Ash-Shams
    {91, 15, 6043, "الشمس", "Ash-Shams", "The Sun", "Meccan", 26, 1},
    // 92. Al-Lail
    {92, 21, 6058, "الليل", "Al-Lail", "The Night", "Meccan", 9, 1},
    // 93. Ad-Dhuhaa
    {93, 11, 6079, "الضحى", "Ad-Dhuhaa", "The Morning Hours", "Meccan", 11, 1},
    // 94. Ash-Sharh
    {94, 8, 6090, "الشرح", "Ash-Sharh", "The Consolation", "Meccan", 12, 1},
    // 95. At-Tin
    {95, 8, 6098, "التين", "At-Tin", "The Fig", "Meccan", 28, 1},
    // 96. Al-Alaq
    {96, 19, 6106, "العلق", "Al-Alaq", "The Clot", "Meccan", 1, 1},
    // 97. Al-Qadr
    {97, 5, 6125, "القدر", "Al-Qadr", "The Power, Fate", "Meccan", 25, 1},
    // 98. Al-Bayyina
    {98, 8, 6130, "البينة", "Al-Bayyina", "The Evidence", "Medinan", 100, 1},
    // 99. Az-Zalzala
    {99, 8, 6138, "الزلزلة", "Az-Zalzala", "The Earthquake", "Medinan", 93, 1},
    // 100. Al-Aadiyaat
    {100, 11, 6146, "العاديات", "Al-Aadiyaat", "The Chargers", "Meccan", 14, 1},
    // 101. Al-Qaari'a
    {101, 11, 6157, "القارعة", "Al-Qaari'a", "The Calamity", "Meccan", 30, 1},
    // 102. At-Takaathur
    {102, 8, 6168, "التكاثر", "At-Takaathur", "Competition", "Meccan", 16, 1},
    // 103. Al-Asr
    {103, 3, 6176, "العصر", "Al-Asr", "The Declining Day, Epoch", "Meccan", 13, 1},
    // 104. Al-Humaza
    {104, 9, 6179, "الهمزة", "Al-Humaza", "The Traducer", "Meccan", 32, 1},
    // 105. Al-Fil
    {105, 5, 6188, "الفيل", "Al-Fil", "The Elephant", "Meccan", 19, 1},
    // 106. Quraish
    {106, 4, 6193, "قريش", "Quraish", "Quraysh", "Meccan", 29, 1},
    // 107. Al-Maa'un
    {107, 7, 6197, "الماعون", "Al-Maa'un", "Almsgiving", "Meccan", 17, 1},
    // 108. Al-Kawthar
    {108, 3, 6204, "الكوثر", "Al-Kawthar", "Abundance", "Meccan", 15, 1},
    // 109. Al-Kaafiroon
    {109, 6, 6207, "الكافرون", "Al-Kaafiroon", "The Disbelievers", "Meccan", 18, 1},
    // 110. An-Nasr
    {110, 3, 6213, "النصر", "An-Nasr", "Divine Support", "Medinan", 114, 1},
    // 111. Al-Masad
    {111, 5, 6216, "المسد", "Al-Masad", "The Palm Fibre", "Meccan", 6, 1},
    // 112. Al-Ikhlaas
    {112, 4, 6221, "الإخلاص", "Al-Ikhlaas", "Sincerity", "Meccan", 22, 1},
    // 113. Al-Falaq
    {113, 5, 6225, "الفلق", "Al-Falaq", "The Dawn", "Meccan", 20, 1},
    // 114. An-Naas
    {114, 6, 6230, "الناس", "An-Naas", "Mankind", "Meccan", 21, 1},
};

// Page to Surah/Ayah mapping (which surah/ayah starts each page)
struct PageLocation {
    int surahNumber;
    int ayahNumber;
};

// Page start locations - first surah/ayah on each page (0-indexed page array)
// Derived from standard Madina Mushaf layout (1-indexed pages in original data)
static const PageLocation PAGE_LOCATIONS[604] = {
    {1, 1},   // Page 0
    {2, 1},   // Page 1
    {2, 6},   // Page 2
    {2, 17},   // Page 3
    {2, 25},   // Page 4
    {2, 30},   // Page 5
    {2, 38},   // Page 6
    {2, 49},   // Page 7
    {2, 58},   // Page 8
    {2, 62},   // Page 9
    {2, 70},   // Page 10
    {2, 77},   // Page 11
    {2, 84},   // Page 12
    {2, 89},   // Page 13
    {2, 94},   // Page 14
    {2, 102},   // Page 15
    {2, 106},   // Page 16
    {2, 113},   // Page 17
    {2, 120},   // Page 18
    {2, 127},   // Page 19
    {2, 135},   // Page 20
    {2, 142},   // Page 21
    {2, 146},   // Page 22
    {2, 154},   // Page 23
    {2, 164},   // Page 24
    {2, 170},   // Page 25
    {2, 177},   // Page 26
    {2, 182},   // Page 27
    {2, 187},   // Page 28
    {2, 191},   // Page 29
    {2, 197},   // Page 30
    {2, 203},   // Page 31
    {2, 211},   // Page 32
    {2, 216},   // Page 33
    {2, 220},   // Page 34
    {2, 225},   // Page 35
    {2, 231},   // Page 36
    {2, 234},   // Page 37
    {2, 238},   // Page 38
    {2, 246},   // Page 39
    {2, 249},   // Page 40
    {2, 253},   // Page 41
    {2, 257},   // Page 42
    {2, 260},   // Page 43
    {2, 265},   // Page 44
    {2, 270},   // Page 45
    {2, 275},   // Page 46
    {2, 282},   // Page 47
    {2, 283},   // Page 48
    {3, 1},   // Page 49
    {3, 10},   // Page 50
    {3, 16},   // Page 51
    {3, 23},   // Page 52
    {3, 30},   // Page 53
    {3, 38},   // Page 54
    {3, 46},   // Page 55
    {3, 53},   // Page 56
    {3, 62},   // Page 57
    {3, 71},   // Page 58
    {3, 78},   // Page 59
    {3, 84},   // Page 60
    {3, 92},   // Page 61
    {3, 101},   // Page 62
    {3, 109},   // Page 63
    {3, 116},   // Page 64
    {3, 122},   // Page 65
    {3, 133},   // Page 66
    {3, 141},   // Page 67
    {3, 149},   // Page 68
    {3, 154},   // Page 69
    {3, 158},   // Page 70
    {3, 166},   // Page 71
    {3, 174},   // Page 72
    {3, 181},   // Page 73
    {3, 187},   // Page 74
    {3, 195},   // Page 75
    {4, 1},   // Page 76
    {4, 7},   // Page 77
    {4, 12},   // Page 78
    {4, 15},   // Page 79
    {4, 20},   // Page 80
    {4, 24},   // Page 81
    {4, 27},   // Page 82
    {4, 34},   // Page 83
    {4, 38},   // Page 84
    {4, 45},   // Page 85
    {4, 52},   // Page 86
    {4, 60},   // Page 87
    {4, 66},   // Page 88
    {4, 75},   // Page 89
    {4, 80},   // Page 90
    {4, 87},   // Page 91
    {4, 92},   // Page 92
    {4, 95},   // Page 93
    {4, 102},   // Page 94
    {4, 106},   // Page 95
    {4, 114},   // Page 96
    {4, 122},   // Page 97
    {4, 128},   // Page 98
    {4, 135},   // Page 99
    {4, 141},   // Page 100
    {4, 148},   // Page 101
    {4, 155},   // Page 102
    {4, 163},   // Page 103
    {4, 171},   // Page 104
    {4, 176},   // Page 105
    {5, 3},   // Page 106
    {5, 6},   // Page 107
    {5, 10},   // Page 108
    {5, 14},   // Page 109
    {5, 18},   // Page 110
    {5, 24},   // Page 111
    {5, 32},   // Page 112
    {5, 37},   // Page 113
    {5, 42},   // Page 114
    {5, 46},   // Page 115
    {5, 51},   // Page 116
    {5, 58},   // Page 117
    {5, 65},   // Page 118
    {5, 71},   // Page 119
    {5, 77},   // Page 120
    {5, 83},   // Page 121
    {5, 90},   // Page 122
    {5, 96},   // Page 123
    {5, 104},   // Page 124
    {5, 109},   // Page 125
    {5, 114},   // Page 126
    {6, 1},   // Page 127
    {6, 9},   // Page 128
    {6, 19},   // Page 129
    {6, 28},   // Page 130
    {6, 36},   // Page 131
    {6, 45},   // Page 132
    {6, 53},   // Page 133
    {6, 60},   // Page 134
    {6, 69},   // Page 135
    {6, 74},   // Page 136
    {6, 82},   // Page 137
    {6, 91},   // Page 138
    {6, 95},   // Page 139
    {6, 102},   // Page 140
    {6, 111},   // Page 141
    {6, 119},   // Page 142
    {6, 125},   // Page 143
    {6, 132},   // Page 144
    {6, 138},   // Page 145
    {6, 143},   // Page 146
    {6, 147},   // Page 147
    {6, 152},   // Page 148
    {6, 158},   // Page 149
    {7, 1},   // Page 150
    {7, 12},   // Page 151
    {7, 23},   // Page 152
    {7, 31},   // Page 153
    {7, 38},   // Page 154
    {7, 44},   // Page 155
    {7, 52},   // Page 156
    {7, 58},   // Page 157
    {7, 68},   // Page 158
    {7, 74},   // Page 159
    {7, 82},   // Page 160
    {7, 88},   // Page 161
    {7, 96},   // Page 162
    {7, 105},   // Page 163
    {7, 121},   // Page 164
    {7, 131},   // Page 165
    {7, 138},   // Page 166
    {7, 144},   // Page 167
    {7, 150},   // Page 168
    {7, 156},   // Page 169
    {7, 160},   // Page 170
    {7, 164},   // Page 171
    {7, 171},   // Page 172
    {7, 179},   // Page 173
    {7, 188},   // Page 174
    {7, 196},   // Page 175
    {8, 1},   // Page 176
    {8, 9},   // Page 177
    {8, 17},   // Page 178
    {8, 26},   // Page 179
    {8, 34},   // Page 180
    {8, 41},   // Page 181
    {8, 46},   // Page 182
    {8, 53},   // Page 183
    {8, 62},   // Page 184
    {8, 70},   // Page 185
    {9, 1},   // Page 186
    {9, 7},   // Page 187
    {9, 14},   // Page 188
    {9, 21},   // Page 189
    {9, 27},   // Page 190
    {9, 32},   // Page 191
    {9, 37},   // Page 192
    {9, 41},   // Page 193
    {9, 48},   // Page 194
    {9, 55},   // Page 195
    {9, 62},   // Page 196
    {9, 69},   // Page 197
    {9, 73},   // Page 198
    {9, 80},   // Page 199
    {9, 87},   // Page 200
    {9, 94},   // Page 201
    {9, 100},   // Page 202
    {9, 107},   // Page 203
    {9, 112},   // Page 204
    {9, 118},   // Page 205
    {9, 123},   // Page 206
    {10, 1},   // Page 207
    {10, 7},   // Page 208
    {10, 15},   // Page 209
    {10, 21},   // Page 210
    {10, 26},   // Page 211
    {10, 34},   // Page 212
    {10, 43},   // Page 213
    {10, 54},   // Page 214
    {10, 62},   // Page 215
    {10, 71},   // Page 216
    {10, 79},   // Page 217
    {10, 89},   // Page 218
    {10, 98},   // Page 219
    {10, 107},   // Page 220
    {11, 6},   // Page 221
    {11, 13},   // Page 222
    {11, 20},   // Page 223
    {11, 29},   // Page 224
    {11, 38},   // Page 225
    {11, 46},   // Page 226
    {11, 54},   // Page 227
    {11, 63},   // Page 228
    {11, 72},   // Page 229
    {11, 82},   // Page 230
    {11, 89},   // Page 231
    {11, 98},   // Page 232
    {11, 109},   // Page 233
    {11, 118},   // Page 234
    {12, 5},   // Page 235
    {12, 15},   // Page 236
    {12, 23},   // Page 237
    {12, 31},   // Page 238
    {12, 38},   // Page 239
    {12, 44},   // Page 240
    {12, 53},   // Page 241
    {12, 64},   // Page 242
    {12, 70},   // Page 243
    {12, 79},   // Page 244
    {12, 87},   // Page 245
    {12, 96},   // Page 246
    {12, 104},   // Page 247
    {13, 1},   // Page 248
    {13, 6},   // Page 249
    {13, 14},   // Page 250
    {13, 19},   // Page 251
    {13, 29},   // Page 252
    {13, 35},   // Page 253
    {13, 43},   // Page 254
    {14, 6},   // Page 255
    {14, 11},   // Page 256
    {14, 19},   // Page 257
    {14, 25},   // Page 258
    {14, 34},   // Page 259
    {14, 43},   // Page 260
    {15, 1},   // Page 261
    {15, 16},   // Page 262
    {15, 32},   // Page 263
    {15, 52},   // Page 264
    {15, 71},   // Page 265
    {15, 91},   // Page 266
    {16, 7},   // Page 267
    {16, 15},   // Page 268
    {16, 27},   // Page 269
    {16, 35},   // Page 270
    {16, 43},   // Page 271
    {16, 55},   // Page 272
    {16, 65},   // Page 273
    {16, 73},   // Page 274
    {16, 80},   // Page 275
    {16, 88},   // Page 276
    {16, 94},   // Page 277
    {16, 103},   // Page 278
    {16, 111},   // Page 279
    {16, 119},   // Page 280
    {17, 1},   // Page 281
    {17, 8},   // Page 282
    {17, 18},   // Page 283
    {17, 28},   // Page 284
    {17, 39},   // Page 285
    {17, 50},   // Page 286
    {17, 59},   // Page 287
    {17, 67},   // Page 288
    {17, 76},   // Page 289
    {17, 87},   // Page 290
    {17, 97},   // Page 291
    {17, 105},   // Page 292
    {18, 5},   // Page 293
    {18, 16},   // Page 294
    {18, 21},   // Page 295
    {18, 28},   // Page 296
    {18, 35},   // Page 297
    {18, 46},   // Page 298
    {18, 54},   // Page 299
    {18, 62},   // Page 300
    {18, 75},   // Page 301
    {18, 84},   // Page 302
    {18, 98},   // Page 303
    {19, 1},   // Page 304
    {19, 12},   // Page 305
    {19, 26},   // Page 306
    {19, 39},   // Page 307
    {19, 52},   // Page 308
    {19, 65},   // Page 309
    {19, 77},   // Page 310
    {19, 96},   // Page 311
    {20, 13},   // Page 312
    {20, 38},   // Page 313
    {20, 52},   // Page 314
    {20, 65},   // Page 315
    {20, 77},   // Page 316
    {20, 88},   // Page 317
    {20, 99},   // Page 318
    {20, 114},   // Page 319
    {20, 126},   // Page 320
    {21, 1},   // Page 321
    {21, 11},   // Page 322
    {21, 25},   // Page 323
    {21, 36},   // Page 324
    {21, 45},   // Page 325
    {21, 58},   // Page 326
    {21, 73},   // Page 327
    {21, 82},   // Page 328
    {21, 91},   // Page 329
    {21, 102},   // Page 330
    {22, 1},   // Page 331
    {22, 6},   // Page 332
    {22, 16},   // Page 333
    {22, 24},   // Page 334
    {22, 31},   // Page 335
    {22, 39},   // Page 336
    {22, 47},   // Page 337
    {22, 56},   // Page 338
    {22, 65},   // Page 339
    {22, 73},   // Page 340
    {23, 1},   // Page 341
    {23, 18},   // Page 342
    {23, 28},   // Page 343
    {23, 43},   // Page 344
    {23, 60},   // Page 345
    {23, 75},   // Page 346
    {23, 90},   // Page 347
    {23, 105},   // Page 348
    {24, 1},   // Page 349
    {24, 11},   // Page 350
    {24, 21},   // Page 351
    {24, 28},   // Page 352
    {24, 32},   // Page 353
    {24, 37},   // Page 354
    {24, 44},   // Page 355
    {24, 54},   // Page 356
    {24, 59},   // Page 357
    {24, 62},   // Page 358
    {25, 3},   // Page 359
    {25, 12},   // Page 360
    {25, 21},   // Page 361
    {25, 33},   // Page 362
    {25, 44},   // Page 363
    {25, 56},   // Page 364
    {25, 68},   // Page 365
    {26, 1},   // Page 366
    {26, 20},   // Page 367
    {26, 40},   // Page 368
    {26, 61},   // Page 369
    {26, 84},   // Page 370
    {26, 112},   // Page 371
    {26, 137},   // Page 372
    {26, 160},   // Page 373
    {26, 184},   // Page 374
    {26, 207},   // Page 375
    {27, 1},   // Page 376
    {27, 14},   // Page 377
    {27, 23},   // Page 378
    {27, 36},   // Page 379
    {27, 45},   // Page 380
    {27, 56},   // Page 381
    {27, 64},   // Page 382
    {27, 77},   // Page 383
    {27, 89},   // Page 384
    {28, 6},   // Page 385
    {28, 14},   // Page 386
    {28, 22},   // Page 387
    {28, 29},   // Page 388
    {28, 36},   // Page 389
    {28, 44},   // Page 390
    {28, 51},   // Page 391
    {28, 60},   // Page 392
    {28, 71},   // Page 393
    {28, 78},   // Page 394
    {28, 85},   // Page 395
    {29, 7},   // Page 396
    {29, 15},   // Page 397
    {29, 24},   // Page 398
    {29, 31},   // Page 399
    {29, 39},   // Page 400
    {29, 46},   // Page 401
    {29, 53},   // Page 402
    {29, 64},   // Page 403
    {30, 6},   // Page 404
    {30, 16},   // Page 405
    {30, 25},   // Page 406
    {30, 33},   // Page 407
    {30, 42},   // Page 408
    {30, 51},   // Page 409
    {31, 1},   // Page 410
    {31, 12},   // Page 411
    {31, 20},   // Page 412
    {31, 29},   // Page 413
    {32, 1},   // Page 414
    {32, 12},   // Page 415
    {32, 21},   // Page 416
    {33, 1},   // Page 417
    {33, 7},   // Page 418
    {33, 16},   // Page 419
    {33, 23},   // Page 420
    {33, 31},   // Page 421
    {33, 36},   // Page 422
    {33, 44},   // Page 423
    {33, 51},   // Page 424
    {33, 55},   // Page 425
    {33, 63},   // Page 426
    {34, 1},   // Page 427
    {34, 8},   // Page 428
    {34, 15},   // Page 429
    {34, 23},   // Page 430
    {34, 32},   // Page 431
    {34, 40},   // Page 432
    {34, 49},   // Page 433
    {35, 4},   // Page 434
    {35, 12},   // Page 435
    {35, 19},   // Page 436
    {35, 31},   // Page 437
    {35, 39},   // Page 438
    {35, 45},   // Page 439
    {36, 13},   // Page 440
    {36, 28},   // Page 441
    {36, 41},   // Page 442
    {36, 55},   // Page 443
    {36, 71},   // Page 444
    {37, 1},   // Page 445
    {37, 25},   // Page 446
    {37, 52},   // Page 447
    {37, 77},   // Page 448
    {37, 103},   // Page 449
    {37, 127},   // Page 450
    {37, 154},   // Page 451
    {38, 1},   // Page 452
    {38, 17},   // Page 453
    {38, 27},   // Page 454
    {38, 43},   // Page 455
    {38, 62},   // Page 456
    {38, 84},   // Page 457
    {39, 6},   // Page 458
    {39, 11},   // Page 459
    {39, 22},   // Page 460
    {39, 32},   // Page 461
    {39, 41},   // Page 462
    {39, 48},   // Page 463
    {39, 57},   // Page 464
    {39, 68},   // Page 465
    {39, 75},   // Page 466
    {40, 8},   // Page 467
    {40, 17},   // Page 468
    {40, 26},   // Page 469
    {40, 34},   // Page 470
    {40, 41},   // Page 471
    {40, 50},   // Page 472
    {40, 59},   // Page 473
    {40, 67},   // Page 474
    {40, 78},   // Page 475
    {41, 1},   // Page 476
    {41, 12},   // Page 477
    {41, 21},   // Page 478
    {41, 30},   // Page 479
    {41, 39},   // Page 480
    {41, 47},   // Page 481
    {42, 1},   // Page 482
    {42, 11},   // Page 483
    {42, 16},   // Page 484
    {42, 23},   // Page 485
    {42, 32},   // Page 486
    {42, 45},   // Page 487
    {42, 52},   // Page 488
    {43, 11},   // Page 489
    {43, 23},   // Page 490
    {43, 34},   // Page 491
    {43, 48},   // Page 492
    {43, 61},   // Page 493
    {43, 74},   // Page 494
    {44, 1},   // Page 495
    {44, 19},   // Page 496
    {44, 40},   // Page 497
    {45, 1},   // Page 498
    {45, 14},   // Page 499
    {45, 23},   // Page 500
    {45, 33},   // Page 501
    {46, 6},   // Page 502
    {46, 15},   // Page 503
    {46, 21},   // Page 504
    {46, 29},   // Page 505
    {47, 1},   // Page 506
    {47, 12},   // Page 507
    {47, 20},   // Page 508
    {47, 30},   // Page 509
    {48, 1},   // Page 510
    {48, 10},   // Page 511
    {48, 16},   // Page 512
    {48, 24},   // Page 513
    {48, 29},   // Page 514
    {49, 5},   // Page 515
    {49, 12},   // Page 516
    {50, 1},   // Page 517
    {50, 16},   // Page 518
    {50, 36},   // Page 519
    {51, 7},   // Page 520
    {51, 31},   // Page 521
    {51, 52},   // Page 522
    {52, 15},   // Page 523
    {52, 32},   // Page 524
    {53, 1},   // Page 525
    {53, 27},   // Page 526
    {53, 45},   // Page 527
    {54, 7},   // Page 528
    {54, 28},   // Page 529
    {54, 50},   // Page 530
    {55, 17},   // Page 531
    {55, 41},   // Page 532
    {55, 68},   // Page 533
    {56, 17},   // Page 534
    {56, 51},   // Page 535
    {56, 77},   // Page 536
    {57, 4},   // Page 537
    {57, 12},   // Page 538
    {57, 19},   // Page 539
    {57, 25},   // Page 540
    {58, 1},   // Page 541
    {58, 7},   // Page 542
    {58, 12},   // Page 543
    {58, 22},   // Page 544
    {59, 4},   // Page 545
    {59, 10},   // Page 546
    {59, 17},   // Page 547
    {60, 1},   // Page 548
    {60, 6},   // Page 549
    {60, 12},   // Page 550
    {61, 6},   // Page 551
    {62, 1},   // Page 552
    {62, 9},   // Page 553
    {63, 5},   // Page 554
    {64, 1},   // Page 555
    {64, 10},   // Page 556
    {65, 1},   // Page 557
    {65, 6},   // Page 558
    {66, 1},   // Page 559
    {66, 8},   // Page 560
    {67, 1},   // Page 561
    {67, 13},   // Page 562
    {67, 27},   // Page 563
    {68, 16},   // Page 564
    {68, 43},   // Page 565
    {69, 9},   // Page 566
    {69, 35},   // Page 567
    {70, 11},   // Page 568
    {70, 40},   // Page 569
    {71, 11},   // Page 570
    {72, 1},   // Page 571
    {72, 14},   // Page 572
    {73, 1},   // Page 573
    {73, 20},   // Page 574
    {74, 18},   // Page 575
    {74, 48},   // Page 576
    {75, 20},   // Page 577
    {76, 6},   // Page 578
    {76, 26},   // Page 579
    {77, 20},   // Page 580
    {78, 1},   // Page 581
    {78, 31},   // Page 582
    {79, 16},   // Page 583
    {80, 1},   // Page 584
    {81, 1},   // Page 585
    {82, 1},   // Page 586
    {83, 7},   // Page 587
    {83, 35},   // Page 588
    {85, 1},   // Page 589
    {86, 1},   // Page 590
    {87, 16},   // Page 591
    {89, 1},   // Page 592
    {89, 24},   // Page 593
    {91, 1},   // Page 594
    {92, 15},   // Page 595
    {95, 1},   // Page 596
    {97, 1},   // Page 597
    {98, 8},   // Page 598
    {100, 10},   // Page 599
    {103, 1},   // Page 600
    {106, 1},   // Page 601
    {109, 1},   // Page 602
    {112, 1},   // Page 603
};

#endif // QURAN_METADATA_H
