#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <sstream>

using namespace std;

// ============================================================
//  STEGANOGRAPHY LOGIC - UNCHANGED
// ============================================================

vector<int> textToBits(const string &text)
{
    vector<int> bits;
    for (char c : text)
        for (int i = 7; i >= 0; i--)
            bits.push_back((c >> i) & 1);
    return bits;
}

string hideMessageLogic(const string &imageName, const string &message)
{
    int width, height, channels;
    unsigned char *img = stbi_load(imageName.c_str(), &width, &height, &channels, 3);
    if (!img)
        return "ERROR: Image not found!";

    int maxChars = (width * height * 3) / 8 - 20;
    if (message.empty())
    {
        stbi_image_free(img);
        return "ERROR: Message is empty!";
    }
    if ((int)message.size() > maxChars)
    {
        stbi_image_free(img);
        return "ERROR: Message too long!";
    }

    string fullMessage = message + "###SECRETEND###";
    vector<int> bits = textToBits(fullMessage);

    for (int i = 0; i < (int)bits.size(); i++)
        img[i] = (img[i] & 0xFE) | bits[i];

    string outputName = "secret_" + imageName;
    size_t dot = outputName.rfind('.');
    if (dot != string::npos)
        outputName = outputName.substr(0, dot) + ".png";

    stbi_write_png(outputName.c_str(), width, height, 3, img, width * 3);
    stbi_image_free(img);
    return "SUCCESS: Saved as " + outputName;
}

string extractMessageLogic(const string &imageName)
{
    int width, height, channels;
    unsigned char *img = stbi_load(imageName.c_str(), &width, &height, &channels, 3);
    if (!img)
        return "ERROR: Image not found!";

    string message = "";
    char currentChar = 0;
    int bitIndex = 0;
    bool found = false;
    int total = width * height * 3;

    for (int i = 0; i < total; i++)
    {
        int bit = img[i] & 1;
        currentChar = (currentChar << 1) | bit;
        bitIndex++;
        if (bitIndex == 8)
        {
            if (currentChar == '\0')
                break;
            message += currentChar;
            currentChar = 0;
            bitIndex = 0;
            if ((int)message.size() >= 15)
            {
                string end = message.substr(message.size() - 15);
                if (end == "###SECRETEND###")
                {
                    message = message.substr(0, message.size() - 15);
                    found = true;
                    break;
                }
            }
            if ((int)message.size() > width * height)
                break;
        }
    }
    stbi_image_free(img);
    if (!found || message.empty())
        return "ERROR: No hidden message found!";
    return "SUCCESS: " + message;
}

// ============================================================
//  GUI HELPER FUNCTIONS
// ============================================================

void drawRoundedRect(sf::RenderWindow &window, float x, float y,
                     float w, float h, sf::Color fillColor, sf::Color outlineColor)
{
    sf::RectangleShape rect(sf::Vector2f(w, h));
    rect.setPosition(x, y);
    rect.setFillColor(fillColor);
    rect.setOutlineColor(outlineColor);
    rect.setOutlineThickness(1.5f);
    window.draw(rect);
}

bool isMouseOver(float x, float y, float w, float h, sf::Vector2i mousePos)
{
    return mousePos.x >= x && mousePos.x <= x + w &&
           mousePos.y >= y && mousePos.y <= y + h;
}

// ============================================================
//  COLORS
// ============================================================
sf::Color BG_DARK(13, 13, 26);
sf::Color BG_PANEL(26, 26, 46);
sf::Color BG_BUTTON(40, 40, 80);
sf::Color BG_BUTTON_HOVER(60, 60, 120);
sf::Color BG_INPUT(20, 20, 40);
sf::Color COLOR_ACCENT(100, 100, 220);
sf::Color COLOR_GREEN(50, 200, 100);
sf::Color COLOR_RED(220, 60, 60);
sf::Color COLOR_YELLOW(220, 180, 50);
sf::Color COLOR_WHITE(220, 220, 255);
sf::Color COLOR_GRAY(120, 120, 160);
sf::Color COLOR_BORDER(60, 60, 100);

