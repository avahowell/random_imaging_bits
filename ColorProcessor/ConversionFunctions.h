#pragma once

#include "Image.h"
#include "NormalizedImage.h"
#include <QPixmap>
#include <QBitmap>
#include <QImage>
#include <stdint.h>
#include <vector>
#include <opencv2/core/core.hpp>
#include "../filterconfig.h"

#ifndef XYZIMAGE_H
#define XYZIMAGE_H

/* Todo:
 * Standardize and document data file formats
 * test other filter bands
 * */
class XYZImage : public RawImage<float> {
	friend class LabImage;
	friend class RGBImage;
public:
	// Constructors:
	XYZImage(const NormalizedImage& input_img, const char* const illuminant_path, const char* const cmf_path);
    XYZImage(const NormalizedImage& input_img, filterconfig* filter);
    XYZImage(const int width, const int height);
	// Weighted average constructor
    XYZImage(std::vector<XYZImage*> images, size_t n_lights, float* weights);
    XYZImage(std::vector<XYZImage *> images, size_t n_lights, std::vector<float> weights);
    XYZImage::XYZImage(std::vector<XYZImage *> images, size_t n_lights, std::vector<float> weights, const cv::Size& dest_size);
    XYZImage(std::vector<XYZImage> images, size_t n_lights, std::vector<float> weights);
	// Rule of 3 (Copy Constructor, Copy Assignment Operator, and Destructor):
	XYZImage(const XYZImage& img);
	XYZImage& operator=(const XYZImage& img);
	~XYZImage();

protected:
	virtual void AllocateImgData();

private:
	static const int XINDEX = 0;
	static const int YINDEX = 1;
	static const int ZINDEX = 2;

    filterconfig *filter_;
};
class LabImage : public RawImage<float> {
public:
	// Constructors:
	LabImage(const XYZImage& input_img);
    // Lab Image crop constructor
    LabImage(const XYZImage &input_img, const cv::Rect& crop);

	// Rule of 3 (Copy Constructor, Copy Assignment Operator, and Destructor):
	LabImage(const LabImage& img);
    LabImage& operator=(const LabImage& img);
    void enhanceContrastAndSaturation();
	~LabImage();
private:
};
class RGBImage : public RawImage<uint8_t> {
public:
	// Class Constructor
	RGBImage(const XYZImage& InputImage);
    RGBImage(const XYZImage& InputImage, const cv::Rect& crop);
    QPixmap getQPixmap();
	// Copy Constructor
	//RGBImage(const RGBImage& img);
	//// Assignment Operator
	//RGBImage& operator=(const RGBImage& img);
	//// Destructor
	//~RGBImage();
private:
};
#endif
