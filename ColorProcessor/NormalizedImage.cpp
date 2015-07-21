#include <fstream>
#include <algorithm>

#include "NormalizedImage.h"
#include <iostream>
#include <algorithm>
#include <opencv2/core/core.hpp>

// NormalizedImage constructor.
NormalizedImage::NormalizedImage(const RawImage<float>& input_img, int wtpt_ulx, int wtpt_uly, int wtpt_lrx, int wtpt_lry, std::vector<float> reference_white) : RawImage<float>(input_img.num_, input_img.width_, input_img.height_)
{
    std::vector<float> measured_values;
    std::vector<float> area_values;
    for (auto filter_index = 0; filter_index < num_; ++filter_index) {
        for (auto y = wtpt_uly; y < wtpt_lry; ++y) {
            for (auto x = wtpt_ulx; x < wtpt_lrx; ++x) {
                area_values.push_back(input_img.img_data_[filter_index][(y * width_) + x]);
            }
        }
        std::nth_element(area_values.begin(), area_values.begin() + area_values.size()/2, area_values.end());
        measured_values.push_back(area_values[area_values.size()/2]);
    }
    for (size_t filter_index = 0; filter_index < num_; ++filter_index) {
        //normalize the data
        for (size_t y = 0; y < height_; ++y) {
            for (size_t x = 0; x < width_; ++x) {
            //divide by the median and multiply by wtpt_value
            img_data_[filter_index][(y * width_) + x] = input_img.img_data_[filter_index][(y * width_) + x] / (measured_values[filter_index] / reference_white[filter_index]);
            }
        }
    }
}
NormalizedImage::NormalizedImage(const RawImage<float>& input_img) : RawImage<float>(input_img.num_, input_img.width_, input_img.height_)
{
    for (auto filter_index = 0; filter_index < num_; ++filter_index) {
        cv::Mat src(height_, width_, CV_32F, input_img.img_data_[filter_index]);
        cv::Mat blurred;
        cv::medianBlur(src, blurred, 3);

        double min, max;
        cv::Point minLoc, maxLoc;

        cv::minMaxLoc(blurred, &min, &max, &minLoc, &maxLoc);
        for (auto y = 0; y < height_; ++y) {
            for (auto x = 0; x < width_; ++x) {
                img_data_[filter_index][(y * width_) + x] = input_img.img_data_[filter_index][(y* width_) + x] * (1.0 / max);
            }
        }
    }
}
NormalizedImage::NormalizedImage(const RawImage<float>& input_img, std::vector<cv::Point2d> regtargets) : RawImage<float>(input_img.num_, input_img.width_, input_img.height_)
{
    for (auto filter_index = 0; filter_index < num_; ++filter_index) {
        cv::Mat src(height_, width_, CV_32F, input_img.img_data_[filter_index]);
        cv::Mat blurred;
        cv::medianBlur(src, blurred, 3);

        double min, max;
        cv::Point minLoc, maxLoc;

        cv::minMaxLoc(blurred, &min, &max, &minLoc, &maxLoc);
        for (auto y = 0; y < height_; ++y) {
            for (auto x = 0; x < width_; ++x) {
                img_data_[filter_index][(y * width_) + x] = input_img.img_data_[filter_index][(y* width_) + x] * (1.0 / max);
            }
        }
        delete input_img.img_data_[filter_index];
    }
    if (regtargets.size() != 2) {
        std::cout << "Registration only supports 2 targets.  Please construct a vector<Point2d> with 2 points." << std::endl;
        return;
    }

    for (auto filter_index = 0; filter_index < num_; ++filter_index) {
        cv::Mat source(height_, width_, CV_32F, img_data_[filter_index]);
        cv::Mat target(height_, width_, CV_32F, img_data_[num_/2]);      // Target filter is num_/2 for now, most likely want to change this to the filter with the most information content

        int reg_size = 50;

        cv::Mat reg0_source, reg0_target, reg1_source, reg1_target;

        reg0_source = source(cv::Range(regtargets[0].y - (reg_size/2), regtargets[0].y + (reg_size/2)), cv::Range(regtargets[0].x - (reg_size/2), regtargets[0].x + (reg_size/2)));
        reg0_target = target(cv::Range(regtargets[0].y - (reg_size/2), regtargets[0].y + (reg_size/2)), cv::Range(regtargets[0].x - (reg_size/2), regtargets[0].x + (reg_size/2)));
        reg1_source = source(cv::Range(regtargets[1].y - (reg_size/2), regtargets[1].y + (reg_size/2)), cv::Range(regtargets[1].x - (reg_size/2), regtargets[1].x + (reg_size/2)));
        reg1_target = target(cv::Range(regtargets[1].y - (reg_size/2), regtargets[1].y + (reg_size/2)), cv::Range(regtargets[1].x - (reg_size/2), regtargets[1].x + (reg_size/2)));

        cv::Point2d offset_center = cv::phaseCorrelate(reg0_source, reg0_target);
        cv::Point2d offset_corner = cv::phaseCorrelate(reg1_source, reg1_target);

        float r, r_prime, deltaX, deltaY;

        deltaX = offset_corner.x - offset_center.x;
        deltaY = offset_corner.y - offset_center.y;

        r = pow((regtargets[1].x - regtargets[0].x), 2) + pow((regtargets[1].y - regtargets[0].y), 2);
        r = sqrt(r);

        r_prime = pow((regtargets[1].x - regtargets[0].x + deltaX), 2) + pow((regtargets[1].y - regtargets[0].y + deltaY), 2);
        r_prime = sqrt(r_prime);

        float scale = r_prime / r;
        float trans_x = (source.size().width  / 2) * (1-scale);
        float trans_y = (source.size().height / 2) * (1-scale);

        std::vector<float> matrix_data(6);
        matrix_data[0] = scale;
        matrix_data[1] = 0;
        matrix_data[2] = trans_x;
        matrix_data[3] = 0;
        matrix_data[4] = scale;
        matrix_data[5] = trans_y;

        cv::Mat affine(2, 3, CV_32F, matrix_data.data());
        cv::Mat scaled;
        cv::warpAffine(source, scaled, affine, source.size());

        cv::Mat scaled_target = scaled(cv::Range(regtargets[0].y - (reg_size/2), regtargets[0].y + (reg_size/2)), cv::Range(regtargets[0].x - (reg_size/2), regtargets[0].x + (reg_size/2)));
        cv::Point2d translation_offset = cv::phaseCorrelate(scaled_target, reg0_target);

        matrix_data[2] += translation_offset.x;
        matrix_data[5] += translation_offset.y;

        cv::warpAffine(source, source, affine, source.size());
    }
}

