#include "screen.h"

#include <algorithm>
#include <format>
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <thread>

#include "controls.h"

Screen::Screen() {
    update();
}

void Screen::update() {
    adb::takeScreenshot();
    screen_image_ = Image::fromScreenshot();
}

void Screen::captureAnswer(int width, int height, bool is_colored, const std::vector<int>& margins) {
    screen_image_.extractAnswer().saveToBitmap(width, height, is_colored, margins);
}

namespace {
    int findClosestColor(const std::vector<cv::Vec3b>& palette, const cv::Vec3b& target) {
        auto distSq = [&](const cv::Vec3b& color) {
            int dr = (int)color[0] - target[0];
            int dg = (int)color[1] - target[1];
            int db = (int)color[2] - target[2];
            return dr*dr + dg*dg + db*db;
        };

        auto comp = [&](const cv::Vec3b& a, const cv::Vec3b& b) {
            return distSq(a) < distSq(b);
        };

        return std::distance(palette.begin(), std::min_element(palette.begin(), palette.end(), comp));
    }
}

void Screen::paint(int width, int height, bool is_colored, bool is_multimode) {
    // if in multimode, tap to the center of the screen once to hide the answer
    if (is_multimode) {
        adb::tap(screen_image_.mat_.cols / 2, screen_image_.mat_.rows / 2);
        // wait until fade animation finishes
        std::this_thread::sleep_for(200ms);
        // update the screen
        update();
    }

    // parse nonogram
    Image nonogram = screen_image_.extractNonogram();
    cv::imwrite("nonogram.png", nonogram.mat_);
    // parse grid and background color
    cv::Vec3b bg_color;
    Image grid = nonogram.extractGrid(bg_color, width, height);
    std::cout << std::format("background color is rgb({}, {}, {})", bg_color[2], bg_color[1], bg_color[0]) << std::endl;
    // calculate cell sizes in pixels
    const double cell_width = (double)grid.mat_.cols / width;
    const double cell_height = (double)grid.mat_.rows / height;
    // calculate x and y positions
    const double x_start = grid.rect_.x + cell_width / 2;
    const double y_start = grid.rect_.y + cell_height / 2;
    double x = x_start;
    double y = y_start;
    if (is_colored) {
        cv::Mat answer = Image::fromBitmap(is_colored).mat_;
        // for debugging
        cv::Mat debug = screen_image_.mat_.clone();
        // parse color palette
        std::vector<cv::Vec3b> palette_colors;
        std::vector<cv::Point> color_coords;
        Image palette = screen_image_.extractPalette(palette_colors, color_coords, nonogram.rect_);
        const int color_count = palette_colors.size();
        palette_colors.push_back(bg_color);

        // the application behaves weirdly on rapid changing of current color,
        // so unlike black & white puzzles when nonogram is filled row by row
        // the colored nonogram will be filled by each color group
        std::vector<std::vector<cv::Point>> color_points(color_count);
        for (int row = 0; row < height; row++, y += cell_height) {
            x = x_start;
            for (int col = 0; col < width; col++, x += cell_width) {
                cv::Vec3b answer_cell_color = answer.at<cv::Vec3b>(cv::Point(col, row));
                int i_color = findClosestColor(palette_colors, answer_cell_color);
                // the last color of the vector is bg color, skip it
                if (i_color == color_count)
                    continue;
                // then tap the cell
                color_points[i_color].push_back({(int)x, (int)y});
                // for debugging
                cv::rectangle(debug, { (int)(x - cell_width / 2), (int)(y - cell_height / 2), (int)cell_width, (int)cell_height }, palette_colors[i_color], cv::FILLED);
            }
        }
        cv::imwrite("debug.png", debug);

        // start painting every color group
        ControlSession ctrl(screen_image_.mat_.cols, screen_image_.mat_.rows);
        for (int i_color = 0; i_color < color_count; i_color++) {
            ctrl.tap(color_coords[i_color].x, color_coords[i_color].y);
            // after tapping the color the application needs some time to apply it
            std::this_thread::sleep_for(500ms);
            // tap every cell of current color
            for (const cv::Point& point : color_points[i_color]) {
                // default touch duration of 5ms may be too fast for application to handle.
                // in case of black & white puzzles the image is completed after the lags are gone
                // but for colored nonograms the colors may not be applied correctly.
                // so it's better to increase touch duration and prevent app from lagging
                ctrl.tap(point.x, point.y, 20ms);
            }
        }
    } else {
        cv::Mat answer = Image::fromBitmap(is_colored).mat_;
        // for debugging
        cv::Mat debug = screen_image_.mat_.clone();
        // start painting
        ControlSession ctrl(screen_image_.mat_.cols, screen_image_.mat_.rows);
        for (int row = 0; row < height; row++, y += cell_height) {
            x = x_start;
            for (int col = 0; col < width; col++, x += cell_width) {
                if (answer.at<uchar>(cv::Point(col, row)) < 230) {
                    ctrl.tap(x, y);
                    cv::rectangle(debug, { (int)(x - cell_width / 2), (int)(y - cell_height / 2), (int)cell_width, (int)cell_height }, cv::Scalar(0, 0, 0), cv::FILLED);
                }
            }
        }
        cv::imwrite("debug.png", debug);
    }
}
