#pragma once

#include "image.h"
#include "ConversionFunctions.h"
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <QRect>
//Opencv2 includes for registration

class NormalizedImage : public RawImage<float>
{
    friend class XYZImage;
public:
    // Constructors:
    NormalizedImage(const RawImage<float>& input_img, int wtpt_ulx, int wtpt_uly, int wtpt_lrx, int wtpt_lry, std::vector<float> reference_white);
    NormalizedImage(const RawImage<float>& input_img, std::vector<float> wtpt_values, std::vector<float> measured_white);
    NormalizedImage(const RawImage<float>& input_img, std::vector<float> wtpt_values, std::vector<float> measured_white, std::vector<QRect> regtargets);
    NormalizedImage(const RawImage<float>& input_img);
    NormalizedImage(const RawImage<float>& input_img, std::vector<cv::Point2d> regtargets);
    // Rule of 3 (Copy Constructor, Copy Assignment Operator, and Destructor):
    NormalizedImage(const NormalizedImage& img);
    NormalizedImage& operator=(const NormalizedImage& img);
    virtual ~NormalizedImage();
};
