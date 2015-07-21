#include <fstream>
#include <sstream>

#include "Image.h"
#include "ConversionFunctions.h"
#include <iostream>
#include <memory>

// XYZImage constructor.
XYZImage::XYZImage(const NormalizedImage& input_img, filterconfig* filter) : RawImage<float>(3, input_img.width_, input_img.height_, NULL), filter_(filter)
{
    AllocateImgData();

    std::map<int, std::vector<float>>::iterator it;

    float scalar_constant[3] = {0, 0, 0};
    for (auto filter_index = 0; filter_index < filter_->nfilters(); ++filter_index) {
        int wavelength = filter_->wavelengthAtPos(filter_index);
        std::vector<float> cmf_v = filter_->cmfValues(wavelength);
        float* cmf = &cmf_v[0];
        float illuminant = filter_->illuminantValue(wavelength);
        //calculate the XYZ values without whitepoint scaling
        for (size_t y = 0; y < height_; ++y) {
            for (size_t x = 0; x < width_; ++x) {
                for (size_t xyz_index = 0; xyz_index < 3; ++xyz_index) {
                    img_data_[xyz_index][(y * width_) + x] += input_img.img_data_[filter_index][(y * input_img.width_)+x] * cmf[xyz_index] * illuminant;
                }
            }
        }
        // calculate the white point for the specified illuminant
        for (size_t xyz_index = 0; xyz_index < 3; ++xyz_index) {
            scalar_constant[xyz_index] += cmf[xyz_index] * illuminant;
        }
        ++filter_index;
    }
    for (size_t y = 0; y < height_; ++y) {
        for (size_t x = 0; x < width_; ++x) {
            for (size_t xyz_index = 0; xyz_index < 3; ++xyz_index) {
                img_data_[xyz_index][(y * width_) + x] /= scalar_constant[xyz_index];
            }
        }
    }
}
XYZImage::XYZImage(const int width, const int height) : RawImage<float>(3, width, height, false, NULL)
{
    img_data_ = new float*[num_];
    const size_t img_size = width_ * height_;
    for (size_t n = 0; n < num_; ++n) {
        img_data_[n] = new float[img_size]();
    }
}

