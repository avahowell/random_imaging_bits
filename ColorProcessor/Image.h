#pragma once

#include <functional>
#include <vector>

template<typename img_type, typename fn_type = std::function<void(img_type*)>> class Image;
template<typename img_type> class RawImage;

template<typename img_type, typename fn_type>
class Image
{
    friend class RawImage<img_type>;
public:
    // Constructors:
    Image(size_t width, size_t height, bool allocate_mem = true);
    Image(size_t width, size_t height, const fn_type& read_function, const fn_type& write_function, bool allocate_mem = true);

    // Rule of 3 (Copy Constructor, Copy Assignment Operator, and Destructor):
    Image(const Image<img_type, fn_type>& img); // Copy constructor.mm
    Image<img_type, fn_type>& operator=(const Image<img_type, fn_type>& img); // Assignment operator.
    virtual ~Image(); // Destructor. NOTE: If you plan on using this class polymorphically, make this function virtual!

    void ReadData();
    void WriteData();

    void SetWriteFunction(const fn_type& write_function) { write_function_ = write_function; }
    void SetReadFunction(const fn_type& read_function) { read_function_ = read_function; }

    size_t width() const { return width_; }
    size_t height() const { return height_; }

protected:
    img_type* img_data_;
    size_t width_;
    size_t height_;
    fn_type read_function_;
    fn_type write_function_;

    virtual void AllocateImgData();

private:
    // Helper functions to prevent some code duplication.
    void CopyImageDataFrom(const Image<img_type, fn_type>& img);
};

template<typename img_type>
class RawImage : public Image<img_type*, std::function<void(img_type**, size_t, char*)>>
{
    friend class NormalizedImage;
    friend class CalibratedImage;
    friend class FlatFieldImage;
public:
    // Constructors:
    RawImage(size_t num, size_t width, size_t height, bool allocate_mem = true, img_type** img_data = NULL);
    RawImage(size_t num, size_t width, size_t height, const std::function<void(img_type**, size_t, char*)>& read_function, const std::function<void(img_type**, size_t, char*)>& write_function, bool allocate_mem = false, img_type** img_data = NULL);

    // Rule of 3 (Copy Constructor, Copy Assignment Operator, and Destructor):
    RawImage(const RawImage<img_type>& img); // Copy constructor.
    RawImage<img_type>& operator=(const RawImage<img_type>& img); // Assignment operator.
    img_type* filterData(int filter);
    virtual ~RawImage(); // Destructor. (NOTE: If you plan on using this class polymorphically, make this function virtual! ???)

    void ReadData(size_t n, char* path = NULL); // See the NOTE for ReadData. // Change this so that path is passed in through the lambda function instead.
    void WriteData(char* path);
    void WriteData(char* path, size_t n);
    size_t num() const { return num_; }
    RawImage<img_type>& operator-=(const Image<img_type>& amb_img);
    RawImage<img_type>& operator-=(const std::vector<unsigned short>& sub_img);
    RawImage<img_type>& operator/(FlatFieldImage& flat_img);
                                                       //    RawImage<float> operator/(const RawImage<img_type>& img);
 //   RawImage<float>& operator/(const RawImage<float>& flat_img);


protected:
    const size_t num_;

    typedef std::function<void(img_type**, size_t, char*)> fn_type;

    // Helper functions to prevent some code duplication.
    virtual void AllocateImgData();
    void CopyImageDataFrom(const RawImage<img_type>& img);
};

template<typename img_type>
img_type* RawImage<img_type>::filterData(int filter)
{
    return img_data_[filter];
}

// Image constructor. NOTE: Only use this constructor when allocating an Image object (i.e. do NOT use it when allocating a RawImage object).
template<typename img_type, typename fn_type>
Image<img_type, fn_type>::Image(size_t width, size_t height, bool allocate_mem) : width_(width), height_(height)
{
    if (allocate_mem) {
        AllocateImgData();
    }
    read_function_ = [](img_type*)->void {};
    write_function_ = [](img_type*)->void {};
}

