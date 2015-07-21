#ifndef CALIBRATEDIMAGE_H
#define CALIBRATEDIMAGE_H

#include "Image.h"
#include "FlatFieldImage.h"

class CalibratedImage : public RawImage<float>
{
public:
    CalibratedImage(const RawImage<unsigned short>& raw_img, const FlatFieldImage& flat_img);
};


#endif // CALIBRATEDIMAGE_H