// ============================================================
//  SCREENS
// ============================================================
enum Screen
{
    MENU,
    HIDE,
    EXTRACT,
    COMPARE,
    HOW_IT_WORKS,
    REAL_USES
};

// ============================================================
//  MAIN
// ============================================================
int main()
{
    sf::RenderWindow window(
        sf::VideoMode(700, 550),
        "SecretImage - Steganography Tool",
        sf::Style::Close | sf::Style::Titlebar);
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("C:/Windows/Fonts/consola.ttf"))
        font.loadFromFile("C:/Windows/Fonts/arial.ttf");

    Screen currentScreen = MENU;

    // Input fields
    string inputImage = "";
    string inputMessage = "";
    string inputImage2 = "";
    int activeInput = 0;

    // Result message
    string resultMsg = "";
    sf::Color resultColor = COLOR_GREEN;

    // Cursor blink
    sf::Clock cursorClock;
    bool showCursor = true;

    while (window.isOpen())
    {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        if (cursorClock.getElapsedTime().asSeconds() > 0.5f)
        {
            showCursor = !showCursor;
            cursorClock.restart();
        }

        // ── EVENTS ──
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Escape)
                {
                    if (currentScreen != MENU)
                    {
                        currentScreen = MENU;
                        resultMsg = "";
                        inputImage = "";
                        inputMessage = "";
                        inputImage2 = "";
                    }
                    else
                    {
                        window.close();
                    }
                }
                if (event.key.code == sf::Keyboard::BackSpace)
                {
                    if (activeInput == 1 && !inputImage.empty())
                        inputImage.pop_back();
                    else if (activeInput == 2 && !inputMessage.empty())
                        inputMessage.pop_back();
                    else if (activeInput == 3 && !inputImage2.empty())
                        inputImage2.pop_back();
                }
            }

            if (event.type == sf::Event::TextEntered)
            {
                char c = (char)event.text.unicode;
                if (c >= 32 && c < 127)
                {
                    if (activeInput == 1)
                        inputImage += c;
                    else if (activeInput == 2)
                        inputMessage += c;
                    else if (activeInput == 3)
                        inputImage2 += c;
                }
            }

            if (event.type == sf::Event::MouseButtonPressed)
            {
                float mx = (float)mousePos.x;
                float my = (float)mousePos.y;

                // ── MENU BUTTONS ──
                if (currentScreen == MENU)
                {
                    if (isMouseOver(200, 160, 300, 50, mousePos))
                    {
                        currentScreen = HIDE;
                        resultMsg = "";
                        inputImage = "";
                        inputMessage = "";
                    }
                    if (isMouseOver(200, 225, 300, 50, mousePos))
                    {
                        currentScreen = EXTRACT;
                        resultMsg = "";
                        inputImage = "";
                    }
                    if (isMouseOver(200, 290, 300, 50, mousePos))
                    {
                        currentScreen = COMPARE;
                        resultMsg = "";
                        inputImage = "";
                        inputImage2 = "";
                    }
                    if (isMouseOver(200, 355, 300, 50, mousePos))
                    {
                        currentScreen = HOW_IT_WORKS;
                    }
                    if (isMouseOver(200, 420, 300, 50, mousePos))
                    {
                        currentScreen = REAL_USES;
                    }
                    if (isMouseOver(560, 490, 120, 40, mousePos))
                    {
                        window.close();
                    }
                }

                // ── HIDE SCREEN ──
                if (currentScreen == HIDE)
                {
                    if (isMouseOver(60, 200, 580, 40, mousePos))
                        activeInput = 1;
                    else if (isMouseOver(60, 300, 580, 40, mousePos))
                        activeInput = 2;
                    else
                        activeInput = 0;

                    // Run button
                    if (isMouseOver(250, 380, 200, 45, mousePos))
                    {
                        resultMsg = hideMessageLogic(inputImage, inputMessage);
                        resultColor = (resultMsg.substr(0, 5) == "ERROR") ? COLOR_RED : COLOR_GREEN;
                    }
                    // Back button
                    if (isMouseOver(20, 500, 100, 35, mousePos))
                    {
                        currentScreen = MENU;
                        resultMsg = "";
                    }
                }

                // ── EXTRACT SCREEN ──
                if (currentScreen == EXTRACT)
                {
                    if (isMouseOver(60, 200, 580, 40, mousePos))
                        activeInput = 1;
                    else
                        activeInput = 0;

                    if (isMouseOver(250, 300, 200, 45, mousePos))
                    {
                        resultMsg = extractMessageLogic(inputImage);
                        resultColor = (resultMsg.substr(0, 5) == "ERROR") ? COLOR_RED : COLOR_GREEN;
                    }
                    if (isMouseOver(20, 500, 100, 35, mousePos))
                    {
                        currentScreen = MENU;
                        resultMsg = "";
                    }
                }

                // ── COMPARE SCREEN ──
                if (currentScreen == COMPARE)
                {
                    if (isMouseOver(60, 200, 580, 40, mousePos))
                        activeInput = 1;
                    else if (isMouseOver(60, 300, 580, 40, mousePos))
                        activeInput = 3;
                    else
                        activeInput = 0;

                    if (isMouseOver(250, 380, 200, 45, mousePos))
                    {
                        int w1, h1, c1, w2, h2, c2;
                        unsigned char *i1 = stbi_load(inputImage.c_str(), &w1, &h1, &c1, 3);
                        unsigned char *i2 = stbi_load(inputImage2.c_str(), &w2, &h2, &c2, 3);
                        if (!i1 || !i2)
                        {
                            resultMsg = "ERROR: Image(s) not found!";
                            resultColor = COLOR_RED;
                        }
                        else if (w1 != w2 || h1 != h2)
                        {
                            resultMsg = "ERROR: Different sizes!";
                            resultColor = COLOR_RED;
                        }
                        else
                        {
                            long long diff = 0, total = (long long)w1 * h1 * 3;
                            for (int i = 0; i < total; i++)
                            {
                                if (i1[i] != i2[i])
                                    diff++;
                            }
                            double sim = 100.0 - (diff * 100.0) / total;
                            ostringstream oss;
                            oss.precision(4);
                            oss << fixed << sim;
                            resultMsg = "Similarity: " + oss.str() + "%";
                            resultColor = sim > 99.9 ? COLOR_GREEN : COLOR_YELLOW;
                        }
                        if (i1)
                            stbi_image_free(i1);
                        if (i2)
                            stbi_image_free(i2);
                    }
                    if (isMouseOver(20, 500, 100, 35, mousePos))
                    {
                        currentScreen = MENU;
                        resultMsg = "";
                    }
                }

                // Back from info screens
                if (currentScreen == HOW_IT_WORKS || currentScreen == REAL_USES)
                {
                    if (isMouseOver(20, 500, 100, 35, mousePos))
                        currentScreen = MENU;
                }
            }
        }

        // ── DRAW ──
        window.clear(BG_DARK);

        // ══ MENU ══
        if (currentScreen == MENU)
        {
            // Header
            drawRoundedRect(window, 0, 0, 700, 140, BG_PANEL, BG_DARK);

            sf::Text title("SecretImage", font, 36);
            title.setFillColor(COLOR_WHITE);
            title.setPosition(200, 20);
            window.draw(title);

            sf::Text sub("Steganography Tool  -  Hide Messages Inside Images", font, 14);
            sub.setFillColor(COLOR_GRAY);
            sub.setPosition(115, 70);
            window.draw(sub);

            sf::Text lsb("LSB Method  |  C++ Project  |  Cybersecurity", font, 12);
            lsb.setFillColor(COLOR_ACCENT);
            lsb.setPosition(170, 100);
            window.draw(lsb);

            // Buttons
            struct Btn
            {
                string label;
                float y;
                sf::Color col;
            };
            vector<Btn> btns = {
                {"[1]  Hide Message", 160, COLOR_GREEN},
                {"[2]  Extract Message", 225, COLOR_YELLOW},
                {"[3]  Compare Images", 290, COLOR_ACCENT},
                {"[4]  How It Works", 355, COLOR_WHITE},
                {"[5]  Real World Uses", 420, COLOR_WHITE},
            };

            for (auto &b : btns)
            {
                bool hover = isMouseOver(200, b.y, 300, 50, mousePos);
                drawRoundedRect(window, 200, b.y, 300, 50,
                                hover ? BG_BUTTON_HOVER : BG_BUTTON, COLOR_BORDER);
                sf::Text t(b.label, font, 16);
                t.setFillColor(hover ? sf::Color::White : b.col);
                t.setPosition(230, b.y + 15);
                window.draw(t);
            }

            // Exit button
            bool exitHover = isMouseOver(560, 490, 120, 40, mousePos);
            drawRoundedRect(window, 560, 490, 120, 40,
                            exitHover ? COLOR_RED : sf::Color(80, 20, 20), COLOR_RED);
            sf::Text exitT("Exit [6]", font, 14);
            exitT.setFillColor(COLOR_WHITE);
            exitT.setPosition(578, 502);
            window.draw(exitT);

            sf::Text esc("Press ESC to exit", font, 11);
            esc.setFillColor(COLOR_GRAY);
            esc.setPosition(20, 530);
            window.draw(esc);
        }

        // ══ HIDE SCREEN ══
        if (currentScreen == HIDE)
        {
            drawRoundedRect(window, 0, 0, 700, 80, BG_PANEL, BG_DARK);
            sf::Text t("Hide Secret Message", font, 24);
            t.setFillColor(COLOR_GREEN);
            t.setPosition(220, 22);
            window.draw(t);

            sf::Text l1("Image name (example: photo.png):", font, 14);
            l1.setFillColor(COLOR_GRAY);
            l1.setPosition(60, 170);
            window.draw(l1);

            drawRoundedRect(window, 60, 200, 580, 40,
                            activeInput == 1 ? BG_INPUT : sf::Color(15, 15, 30), COLOR_ACCENT);
            sf::Text v1(inputImage + (activeInput == 1 && showCursor ? "|" : ""), font, 14);
            v1.setFillColor(COLOR_WHITE);
            v1.setPosition(70, 210);
            window.draw(v1);

            sf::Text l2("Secret message:", font, 14);
            l2.setFillColor(COLOR_GRAY);
            l2.setPosition(60, 270);
            window.draw(l2);

            drawRoundedRect(window, 60, 300, 580, 40,
                            activeInput == 2 ? BG_INPUT : sf::Color(15, 15, 30), COLOR_ACCENT);
            sf::Text v2(inputMessage + (activeInput == 2 && showCursor ? "|" : ""), font, 14);
            v2.setFillColor(COLOR_WHITE);
            v2.setPosition(70, 310);
            window.draw(v2);

            bool runHover = isMouseOver(250, 380, 200, 45, mousePos);
            drawRoundedRect(window, 250, 380, 200, 45,
                            runHover ? COLOR_GREEN : sf::Color(20, 80, 40), COLOR_GREEN);
            sf::Text run("Hide Message", font, 16);
            run.setFillColor(COLOR_WHITE);
            run.setPosition(278, 393);
            window.draw(run);

            if (!resultMsg.empty())
            {
                sf::Text res(resultMsg, font, 13);
                res.setFillColor(resultColor);
                res.setPosition(60, 445);
                window.draw(res);
            }

            drawRoundedRect(window, 20, 500, 100, 35,
                            isMouseOver(20, 500, 100, 35, mousePos) ? BG_BUTTON_HOVER : BG_BUTTON, COLOR_BORDER);
            sf::Text back("< Back", font, 14);
            back.setFillColor(COLOR_WHITE);
            back.setPosition(35, 510);
            window.draw(back);
        }

        // ══ EXTRACT SCREEN ══
        if (currentScreen == EXTRACT)
        {
            drawRoundedRect(window, 0, 0, 700, 80, BG_PANEL, BG_DARK);
            sf::Text t("Extract Secret Message", font, 24);
            t.setFillColor(COLOR_YELLOW);
            t.setPosition(190, 22);
            window.draw(t);

            sf::Text l1("Secret image name:", font, 14);
            l1.setFillColor(COLOR_GRAY);
            l1.setPosition(60, 170);
            window.draw(l1);

            drawRoundedRect(window, 60, 200, 580, 40,
                            activeInput == 1 ? BG_INPUT : sf::Color(15, 15, 30), COLOR_ACCENT);
            sf::Text v1(inputImage + (activeInput == 1 && showCursor ? "|" : ""), font, 14);
            v1.setFillColor(COLOR_WHITE);
            v1.setPosition(70, 210);
            window.draw(v1);

            bool runHover = isMouseOver(250, 300, 200, 45, mousePos);
            drawRoundedRect(window, 250, 300, 200, 45,
                            runHover ? COLOR_YELLOW : sf::Color(80, 60, 20), COLOR_YELLOW);
            sf::Text run("Extract", font, 16);
            run.setFillColor(COLOR_WHITE);
            run.setPosition(308, 313);
            window.draw(run);

            if (!resultMsg.empty())
            {
                sf::Text res(resultMsg, font, 13);
                res.setFillColor(resultColor);
                res.setPosition(60, 380);
                window.draw(res);
            }

            drawRoundedRect(window, 20, 500, 100, 35,
                            isMouseOver(20, 500, 100, 35, mousePos) ? BG_BUTTON_HOVER : BG_BUTTON, COLOR_BORDER);
            sf::Text back("< Back", font, 14);
            back.setFillColor(COLOR_WHITE);
            back.setPosition(35, 510);
            window.draw(back);
        }

        // ══ COMPARE SCREEN ══
        if (currentScreen == COMPARE)
        {
            drawRoundedRect(window, 0, 0, 700, 80, BG_PANEL, BG_DARK);
            sf::Text t("Compare Two Images", font, 24);
            t.setFillColor(COLOR_ACCENT);
            t.setPosition(210, 22);
            window.draw(t);

            sf::Text l1("Original image:", font, 14);
            l1.setFillColor(COLOR_GRAY);
            l1.setPosition(60, 170);
            window.draw(l1);

            drawRoundedRect(window, 60, 200, 580, 40,
                            activeInput == 1 ? BG_INPUT : sf::Color(15, 15, 30), COLOR_ACCENT);
            sf::Text v1(inputImage + (activeInput == 1 && showCursor ? "|" : ""), font, 14);
            v1.setFillColor(COLOR_WHITE);
            v1.setPosition(70, 210);
            window.draw(v1);

            sf::Text l2("Secret image:", font, 14);
            l2.setFillColor(COLOR_GRAY);
            l2.setPosition(60, 270);
            window.draw(l2);

            drawRoundedRect(window, 60, 300, 580, 40,
                            activeInput == 3 ? BG_INPUT : sf::Color(15, 15, 30), COLOR_ACCENT);
            sf::Text v2(inputImage2 + (activeInput == 3 && showCursor ? "|" : ""), font, 14);
            v2.setFillColor(COLOR_WHITE);
            v2.setPosition(70, 310);
            window.draw(v2);

            bool runHover = isMouseOver(250, 380, 200, 45, mousePos);
            drawRoundedRect(window, 250, 380, 200, 45,
                            runHover ? COLOR_ACCENT : sf::Color(30, 30, 80), COLOR_ACCENT);
            sf::Text run("Compare", font, 16);
            run.setFillColor(COLOR_WHITE);
            run.setPosition(300, 393);
            window.draw(run);

            if (!resultMsg.empty())
            {
                sf::Text res(resultMsg, font, 14);
                res.setFillColor(resultColor);
                res.setPosition(60, 450);
                window.draw(res);
            }

            drawRoundedRect(window, 20, 500, 100, 35,
                            isMouseOver(20, 500, 100, 35, mousePos) ? BG_BUTTON_HOVER : BG_BUTTON, COLOR_BORDER);
            sf::Text back("< Back", font, 14);
            back.setFillColor(COLOR_WHITE);
            back.setPosition(35, 510);
            window.draw(back);
        }

        // ══ HOW IT WORKS ══
        if (currentScreen == HOW_IT_WORKS)
        {
            drawRoundedRect(window, 0, 0, 700, 80, BG_PANEL, BG_DARK);
            sf::Text t("How Steganography Works", font, 22);
            t.setFillColor(COLOR_WHITE);
            t.setPosition(180, 25);
            window.draw(t);

            vector<pair<string, sf::Color>> lines = {
                {"Step 1 - Every image = millions of pixels", COLOR_ACCENT},
                {"        Each pixel has R, G, B values (0-255)", COLOR_GRAY},
                {"", COLOR_GRAY},
                {"Step 2 - Each value = 8 binary digits", COLOR_ACCENT},
                {"        Example: 202 = 1100101[0]  <- LSB", COLOR_GRAY},
                {"", COLOR_GRAY},
                {"Step 3 - We change only the LAST bit", COLOR_ACCENT},
                {"        Before: 11001010  After: 11001011", COLOR_GRAY},
                {"        Difference = 1 out of 255 (invisible!)", COLOR_GREEN},
                {"", COLOR_GRAY},
                {"Step 4 - A 1000x1000 image can hide 375,000 chars!", COLOR_YELLOW},
            };

            float y = 110;
            for (auto &l : lines)
            {
                sf::Text txt(l.first, font, 14);
                txt.setFillColor(l.second);
                txt.setPosition(50, y);
                window.draw(txt);
                y += 32;
            }

            drawRoundedRect(window, 20, 500, 100, 35,
                            isMouseOver(20, 500, 100, 35, mousePos) ? BG_BUTTON_HOVER : BG_BUTTON, COLOR_BORDER);
            sf::Text back("< Back", font, 14);
            back.setFillColor(COLOR_WHITE);
            back.setPosition(35, 510);
            window.draw(back);
        }

        // ══ REAL USES ══
        if (currentScreen == REAL_USES)
        {
            drawRoundedRect(window, 0, 0, 700, 80, BG_PANEL, BG_DARK);
            sf::Text t("Real World Uses", font, 26);
            t.setFillColor(COLOR_YELLOW);
            t.setPosition(240, 22);
            window.draw(t);

            vector<pair<string, sf::Color>> lines = {
                {"Intelligence & Military", COLOR_RED},
                {"  Sending secret orders hidden in normal images", COLOR_GRAY},
                {"", COLOR_GRAY},
                {"Journalism in Repressive Countries", COLOR_YELLOW},
                {"  Smuggling documents hidden in innocent images", COLOR_GRAY},
                {"", COLOR_GRAY},
                {"Medical Sector", COLOR_GREEN},
                {"  Hiding patient data inside X-ray images", COLOR_GRAY},
                {"", COLOR_GRAY},
                {"Digital Forensics", COLOR_ACCENT},
                {"  Tracking sources of information leaks", COLOR_GRAY},
            };

            float y = 110;
            for (auto &l : lines)
            {
                sf::Text txt(l.first, font, 14);
                txt.setFillColor(l.second);
                txt.setPosition(50, y);
                window.draw(txt);
                y += 32;
            }

            drawRoundedRect(window, 20, 500, 100, 35,
                            isMouseOver(20, 500, 100, 35, mousePos) ? BG_BUTTON_HOVER : BG_BUTTON, COLOR_BORDER);
            sf::Text back("< Back", font, 14);
            back.setFillColor(COLOR_WHITE);
            back.setPosition(35, 510);
            window.draw(back);
        }

        window.display();
    }
    return 0;
}