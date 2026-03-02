#pragma once

#include <opencv2/imgproc.hpp>
#include <vector>

// convinient wrapper around cv::Mat that also provides a way of
// accessing its parent rect, which this image was obtained from
class Image {
public:
    Image();
    Image(const cv::Mat& mat);
    Image(const cv::Mat& mat, const cv::Rect& rect);

public:
    // takes screenshot and initialize image from it
    static Image fromScreenshot();
    // loads previously saved bitmap
    // if is_colored is false loads grayscale matrix
    static Image fromBitmap(bool is_colored);

    // save pixels to bitmap
    void saveToBitmap(int nonogram_width, int nonogram_height, bool is_colored, const std::vector<int>& margins);
    // calculate image mask to help mask out background cells
    // if `is_inverted` is false, background colored cells are `1`
    Image getMask(int thresh = 240, bool is_inverted = false);
    // reduce noise on current mask
    Image reduceNoise();

    // retrieve answer image
    // the answer MUST be presented in full screen
    Image extractAnswer();

    // retrieve the whole nonogram grid from the screen
    // the nonogram MUST be in clear state and default position as if it's just opened for the first time
    Image extractNonogram();
    // retrieve the grid of the nonogram (the actual drawing area)
    // as well as its background color
    // MUST be called on the result of `extractNonogram()` function
    Image extractGrid(cv::Vec3b& bg_color, int width, int height);
    // retrieve the palette of the nonogram
    // and put all the colors in corresponding order inside the vector
    Image extractPalette(std::vector<cv::Vec3b>& palette_colors, std::vector<cv::Point>& color_coords, cv::Rect nonogram_rect);

public:
    // opencv matrix
    cv::Mat mat_;
    // if not empty, matches the rect of another image that contains this image
    cv::Rect rect_;
};