XYZImage::XYZImage(const NormalizedImage& input_img, const char* const illuminant_path, const char* const cmf_path) : RawImage<float>(3, input_img.width_, input_img.height_, NULL)
{
	AllocateImgData();
	std::ifstream istr;

	// read in illuminant & cmf values from file path
	// NOTE: This is a naive way of reading in the values.
  // Use the provided filterconfig class instead.
  //
	float* illuminant = new float[input_img.num_];
	istr.open(illuminant_path);
	for (size_t n = 0; n < input_img.num_; ++n) {
		istr >> illuminant[n];
	}
	istr.close();

	float** cmf_values = new float*[3];
	for (size_t xyz_index = 0; xyz_index < 3; ++xyz_index) {
		cmf_values[xyz_index] = new float[input_img.num_];
	}
	istr.open(cmf_path);
	std::string token;
	for (size_t xyz_index = 0; xyz_index < 3; ++xyz_index) {
		istr >> token; // token should be X:, then Y:, then Z:
		for (size_t n = 0; n < input_img.num_; ++n) {
			istr >> cmf_values[xyz_index][n];
		}
	}
	istr.close();

	float scalar_constant[3] = {0, 0, 0};
	for (size_t filter_index = 0; filter_index < input_img.num_; ++filter_index) {
		//calculate the XYZ values without whitepoint scaling
		for (size_t y = 0; y < height_; ++y) {
			for (size_t x = 0; x < width_; ++x) {
				for (size_t xyz_index = 0; xyz_index < 3; ++xyz_index) {
					img_data_[xyz_index][(y * width_) + x] += input_img.img_data_[filter_index][(y * input_img.width_)+x] * cmf_values[xyz_index][filter_index] * illuminant[filter_index];
				}
			}
		}
		// calculate the white point for the specified illuminant
		for (size_t xyz_index = 0; xyz_index < 3; ++xyz_index) {
			scalar_constant[xyz_index] += cmf_values[xyz_index][filter_index] * illuminant[filter_index];
		}
	}
	for (size_t y = 0; y < height_; ++y) {
		for (size_t x = 0; x < width_; ++x) {
			for (size_t xyz_index = 0; xyz_index < 3; ++xyz_index) {
				img_data_[xyz_index][(y * width_) + x] /= scalar_constant[xyz_index];
			}
		}
	}
}
// XYZImage weighted average constructor
XYZImage::XYZImage(std::vector<XYZImage*> images, size_t n_lights, float* weights) : RawImage<float>(3, images[0]->width_, images[0]->height_, NULL)
{
	AllocateImgData();
	// TODO: CHECK THAT WIDTH AND HEIGHT ARE THE SAME ACROSS XYZIMAGES

	bool weights_is_null = false;
	if (weights == NULL) {
		weights_is_null = true;
	
		weights = new float[n_lights];
		for (size_t light_index = 0; light_index < n_lights; light_index++) {
			weights[light_index] = 1.0 / n_lights;
		}
	}

	for (size_t xyz_index = 0; xyz_index < 3; ++xyz_index) {
		for (size_t y = 0; y < height_; ++y) {
			for (size_t x = 0; x < width_; ++x) {
				for (size_t light_index = 0; light_index < n_lights; ++light_index) {
                    img_data_[xyz_index][(y * width_) + x] += images[light_index]->img_data_[xyz_index][(y * width_) + x] * weights[light_index];
				}
			}
		}
	}
	if (weights_is_null) {
		delete [] weights;
	}
}
XYZImage::XYZImage(std::vector<XYZImage *> images, size_t n_lights, std::vector<float> weights) : RawImage<float>(3, images[0]->width_, images[0]->height_, NULL)
{
    AllocateImgData();
    // TODO: CHECK THAT WIDTH AND HEIGHT ARE THE SAME ACROSS XYZIMAGES

    for (size_t xyz_index = 0; xyz_index < 3; ++xyz_index) {
        for (size_t y = 0; y < height_; ++y) {
            for (size_t x = 0; x < width_; ++x) {
                for (size_t light_index = 0; light_index < n_lights; ++light_index) {
                    img_data_[xyz_index][(y * width_) + x] += images[light_index]->img_data_[xyz_index][(y * width_) + x] * weights[light_index];
                }
            }
        }
    }
}
XYZImage::XYZImage(std::vector<XYZImage *> images, size_t n_lights, std::vector<float> weights, const cv::Size& dest_size) : RawImage<float>(3, 0, 0, NULL)
{
    float scale = (float)dest_size.width / (float)images[0]->width();

    cv::Size render_size(images[0]->width()*scale, images[0]->height()*scale);

    width_ = render_size.width;
    height_ = render_size.height;

    AllocateImgData();
    // TODO: CHECK THAT WIDTH AND HEIGHT ARE THE SAME ACROSS XYZIMAGES

    // Resize source data to supplied cv::size and weight the data

    for (size_t xyz_index = 0; xyz_index < 3; ++xyz_index) {
        for (size_t light_index = 0; light_index < images.size(); ++light_index) {
            std::unique_ptr<float> scaled_data(new float[render_size.width*render_size.height]);
            cv::Mat dst(height_, width_, CV_32F, scaled_data.get());
            cv::Mat src(images[light_index]->height(), images[light_index]->width(), CV_32F, images[light_index]->filterData(xyz_index));
            cv::resize(src, dst, render_size);

            for (auto y = 0; y < height_; ++y) {
                for (auto x = 0; x < width_; ++x) {
                    img_data_[xyz_index][(y * width_) + x] += scaled_data.get()[(y*width_)+x] * weights[light_index];
                }
            }
        }
    }
}