// Image constructor.
template<typename img_type, typename fn_type>
Image<img_type, fn_type>::Image(size_t width, size_t height, const fn_type& read_function, const fn_type& write_function, bool allocate_mem) : width_(width), height_(height), read_function_(read_function), write_function_(write_function)
{
    if (allocate_mem) {
        AllocateImgData();
    }
}

// Image copy constructor.
template<typename img_type, typename fn_type>
Image<img_type, fn_type>::Image(const Image<img_type, fn_type>& img) : width_(img.width_), height_(img.height_), read_function_(img.read_function_), write_function_(img.write_function_)
{
    AllocateImgData();
    CopyImageDataFrom(img);
}

// Image assignment operator.
template<typename img_type, typename fn_type>
Image<img_type, fn_type>& Image<img_type, fn_type>::operator=(const Image<img_type, fn_type>& img)
{
    if (this != &img) {
        this->~Image();
        this->width_ = img.width_;
        this->height_ = img.height_;
        this->read_function_ = img.read_function_;
        this->write_function_ = img.write_function_;

        CopyImageDataFrom(img); // Copy image data.
    }
    return *this;
}

// Image destructor.
template<typename img_type, typename fn_type>
Image<img_type, fn_type>::~Image()
{
    delete [] img_data_;
}

// Image read data function.
template<typename img_type, typename fn_type>
void Image<img_type, fn_type>::ReadData()
{
    if (read_function_) {
        (read_function_)(img_data_);
    }
}

// Image write data function.
template<typename img_type, typename fn_type>
void Image<img_type, fn_type>::WriteData()
{
    if (write_function_) {
        (write_function_)(img_data_);
    }
}

// Image helper function to allocate image data array.
template<typename img_type, typename fn_type>
void Image<img_type, fn_type>::AllocateImgData()
{
    img_data_ = new img_type[width_ * height_];
}

// Image helper function to copy image data array.
template<typename img_type, typename fn_type>
void Image<img_type, fn_type>::CopyImageDataFrom(const Image<img_type, fn_type>& img)
{
    const size_t img_size = this->width_ * this->height_;
    for (size_t i = 0; i < img_size; ++i) {
        this->img_data_[i] = img.img_data_[i];
    }
}

// RawImage constructor.
template<typename img_type>
RawImage<img_type>::RawImage(size_t num, size_t width, size_t height, bool allocate_mem, img_type** img_data) : num_(num), Image<img_type*, std::function<void(img_type**, size_t, char*)>>(width, height, [](img_type** img_data, size_t n, char* path)->void {}, [](img_type** img_data, size_t n, char* path)->void {}, false)
{
    if (allocate_mem) {
        AllocateImgData();
    } else if (img_data) {
        img_data_ = img_data;
    }
    read_function_ = [](img_type** img_data, size_t n, char* path)->void {};
    write_function_ = [](img_type** img_data, size_t n, char* path)->void {};
}

// RawImage constructor.
template<typename img_type>
RawImage<img_type>::RawImage(size_t num, size_t width, size_t height, const std::function<void(img_type**, size_t, char*)>& read_function, const std::function<void(img_type**, size_t, char*)>& write_function, bool allocate_mem, img_type** img_data) : num_(num), Image<img_type*, std::function<void(img_type**, size_t, char*)>>(width, height, read_function, write_function, false)
{
    if (allocate_mem) {
        AllocateImgData();
    } else if (img_data) {
        img_data_ = img_data;
    }
}

// RawImage copy constructor.
template<typename img_type>
RawImage<img_type>::RawImage(const RawImage<img_type>& img) : num_(img.num_), Image<img_type*, std::function<void(img_type**, size_t, char*)>>(img.width_, img.height_, [](img_type** img_data, size_t n, char* path)->void {}, [](img_type** img_data, size_t n, char* path)->void {}, false)
{
    AllocateImgData();
    CopyImageDataFrom(img);
}

