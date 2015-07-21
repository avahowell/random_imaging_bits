#include "calibratedimage.h"

CalibratedImage::CalibratedImage(const RawImage<unsigned short>& raw_img, const FlatFieldImage& flat_img) : RawImage<float>(raw_img.num_, raw_img.width_, raw_img.height_)
{
    const size_t img_size = width_ * height_;
    for (size_t n = 0; n < num_; ++n) {
        for (size_t i = 0; i < img_size; ++i) {
            if (flat_img.img_data_[n][i] == 0) {
                img_data_[n][i] = 0; // Temporary fix to prevent dividing by 0. Maybe change this in the future to guess a value?
            } else {
                img_data_[n][i] = float(raw_img.img_data_[n][i]) / float(flat_img.img_data_[n][i]);
            }
        }
    }
}
