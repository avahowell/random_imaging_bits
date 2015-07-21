#ifndef FILTERCONFIG_H
#define FILTERCONFIG_H
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <iostream>
#include <sstream>

/*
 *
 * This class will encapsulate information necessary to construct a color image from n filters
 * this includes: bandpass information complete with color matching functions,
 *                hardware filterwheel index (where on the physical device a given filter is stored)
 *                focus offset values
 *
 * This class will also have disk read/write functionality.
 *
 * TODO
 * copy constructor, assignment operator
 * Whitepoint storage? No - seperate class for this
 * */
class filterconfig
{
private:
    std::vector<int> wavelengths_;
    std::vector<int> filterpositions_;
    std::vector<long> focuspositions_;
    std::vector<float> xyz_scalarconstant_;
    std::map<int, std::vector<float>> wavelength_cmf_;
    std::map<int, float> illuminant_;
    int bandpass_width_;

public:
    static const int filterconfig_43014;
    static const int filterconfig_43014_averaged;
    static const int filterconfig_51414;
    filterconfig(const std::string& cmfcsv_path, const std::string& illcsv_path, const int& configindex);
   // filterconfig() {}
    filterconfig::~filterconfig() { std::cout << " Filter destroyed! " << std::endl; }


    const int wavelengthAtPos(int pos) const
    {
        return wavelengths_[pos];
    }
    const int hardwarePositions(int pos) const
    {
        return filterpositions_[pos];
    }
    const int focusPosition(int pos) const
    {
        return focuspositions_[pos];
    }
    const std::vector<float>& cmfValues(int wavelength) const
    {
        return wavelength_cmf_.at(wavelength);
    }
    const float illuminantValue(int wavelength) const
    {
        return illuminant_.at(wavelength);
    }
    const float scalarConstant(int color) const
    {
        return xyz_scalarconstant_[color];
    }
    const int nfilters() const
    {
        return filterpositions_.size();
    }
    const int bandpassWidth() const
    {
        return bandpass_width_;
    }
};

#endif