// RawImage assignment operator.
template<typename img_type>
RawImage<img_type>& RawImage<img_type>::operator=(const RawImage<img_type>& img)
{
    if (this != &img) {
        this->~RawImage();
        this->width_ = img.width_;
        this->height_ = img.height_;
        this->read_function_ = img.read_function_;
        this->write_function_ = img.write_function_;

        CopyImageDataFrom(img); // Copy image data.
    }
    return *this;
}
                           #include <iostream>
// RawImage destructor.
template<typename img_type>
RawImage<img_type>::~RawImage()
{
    for (size_t n = 0; n < num_; ++n) {
        delete [] img_data_[n];
    }
    // NOTE: delete [] img_data_ is called by the Image destructor, and is therefore not needed here.
}

// RawImage read data function.
// NOTE: This function should be called for every image that is read in.
// read_function_ should be assigned accordingly.
template<typename img_type>
void RawImage<img_type>::ReadData(size_t n, char* path)
{
    if (read_function_) {
        (read_function_)(img_data_, n, path);
    }
}

// RawImage write data function.
template<typename img_type>
void RawImage<img_type>::WriteData(char* path)
{
    if (write_function_) {
        (write_function_)(img_data_, num_, path);
    }
}
template<typename img_type>
void RawImage<img_type>::WriteData(char* path, size_t n)
{
    if (write_function_) {
        (write_function_)(img_data_, n, path);
    }
}

// RawImage overloaded minus equals operator (-=)
template<typename img_type>
RawImage<img_type>& RawImage<img_type>::operator-=(const Image<img_type>& amb_img)
{
    const size_t img_size = this->width_ * this->height_;
    for (size_t n = 0; n < this->num_; ++n) {
        for (size_t i = 0; i < img_size; ++i) { // use min() to use the smallest image instead
            if (this->img_data_[n][i] > amb_img.img_data_[i]) {
                this->img_data_[n][i] -= amb_img.img_data_[i];
            } else {
                this->img_data_[n][i] = 0;
            }
        }
    }
    return *this;
}
template<typename img_type>
RawImage<img_type>& RawImage<img_type>::operator-=(const std::vector<unsigned short>& sub_img)
{
    const size_t img_size = this->width_ * this->height_;
    for (size_t n = 0; n < this->num_; ++n) {
        for (size_t i = 0; i < sub_img.size(); ++i) { // use min() to use the smallest image instead
            if (this->img_data_[n][i] > sub_img[i]) {
                this->img_data_[n][i] -= (float)sub_img[i];
            } else {
                this->img_data_[n][i] = 0;
            }
        }
    }
    return *this;
}
template<typename img_type>
RawImage<img_type>& RawImage<img_type>::operator/(FlatFieldImage& flat_img)
{
    const size_t img_size = this->width_ * this->height_;
    for (size_t n = 0; n < this->num_; ++n) {
        for (size_t i = 0; i < img_size; ++i) {
            if (flat_img.filterData(n)[i] == 0) {
                this->img_data_[n][i] = 0;
            } else {
                this->img_data_[n][i] /= flat_img.filterData(n)[i];
            }
        }
    }
    return *this;
}



// RawImage helper functon to allocate image data array.
template<typename img_type>
void RawImage<img_type>::AllocateImgData()
{
    img_data_ = new img_type*[num_];
    const size_t img_size = width_ * height_;
    for (size_t n = 0; n < num_; ++n) {
        img_data_[n] = new img_type[img_size];
    }
}

// RawImage helper function to copy image data array.
template<typename img_type>
void RawImage<img_type>::CopyImageDataFrom(const RawImage<img_type>& img)
{
    const size_t img_size = this->width_ * this->height_;
    for (size_t n = 0; n < this->num_; ++n) {
        for (size_t i = 0; i < img_size; ++i) {
            this->img_data_[n][i] = img.img_data_[n][i];
        }
    }
}
