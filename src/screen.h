#pragma once

#include <vector>

#include "image.h"

// represents the device screen controller
class Screen {
public:
    // on every contstruction the screenshot is taken
    Screen();

    // make a new screenshot
    void update();

public:
    // capture an answer picture from the screen
    // width and height correspond to the actual nonogram sizes
    void captureAnswer(int width, int height, bool is_colored, const std::vector<int>& margins);

    // paints the answer on the nonogram grid
    // width and height correspond to the actual nonogram sizes
    void paint(int width, int height, bool is_colored, bool is_multimode);

private:
    Image screen_image_;
};