XYZImage::XYZImage(std::vector<XYZImage> images, size_t n_lights, std::vector<float> weights) : RawImage<float>(3, images[0].width_, images[0].height_, NULL)
{
    AllocateImgData();
    // TODO: CHECK THAT WIDTH AND HEIGHT ARE THE SAME ACROSS XYZIMAGES

    for (size_t xyz_index = 0; xyz_index < 3; ++xyz_index) {
        for (size_t y = 0; y < height_; ++y) {
            for (size_t x = 0; x < width_; ++x) {
                for (size_t light_index = 0; light_index < n_lights; ++light_index) {
                    img_data_[xyz_index][(y * width_) + x] += images[light_index].img_data_[xyz_index][(y * width_) + x] * weights[light_index];
                }
            }
        }
    }
}

// XYZImage copy constructor.
XYZImage::XYZImage(const XYZImage& img) : RawImage<float>(img)
{ }

// XYZImage assignment operator.
XYZImage& XYZImage::operator=(const XYZImage& img)
{
	if (this != &img) {
		RawImage::operator=(img);
	}
	return *this;
}

// XYZImage destructor.
// NOTE: the base class's (RawImage) destructor is automatically called,
// so there is no need to call it again in this destructor.
XYZImage::~XYZImage()
{ }

// XYZImage helper functon to allocate image data array, and set the initial values to 0.
void XYZImage::AllocateImgData()
{
	img_data_ = new float*[num_];
	const size_t img_size = width_ * height_;
	for (size_t n = 0; n < num_; ++n) {
		img_data_[n] = new float[img_size]();
		// Note the extra "()"!           ^^ This initializes their values to their default value (0).
		// This is (I think) more efficient than looping through the array and manually setting them equal to 0.
	}
}

// LabImage constructor.
LabImage::LabImage(const XYZImage& input_img) : RawImage<float>(3, input_img.width_, input_img.height_)
{
	float f_xyz[3];
	for (size_t y = 0; y < input_img.height_; ++y) {
		for (size_t x = 0; x < input_img.width_; ++x) {
			for (size_t xyz_index = 0; xyz_index < 3; ++xyz_index) {
				//calculate the unscaled L*ab values
				if (input_img.img_data_[xyz_index][(y * input_img.width_) + x] > 216.0/24389.0) {
					f_xyz[xyz_index] = pow(input_img.img_data_[xyz_index][(y * input_img.width_) + x], (1.0/3.0));
				} else {
					f_xyz[xyz_index] = ((input_img.img_data_[xyz_index][(y * input_img.width_) + x] * (24389.0 / 27.0)) + 16) / 116.0;
				}
			}
			
			img_data_[0][(y * width_) + x] = ((116.0 * f_xyz[1]) - 16.0);
			img_data_[1][(y * width_) + x] = (500.0 * (f_xyz[0] - f_xyz[1]));
			img_data_[2][(y * width_) + x] = (200.0 * (f_xyz[1] - f_xyz[2]));
		}
	}
}

LabImage::LabImage(const XYZImage &input_img, const cv::Rect& crop) : RawImage<float>(3, crop.width, crop.height)
{
    float f_xyz[3];
    int img_x = 0;
    int img_y = 0;
    for (auto y = crop.y; y < crop.height + crop.y; ++y) {
        for (auto x = crop.x; x < crop.width + crop.x; ++x) {
            for (size_t xyz_index = 0; xyz_index < 3; ++xyz_index) {
                //calculate the unscaled L*ab values
                if (input_img.img_data_[xyz_index][(y * input_img.width_) + x] > 216.0/24389.0) {
                    f_xyz[xyz_index] = pow(input_img.img_data_[xyz_index][(y * input_img.width_) + x], (1.0/3.0));
                } else {
                    f_xyz[xyz_index] = ((input_img.img_data_[xyz_index][(y * input_img.width_) + x] * (24389.0 / 27.0)) + 16) / 116.0;
                }
            }

            img_data_[0][(img_y * width_) + img_x] = ((116.0 * f_xyz[1]) - 16.0);
            img_data_[1][(img_y * width_) + img_x] = (500.0 * (f_xyz[0] - f_xyz[1]));
            img_data_[2][(img_y * width_) + img_x] = (200.0 * (f_xyz[1] - f_xyz[2]));


            //img_data_[2][(img_y * width_) + img_x] += abs(img_data_[2][(img_y* width_) +img_x]) * 0.05;
            ++img_x;
        }
        ++img_y;
        img_x = 0;
    }
}