NormalizedImage::NormalizedImage(const RawImage<float>& input_img, std::vector<float> wtpt_values, std::vector<float> measured_white) : RawImage<float>(input_img.num_, input_img.width_, input_img.height_)
{
    for (size_t filter_index = 0; filter_index < num_; ++filter_index) {
        //normalize the data
        for (size_t y = 0; y < height_; ++y) {
            for (size_t x = 0; x < width_; ++x) {
            //divide by the median and multiply by wtpt_value
            img_data_[filter_index][(y * width_) + x] = input_img.img_data_[filter_index][(y * width_) + x] / (measured_white[filter_index] / wtpt_values[filter_index]);
            }
        }
    }
}
NormalizedImage::NormalizedImage(const RawImage<float>& input_img, std::vector<float> wtpt_values, std::vector<float> measured_white, std::vector<QRect> regtargets) : RawImage<float>(input_img.num_, input_img.width_, input_img.height_)
{
    for (size_t filter_index = 0; filter_index < num_; ++filter_index) {
        //normalize the data
        for (size_t y = 0; y < height_; ++y) {
            for (size_t x = 0; x < width_; ++x) {
            //divide by the median and multiply by wtpt_value
            img_data_[filter_index][(y * width_) + x] = input_img.img_data_[filter_index][(y * width_) + x] / (measured_white[filter_index] / wtpt_values[filter_index]);
            }
        }
    }
    std::cout << "Normalization complete" << std::endl;

    if (regtargets.size() != 2) {
        std::cout << "Registration only supports 2 targets.  Please construct a vector<Point2d> with 2 points." << std::endl;
        return;
    }

    for (auto filter_index = 0; filter_index < num_; ++filter_index) {
        cv::Mat source(height_, width_, CV_32F, img_data_[filter_index]);
        cv::Mat target(height_, width_, CV_32F, img_data_[num_/2]);      // Target filter is num_/2 for now, most likely want to change this to the filter with the most information content

        cv::Mat reg0_source, reg0_target, reg1_source, reg1_target;

        reg0_source = source(cv::Range(regtargets[0].y(), regtargets[0].y() + regtargets[0].size().height()), cv::Range(regtargets[0].x(), regtargets[0].x() + regtargets[0].size().width()));
        reg0_target = target(cv::Range(regtargets[0].y(), regtargets[0].y() + regtargets[0].size().height()), cv::Range(regtargets[0].x(), regtargets[0].x() + regtargets[0].size().width()));// - (reg_size/2), regtargets[0].y + (reg_size/2)), cv::Range(regtargets[0].x - (reg_size/2), regtargets[0].x + (reg_size/2)));
        reg1_source = source(cv::Range(regtargets[1].y(), regtargets[1].y() + regtargets[1].size().height()), cv::Range(regtargets[1].x(), regtargets[1].x() + regtargets[1].size().width()));// - (reg_size/2), regtargets[1].y + (reg_size/2)), cv::Range(regtargets[1].x - (reg_size/2), regtargets[1].x + (reg_size/2)));
        reg1_target = target(cv::Range(regtargets[1].y(), regtargets[1].y() + regtargets[1].size().height()), cv::Range(regtargets[1].x(), regtargets[1].x() + regtargets[1].size().width()));// - (reg_size/2), regtargets[1].y + (reg_size/2)), cv::Range(regtargets[1].x - (reg_size/2), regtargets[1].x + (reg_size/2)));


        cv::Point2d offset_center = cv::phaseCorrelate(reg0_source, reg0_target);
        cv::Point2d offset_corner = cv::phaseCorrelate(reg1_source, reg1_target);

        float r, r_prime, deltaX, deltaY;

        deltaX = offset_corner.x - offset_center.x;
        deltaY = offset_corner.y - offset_center.y;

        r = pow((regtargets[1].center().x() - regtargets[0].center().x()), 2) + pow((regtargets[1].center().y() - regtargets[0].center().y()), 2);
        r = sqrt(r);

        r_prime = pow((regtargets[1].center().x() - regtargets[0].center().x() + deltaX), 2) + pow((regtargets[1].center().y() - regtargets[0].center().y() + deltaY), 2);
        r_prime = sqrt(r_prime);


        float scale = r_prime / r;
        float trans_x = (source.size().width  / 2) * (1-scale);
        float trans_y = (source.size().height / 2) * (1-scale);

        std::vector<float> matrix_data(6);
        matrix_data[0] = scale;
        matrix_data[1] = 0;
        matrix_data[2] = trans_x;
        matrix_data[3] = 0;
        matrix_data[4] = scale;
        matrix_data[5] = trans_y;

        cv::Mat affine(2, 3, CV_32F, matrix_data.data());
        cv::Mat scaled;
        cv::warpAffine(source, scaled, affine, source.size());

        cv::Mat scaled_target = scaled(cv::Range(regtargets[0].y(), regtargets[0].y() + regtargets[0].size().height()), cv::Range(regtargets[0].x(), regtargets[0].x() + regtargets[0].size().width()));
        cv::Point2d translation_offset = cv::phaseCorrelate(scaled_target, reg0_target);


        matrix_data[2] += translation_offset.x;
        matrix_data[5] += translation_offset.y;

        cv::warpAffine(source, source, affine, source.size());
    }
}

// NormalizedImage copy constructor.
// NOTE: This calls RawImage's copy constructor since no other data needs to be coppied.
NormalizedImage::NormalizedImage(const NormalizedImage& img) : RawImage<float>(img)
{ }

// NormalizedImage assignment operator.
NormalizedImage& NormalizedImage::operator=(const NormalizedImage& img)
{
    if (this != &img) {
        RawImage::operator=(img);
    }
    return *this;
}

// NormalizedImage destructor.
// NOTE: the base class's (RawImage) destructor is automatically called,
// so there is no need to call it again in this destructor.
NormalizedImage::~NormalizedImage()
{
}
