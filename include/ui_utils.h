#ifndef UI_UTILS_H
#define UI_UTILS_H

#include <TFT_eSPI.h>

class UIUtils {
public:
    // Szín sötétítése érintési állapothoz (RGB565)
    static uint16_t darkenColor(uint16_t color, uint8_t percent = 35) {
        uint8_t r = (color >> 11) & 0x1F;
        uint8_t g = (color >> 5) & 0x3F;
        uint8_t b = color & 0x1F;

        uint8_t factor = 100 - percent;
        r = (r * factor) / 100;
        g = (g * factor) / 100;
        b = (b * factor) / 100;

        return (r << 11) | (g << 5) | b;
    }

    // Dinamikus gomb rajzolása (tetszőleges mérettel és pozícióval)
    static void drawButton(TFT_eSPI* tft, int x, int y, int w, int h, const String& label,
                           uint16_t normalBg, uint16_t textCol = TFT_WHITE,
                           bool isPressed = false, uint8_t font = 2, uint8_t radius = 5) {
        
        uint16_t bg = isPressed ? darkenColor(normalBg) : normalBg;

        tft->fillRoundRect(x, y, w, h, radius, bg);
        if (label.length() > 0) {
            tft->setTextDatum(MC_DATUM);
            tft->setTextColor(textCol, bg);
            // Nyomáskor 1 pixellel lejjebb mozdul a szöveg (3D benyomódás hatás)
            int textY = isPressed ? (y + h / 2 + 1) : (y + h / 2);
            tft->drawString(label, x + w / 2, textY, font);
        }
    }

    // Érintési tartomány ellenőrzése
    static bool isTouched(uint16_t tx, uint16_t ty, int x, int y, int w, int h) {
        return (tx >= x && tx <= (x + w) && ty >= y && ty <= (y + h));
    }

    // Globális érintési visszajelzés (Benyomódik -> Várazik -> Visszaáll)
    static void pressFeedback(TFT_eSPI* tft, int x, int y, int w, int h, const String& label,
                              uint16_t normalBg, uint16_t textCol = TFT_WHITE,
                              uint8_t font = 2, uint8_t radius = 5, uint16_t delayMs = 60) {
        
        drawButton(tft, x, y, w, h, label, normalBg, textCol, true, font, radius);
        delay(delayMs);
        drawButton(tft, x, y, w, h, label, normalBg, textCol, false, font, radius);
    }
};

#endif