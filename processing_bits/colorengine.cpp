#include "colorengine.h"

void colorengine::threadFunc()
{
    std::unique_ptr<float> regdata(new float[width_*height_]);

    std::vector<float> scalar_constant(3);
    scalar_constant.push_back(0);
    scalar_constant.push_back(0);
    scalar_constant.push_back(0);

    _XTIFFInitialize();
    rawdata_tiff = TIFFOpen(raw_tiff_path.c_str(), "w");
    TIFFSetField(rawdata_tiff, TIFFTAG_NFILTERS, filter_->nfilters());
    TIFFSetField(rawdata_tiff, TIFFTAG_NLIGHTS, nlights_);

    for (auto filter_index = 0; filter_index < filter_->nfilters(); ++filter_index) {
        std::vector<float> cmf = filter_->cmfValues(filter_->wavelengthAtPos(filter_index));
        float illuminant = filter_->illuminantValue(filter_->wavelengthAtPos(filter_index));
        for (auto xyz_index = 0; xyz_index < 3; ++xyz_index) {
            scalar_constant[xyz_index] += cmf[xyz_index] * illuminant;
        }
    }
    int page_index = 0;
    for (int filter_index = 0; filter_index < filter_->nfilters(); ++filter_index) {  
        for (int light_index = 0; light_index < nlights_; ++light_index) {
            std::shared_ptr<unsigned short> data = data_queue_.pop();
            if (cancel_) {
                return;
            }
            std::unique_ptr<float> floatdata(new float[width_*height_]);

            for (auto y = 0; y < height_; ++y) {
                for (auto x = 0; x < width_; ++x) {
                    if (data.get()[y*width_+x] - bias_data.get()[y*width_+x] < 0) {
                        data.get()[y*width_+x] = 0;
                    } else {
                        data.get()[y*width_+x] -= bias_data.get()[y*width_+x];
                    }
                }
            }
            for (auto y = 0; y < height_; ++y) {
                for (auto x = 0; x <width_; ++x) {
                    if (flat_data[light_index]->filterData(filter_index)[y*width_+x] == 0) {
                        floatdata.get()[y*width_+x] = 0;
                    } else {
                        floatdata.get()[y*width_+x] = (float)data.get()[y*width_+x] / (float)flat_data[light_index]->filterData(filter_index)[y*width_+x];
                    }
                }
            }
            // subtract bias, divide flat field
            cv::Mat floatdatamat(height_, width_, CV_32F, floatdata.get());
            // Normalize data to a white reference

            std::vector<float> values;

            for (auto y = wtpt_rect_.y(); y < wtpt_rect_.y() + wtpt_rect_.size().height(); ++y) {
                for (auto x = wtpt_rect_.x(); x < wtpt_rect_.x() + wtpt_rect_.size().width(); ++x) {
                    values.push_back(floatdata.get()[y*width_+x]);
                }
            }
            std::nth_element(values.begin(), values.begin()+(values.size()/2), values.end()); // median sort in constant time
            float measured_wtpt = values[values.size()/2];

            for (auto y = 0; y < height_; ++y) {
                for (auto x = 0; x < width_; ++x) {
                    floatdata.get()[y*width_+x] /= (measured_wtpt / absolute_wtpt_values_[filter_index]);
                }
            }

            if (filter_index == 0) { // use first filter as registration target
                for (auto y = 0; y < height_; ++y) {
                    for (auto x = 0; x < width_; ++x) {
                        regdata.get()[y*width_+x] = floatdata.get()[y*width_+x];
                    }
                }
                cv::Mat regdatamat(height_, width_, CV_32F, regdata.get());

            } else {                // Register image to regtarget_data
                // Phase-correlate based registration algorithm to align image planes based on two concentric circle targets
                // Concentric targets are used because of their non-repeating nature; phase correlate gets tripped up by repeating patterns as the peaks can be matched at errant points

                //cv::mat construction is efficient and does not copy data.  Initialize registration target from our regdata


                cv::Mat registerTo(height_, width_, CV_32F, regdata.get());
                cv::Mat reg0_source, reg0_target, reg1_source, reg1_target;
                cv::Mat reg0_sourcef, reg0_targetf, reg1_sourcef, reg1_targetf;

                // These cv::Mats' are our registration targets; selected by the user in the main application.

                //reg0_source = registration target 0 on the image that is going to be registered
                //reg0_target = registration target 1 on the image that is going to be registered to
                reg0_sourcef = floatdatamat(cv::Range(regtargets[0].y(), regtargets[0].y() + regtargets[0].size().height()), cv::Range(regtargets[0].x(), regtargets[0].x() + regtargets[0].size().width()));
                reg0_targetf = registerTo(cv::Range(regtargets[0].y(), regtargets[0].y() + regtargets[0].size().height()), cv::Range(regtargets[0].x(), regtargets[0].x() + regtargets[0].size().width()));// - (reg_size/2), regtargets[0].y + (reg_size/2)), cv::Range(regtargets[0].x - (reg_size/2), regtargets[0].x + (reg_size/2)));
                reg1_sourcef = floatdatamat(cv::Range(regtargets[1].y(), regtargets[1].y() + regtargets[1].size().height()), cv::Range(regtargets[1].x(), regtargets[1].x() + regtargets[1].size().width()));// - (reg_size/2), regtargets[1].y + (reg_size/2)), cv::Range(regtargets[1].x - (reg_size/2), regtargets[1].x + (reg_size/2)));
                reg1_targetf = registerTo(cv::Range(regtargets[1].y(), regtargets[1].y() + regtargets[1].size().height()), cv::Range(regtargets[1].x(), regtargets[1].x() + regtargets[1].size().width()));// - (reg_size/2), regtargets[1].y + (reg_size/2)), cv::Range(regtargets[1].x - (reg_size/2), regtargets[1].x + (reg_size/2)));

                double min, max;
                cv::Point minLoc, maxLoc;

                cv::minMaxLoc(reg0_sourcef, &min, &max, &minLoc, &maxLoc);
                reg0_sourcef *= 255/max;
                cv::minMaxLoc(reg1_sourcef, &min, &max, &minLoc, &maxLoc);
                reg1_sourcef *= 255/max;
                cv::minMaxLoc(reg0_targetf, &min, &max, &minLoc, &maxLoc);
                reg0_targetf *= 255/max;
                cv::minMaxLoc(reg1_targetf, &min, &max, &minLoc, &maxLoc);
                reg1_targetf *= 255/max;

                reg0_sourcef.convertTo(reg0_source, CV_8U);
                reg0_targetf.convertTo(reg0_target, CV_8U);
                reg1_sourcef.convertTo(reg1_source, CV_8U);
                reg1_targetf.convertTo(reg1_target, CV_8U);

                cv::adaptiveThreshold(reg0_source, reg0_source, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 11, 2);
                cv::adaptiveThreshold(reg0_target, reg0_target, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 11, 2);
                cv::adaptiveThreshold(reg1_source, reg1_source, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 11, 2);
                cv::adaptiveThreshold(reg1_target, reg1_target, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 11, 2);

                reg0_source.convertTo(reg0_source, CV_32F);
                reg1_source.convertTo(reg1_source, CV_32F);
                reg0_target.convertTo(reg0_target, CV_32F);
                reg1_target.convertTo(reg1_target, CV_32F);

                reg0_sourcef /= 255;
                reg0_targetf /= 255;
                reg1_sourcef /= 255;
                reg1_targetf /= 255;

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
                float trans_x = (floatdatamat.size().width  / 2) * (1-scale);
                float trans_y = (floatdatamat.size().height / 2) * (1-scale);

                std::vector<float> matrix_data(6);
                matrix_data[0] = scale;
                matrix_data[1] = 0;
                matrix_data[2] = trans_x;
                matrix_data[3] = 0;
                matrix_data[4] = scale;
                matrix_data[5] = trans_y;

                cv::Mat affine(2, 3, CV_32F, matrix_data.data());
                cv::Mat scaled;
                cv::warpAffine(floatdatamat, scaled, affine, floatdatamat.size());

                cv::Mat scaled_target = scaled(cv::Range(regtargets[0].y(), regtargets[0].y() + regtargets[0].size().height()), cv::Range(regtargets[0].x(), regtargets[0].x() + regtargets[0].size().width()));
                cv::Point2d translation_offset = cv::phaseCorrelate(scaled_target, reg0_target);


                matrix_data[2] += translation_offset.x;
                matrix_data[5] += translation_offset.y;

                cv::warpAffine(floatdatamat, floatdatamat, affine, floatdatamat.size());
            }
            if (raw_tiff_path.size() > 0) {
                TIFFSetField(rawdata_tiff, TIFFTAG_IMAGEWIDTH, width_);
                TIFFSetField(rawdata_tiff, TIFFTAG_IMAGELENGTH, height_);
                TIFFSetField(rawdata_tiff, TIFFTAG_BITSPERSAMPLE, sizeof(float) * 8);
                TIFFSetField(rawdata_tiff, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
                TIFFSetField(rawdata_tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
                TIFFSetField(rawdata_tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
                TIFFSetField(rawdata_tiff, TIFFTAG_SAMPLESPERPIXEL, 1);
                TIFFSetField(rawdata_tiff, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
                TIFFSetField(rawdata_tiff, TIFFTAG_WTPTVAL, absolute_wtpt_values_[filter_index]);
                TIFFSetField(rawdata_tiff, TIFFTAG_WTPTMEASURED, measured_wtpt);
                TIFFSetField(rawdata_tiff, TIFFTAG_PAGENUMBER, page_index);

                for (auto row = 0; row < height_; ++row) {
                    TIFFWriteScanline(rawdata_tiff, &floatdata.get()[row*width_], row);
                }
                TIFFWriteDirectory(rawdata_tiff);
                ++page_index;
            }

            int wavelength = filter_->wavelengthAtPos(filter_index);
            std::vector<float> cmf = filter_->cmfValues(wavelength);
            float illuminant = filter_->illuminantValue(wavelength);
            for (auto y = 0; y < height_; ++y) {
                for (auto x = 0; x < width_; ++x) {
                    for (auto xyz_index = 0; xyz_index < 3; ++xyz_index) {
                        xyz_data[light_index].get()->filterData(xyz_index)[y*width_+x] += floatdata.get()[y*width_+x] * cmf[xyz_index] * illuminant;
                    }
                }
            }
        }
    }
    if (raw_tiff_path.size() > 0) {
        TIFFClose(rawdata_tiff);
    }
    // Scale xyz data by scalar constant
    for (auto light = 0; light < nlights_; ++light) {
        for (auto y = 0; y < height_; ++y) {
            for (auto x = 0; x < width_; ++x) {
                for (auto xyz_index = 0; xyz_index < 3; ++xyz_index) {
                     xyz_data[light].get()->filterData(xyz_index)[y*width_+x] /= scalar_constant[xyz_index];
                }
            }
        }
    }


    std::vector<XYZImage*> xyz_ptr;
    for (auto i = 0; i < xyz_data.size(); ++i) {
        xyz_ptr.push_back(xyz_data[i].get());
    }
    std::vector<float> weights(nlights_);
    for (auto weight = 0; weight < weights.size(); ++weight) {
       weights[weight] = 1.0 / float(nlights_);
    }

    master_xyz = std::shared_ptr<XYZImage>(new XYZImage(xyz_ptr, nlights_, weights));
}
void colorengine::setBlckpt(const QRect& blkpt)
{
    bkpt_rect_ = blkpt;
}

QPixmap colorengine::getQPixmap(const cv::Rect& crop)
{
    return RGBImage(*master_xyz.get(), crop).getQPixmap();
}
LabImage* colorengine::getLabImage(const cv::Rect& crop) //cropping, light weights
{
    return new LabImage(*master_xyz.get(), crop);
}

void colorengine::waitForThreadFinish()
{
    if (colorthread_.joinable()) colorthread_.join();
}
void colorengine::setRawDataSavepath(const std::string& path)
{
    raw_tiff_path = path;
}

void colorengine::addDataToQueue(const std::shared_ptr<unsigned short>& data)
{
    data_queue_.push(data);
}
colorengine::colorengine(int width, int height, filterconfig* filter, int nlights) : width_(width), height_(height), filter_(filter), nlights_(nlights)
{
    for (auto light = 0; light < nlights_; ++light) {
        xyz_data.push_back(std::shared_ptr<XYZImage>(new XYZImage(width_, height_)));
    }
    absolute_wtpt_values_ = std::vector<float>(filter_->nfilters());

    for (auto i = 0; i < absolute_wtpt_values_.size(); ++i) {
        absolute_wtpt_values_[i] = 0.9666f;
    }

    cancel_ = false;
}

colorengine::colorengine(int width, int height, filterconfig* filter, int nlights, const std::string& capturename) : width_(width), height_(height), filter_(filter), nlights_(nlights)
{
    for (auto light = 0; light < nlights_; ++light) {
        xyz_data.push_back(std::shared_ptr<XYZImage>(new XYZImage(width_, height_)));
    }
    absolute_wtpt_values_ = std::vector<float>(filter_->nfilters());


    for (auto i = 0; i < absolute_wtpt_values_.size(); ++i) {
        absolute_wtpt_values_[i] = 0.9666f;
    }

    raw_tiff_path = capturename;

    cancel_ = false;
}
void colorengine::stopAsync()
{
    cancel_ = true;
    // Push back one additional data object onto queue to unblock worker thread
    data_queue_.push(std::shared_ptr<unsigned short>());
    // Wait for thread to return, then clear the queue
    if (colorthread_.joinable()) colorthread_.join();
    data_queue_.clear();
}

void colorengine::addBias(const std::shared_ptr<unsigned short>& bias)
{
    bias_data = bias;
}
void colorengine::addFlatField(const std::shared_ptr<FlatFieldImage>& flatimg)
{
    flat_data.push_back(flatimg);
}
void colorengine::setRegtargets(const std::vector<QRect>& targets)
{
    regtargets = targets;
}
void colorengine::setWtpt(const QRect& wtpt)
{
    wtpt_rect_ = wtpt;
}
void colorengine::startAsync()
{
    if (colorthread_.joinable()) colorthread_.join();
    colorthread_ = std::thread(&colorengine::threadFunc, this);
}
void colorengine::setLightWeights(const std::vector<float> &weights)
{
    weights_ = weights;

    std::vector<XYZImage*> xyz_ptr;
    for (auto i = 0; i < xyz_data.size(); ++i) {
        xyz_ptr.push_back(xyz_data[i].get());
    }

    master_xyz = std::shared_ptr<XYZImage>(new XYZImage(xyz_ptr, nlights_, weights));
}
void colorengine::setLightWeights(const std::vector<float>& weights, cv::Size master_dest_size)
{
    weights_ = weights;

    std::vector<XYZImage*> xyz_ptr;
    for (auto i = 0; i < xyz_data.size(); ++i) {
        xyz_ptr.push_back(xyz_data[i].get());
    }

    master_xyz = std::shared_ptr<XYZImage>(new XYZImage(xyz_ptr, nlights_, weights, master_dest_size));
}

colorengine::~colorengine()
{
    if(colorthread_.joinable()) colorthread_.join();
}
