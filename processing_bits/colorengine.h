#ifndef COLORENGINE_H
#define COLORENGINE_H

#include "filterconfig.h"
#include <thread>
#include <memory>
#include "ColorProcessor/Image.h"
#include "ColorProcessor/ConversionFunctions.h"
#include "ColorProcessor/FlatFieldImage.h"
#include <QRect>
#include <QPixmap>
#include "threadqueue.h"

// Opencv for image division/registration operations
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>


#include "ColorProcessor/libtiff/tiffio.h"
#include "ColorProcessor/customtifftags.h"

// colorengine: worker class that supports asynchronus color image calculation using a thread-safe FIFO queue (dataqueue)
// This enables color data to be processed parallel with image acquisition.
//
class colorengine
{
private:
    std::vector<std::shared_ptr<XYZImage>> xyz_data;
    std::shared_ptr<XYZImage> master_xyz;

    std::shared_ptr<unsigned short> bias_data;
    std::vector<std::shared_ptr<FlatFieldImage>> flat_data;
    std::shared_ptr<unsigned short> regtarget_data;

    int width_, height_;
    filterconfig* filter_;
    std::vector<float> absolute_wtpt_values_;
    TIFF* rawdata_tiff;
    std::string raw_tiff_path;

    threadqueue< std::shared_ptr<unsigned short> > data_queue_;
    std::thread colorthread_;

    std::vector<QRect> regtargets;
    std::vector<float> weights_;
    QRect wtpt_rect_;
    QRect bkpt_rect_;
    int nlights_;
    bool cancel_;
    void threadFunc();
public:
    colorengine(int width, int height, filterconfig* filter, int nlights, const std::string& capturename);
    colorengine(int width, int height, filterconfig* filter, int nlights);
    colorengine() {};
    ~colorengine();

    void addBias(const std::shared_ptr<unsigned short>& bias);
    void addFlatField(const std::shared_ptr<FlatFieldImage>& flatimg);
    void setWtpt(const QRect& wtpt);
    void setBlckpt(const QRect& blkpt);
    void setRegtargets(const std::vector<QRect>& targets);
    void setRawDataSavepath(const std::string& path);
    // set light weights
    void setLightWeights(const std::vector<float>& weights);
    void setLightWeights(const std::vector<float>& weights, cv::Size master_dest_size);

    void addDataToQueue(const std::shared_ptr<unsigned short>& data);
    //void addDataPlane(const dataplane<unsigned short>& data);
    void startAsync();
    void stopAsync();
    void waitForThreadFinish();

    QPixmap getQPixmap(const cv::Rect& crop);
    LabImage* getLabImage(const cv::Rect& crop);
};
#endif // COLORENGINE_H