// LabImage copy constructor.
LabImage::LabImage(const LabImage& img) : RawImage<float>(img)
{ }

// LabImage assignment operator.
LabImage& LabImage::operator=(const LabImage& img)
{
	if (this != &img) {
		RawImage::operator=(img);
	}
	return *this;
}

// LabImage destructor.
// NOTE: the base class's (RawImage) destructor is automatically called,
// so there is no need to call it again in this destructor.
LabImage::~LabImage()
{ }

RGBImage::RGBImage(const XYZImage& InputImage) : RawImage<uint8_t>(3, InputImage.width_, InputImage.height_)
{
	float xyz_to_rgb_m[3][3];

	//this is sRGB (D50 bradford-adapted) conversion matrix
	xyz_to_rgb_m[0][0] = 3.1338561f		;
	xyz_to_rgb_m[1][0] = -1.6168667f	;
	xyz_to_rgb_m[2][0] = -0.4906146f	;
	xyz_to_rgb_m[0][1] = -0.9787684f	;
	xyz_to_rgb_m[1][1] = 1.9161415f		;
	xyz_to_rgb_m[2][1] = 0.0334540f		;
	xyz_to_rgb_m[0][2] = 0.0719453f		;
	xyz_to_rgb_m[1][2] = -0.2289914f	;
	xyz_to_rgb_m[2][2] =  1.4052427f	;

	int img_x = 0;

	for (int y = 0; y < height_; y++) {
		for (int x = 0; x < width_; x++) {
			float rf, gf, bf;
            // The values 0.96422 and 0.82521 are hardcoded illuminant values, in this case for D50. 96422 82521
            rf = ( (xyz_to_rgb_m[0][0] * InputImage.img_data_[XYZImage::XINDEX][(y*width_)+x] * 0.96422) + (xyz_to_rgb_m[1][0] * InputImage.img_data_[XYZImage::YINDEX][(y*width_)+x]) + (xyz_to_rgb_m[2][0] * InputImage.img_data_[XYZImage::ZINDEX][(y*width_)+x] * 0.82521));
            gf = ( (xyz_to_rgb_m[0][1] * InputImage.img_data_[XYZImage::XINDEX][(y*width_)+x] * 0.96422) + (xyz_to_rgb_m[1][1] * InputImage.img_data_[XYZImage::YINDEX][(y*width_)+x]) + (xyz_to_rgb_m[2][1] * InputImage.img_data_[XYZImage::ZINDEX][(y*width_)+x] * 0.82521));
            bf = ( (xyz_to_rgb_m[0][2] * InputImage.img_data_[XYZImage::XINDEX][(y*width_)+x] * 0.96422) + (xyz_to_rgb_m[1][2] * InputImage.img_data_[XYZImage::YINDEX][(y*width_)+x]) + (xyz_to_rgb_m[2][2] * InputImage.img_data_[XYZImage::ZINDEX][(y*width_)+x] * 0.82521));

			// Gamma scaling
			rf = pow(rf, (1.0/1.8));
			gf = pow(gf, (1.0/1.8));
			bf = pow(bf, (1.0/1.8));

			// Scale from 0-1 to 0-255 (8-bit)
			rf *= 255;
			gf *= 255;
			bf *= 255;

			// Clipping
			if (rf > 255)
				rf = 255;
			if (bf > 255)
				bf = 255;
            if (gf > 255)
				gf = 255;

            uint8_t r, g, b;
			r = floor(rf + 0.5);
			g = floor(gf + 0.5);
			b = floor(bf + 0.5);

			img_data_[0][(y * width_) + x] = r;
			img_data_[1][(y * width_) + x] = g;
			img_data_[2][(y * width_) + x] = b;
			img_x++;
        }
        img_x = 0;
	}
}
RGBImage::RGBImage(const XYZImage& InputImage, const cv::Rect& crop) : RawImage<uint8_t>(3, crop.width, crop.height)
{
    float xyz_to_rgb_m[3][3];

    //this is sRGB (D50 bradford-adapted)
    xyz_to_rgb_m[0][0] = 3.1338561f		;
    xyz_to_rgb_m[1][0] = -1.6168667f	;
    xyz_to_rgb_m[2][0] = -0.4906146f	;
    xyz_to_rgb_m[0][1] = -0.9787684f	;
    xyz_to_rgb_m[1][1] = 1.9161415f		;
    xyz_to_rgb_m[2][1] = 0.0334540f		;
    xyz_to_rgb_m[0][2] = 0.0719453f		;
    xyz_to_rgb_m[1][2] = -0.2289914f	;
    xyz_to_rgb_m[2][2] =  1.4052427f	;

    int img_x = 0;
    int img_y = 0;

    for (int y = crop.y; y < crop.height + crop.y; y++) {
        for (int x = crop.x; x < crop.width + crop.x; x++) {
            float rf, gf, bf;

            // The values 0.96422 and 0.82521 are hardcoded illuminant values, in this case for D50. 96422 82521
            rf = ( (xyz_to_rgb_m[0][0] * InputImage.img_data_[XYZImage::XINDEX][(y*InputImage.width_)+x] * 0.96422) + (xyz_to_rgb_m[1][0] * InputImage.img_data_[XYZImage::YINDEX][(y*InputImage.width_)+x]) + (xyz_to_rgb_m[2][0] * InputImage.img_data_[XYZImage::ZINDEX][(y*InputImage.width_)+x] * 0.82521));
            gf = ( (xyz_to_rgb_m[0][1] * InputImage.img_data_[XYZImage::XINDEX][(y*InputImage.width_)+x] * 0.96422) + (xyz_to_rgb_m[1][1] * InputImage.img_data_[XYZImage::YINDEX][(y*InputImage.width_)+x]) + (xyz_to_rgb_m[2][1] * InputImage.img_data_[XYZImage::ZINDEX][(y*InputImage.width_)+x] * 0.82521));
            bf = ( (xyz_to_rgb_m[0][2] * InputImage.img_data_[XYZImage::XINDEX][(y*InputImage.width_)+x] * 0.96422) + (xyz_to_rgb_m[1][2] * InputImage.img_data_[XYZImage::YINDEX][(y*InputImage.width_)+x]) + (xyz_to_rgb_m[2][2] * InputImage.img_data_[XYZImage::ZINDEX][(y*InputImage.width_)+x] * 0.82521));

            // Gamma scaling
            rf = pow(rf, (1.0/1.8));
            gf = pow(gf, (1.0/1.8));
            bf = pow(bf, (1.0/1.8));

            // Scale from 0-1 to 0-255 (8-bit)
            rf *= 255;
            gf *= 255;
            bf *= 255;

            // Clipping
            if (rf > 255)
                rf = 255;
            if (bf > 255)
                bf = 255;
            if (gf > 255)
                gf = 255;

            uint8_t r, g, b;
            r = floor(rf + 0.5);
            g = floor(gf + 0.5);
            b = floor(bf + 0.5);

            img_data_[0][(img_y * width_) + img_x] = r;
            img_data_[1][(img_y * width_) + img_x] = g;
            img_data_[2][(img_y * width_) + img_x] = b;

            ++img_x;
        }
        ++img_y;
        img_x = 0;
    }
}

QPixmap RGBImage::getQPixmap()
{
    size_t scanline_size = width_ * 3;

    while (scanline_size % 4) ++scanline_size;
    std::shared_ptr<uint8_t> rgbdata(new uint8_t[scanline_size * height_]);
    int img_x = 0;
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < scanline_size; x+=3) {
            for (int rgb_index = 0; rgb_index < 3; ++rgb_index) {
                rgbdata.get()[(y * scanline_size) + x + rgb_index] = img_data_[rgb_index][(y*width_)+img_x];
            }
            ++img_x;
        }
        img_x = 0;
    }
    QImage img(rgbdata.get(), width_, height_, QImage::Format_RGB888);
    QPixmap answer = QPixmap::fromImage(img);
    return answer;
}
