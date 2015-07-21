#include "filterconfig.h"
const int filterconfig::filterconfig_43014 = 0;
const int filterconfig::filterconfig_51414 = 1;

filterconfig::filterconfig(const std::string& cmfcsv_path, const std::string& illcsv_path, const int& configindex)
{
    std::ifstream cmf_ifs(cmfcsv_path);
    std::ifstream ill_ifs(illcsv_path);
    std::string row;

    std::vector<int> cmf_wavelengths;
    std::vector<float> cmfX;
    std::vector<float> cmfY;
    std::vector<float> cmfZ;

    while (std::getline(cmf_ifs, row))
    {
        std::string cell;
        std::stringstream rowstream(row);
        int i = 0;
        while (std::getline(rowstream, cell, ',')) {
            if (i == 0) {
                int wavelength;
                std::stringstream ss(cell);
                ss >> wavelength;
                cmf_wavelengths.push_back(wavelength);
                ++i;
            } else
            if (i == 1) {
                float x;
                std::stringstream ss(cell);
                ss >> x;
                cmfX.push_back(x);
                ++i;
            } else
            if (i == 2) {
                float y;
                std::stringstream ss(cell);
                ss >> y;
                cmfY.push_back(y);
                ++i;
            } else
            if (i == 3) {
                float z;
                std::stringstream ss(cell);
                ss >> z;
                cmfZ.push_back(z);
                i = 0;
            }
        }
    }
    for (int l = 0; l < cmf_wavelengths.size(); ++l) {
        std::vector<float> cmfs(3);
        cmfs[0] = cmfX[l];
        cmfs[1] = cmfY[l];
        cmfs[2] = cmfZ[l];
        int wl = cmf_wavelengths[l];

        wavelength_cmf_[wl] = cmfs;
    }
    std::vector<float> illuminant_vector;
    std::vector<float> illuminant_wavelengths;
    while (std::getline(ill_ifs, row))
    {
        std::string cell;
        std::stringstream rowstream(row);
        int i = 0;
        int wavelength;
        while (std::getline(rowstream, cell, ',')) {
            if (i == 0) {
                std::stringstream ss(cell);
                ss >> wavelength;
                ++i;
            } else
            if (i == 1) {
                std::stringstream ss(cell);
                float illval;
                ss >> illval;
                illuminant_[wavelength] = (illval / 100);
                illuminant_vector.push_back(illval/100);
                illuminant_wavelengths.push_back(wavelength);
                i = 0;
            }
        }
    }
    // Interpolate 5nm illuminant data to 1nm resolution
    // NOTE: this assumes input data was in 5nm steps and starts at 360nm

    int ill_index = 0;
    for (int l = 0; l < cmf_wavelengths.size(); ++l) {
        if (illuminant_.find(cmf_wavelengths[l]) == illuminant_.end()) { //wavelength not in illuminant_, perform interpolation
            if (ill_index+1 >= illuminant_vector.size()) break;
            if (ill_index+1 >= illuminant_wavelengths.size()) break;

            float x0 = illuminant_vector[ill_index];
            float x1 = illuminant_vector[ill_index+1];
            float w0 = illuminant_wavelengths[ill_index];
            float w1 = illuminant_wavelengths[ill_index+1];
            float wn = cmf_wavelengths[l];

            illuminant_[cmf_wavelengths[l]] = x0 + ((x1 - x0) * ((wn - w0) / (w1 - w0)));
        } else {
            ++ill_index;
        }
    }

    if (configindex == filterconfig::filterconfig_51414) {
        /* this is a default hardware filter configuration for a prototype camera, created on 5/14/2014
         * The configuration is as follows:
         * 15 bandpass filters, 400nm to 690nm in 25nm steps
         * */
        int nfilters = 13;
        int filter_width = 25;
        int start_wavelength = 400;
        int hardware_startindex = 1;


        for (auto filter = 0; filter < 13; ++filter) {
            wavelengths_.push_back(start_wavelength+(filter*filter_width));
            filterpositions_.push_back(hardware_startindex+filter);
        }
        focuspositions_.push_back(52500);
        focuspositions_.push_back(49500);
        focuspositions_.push_back(56000);
        focuspositions_.push_back(47500);
        focuspositions_.push_back(47000);
        focuspositions_.push_back(47000);
        focuspositions_.push_back(47500);
        focuspositions_.push_back(48000);
        focuspositions_.push_back(48000);
        focuspositions_.push_back(48500);
        focuspositions_.push_back(48000);
        focuspositions_.push_back(50000);
        focuspositions_.push_back(50000);

    }

    if (configindex == filterconfig::filterconfig_43014) {
        /* this is a default hardware filter configuration for a prototype camera, created on 4/30/2014
         * The configuration is as follows:
         * 15 bandpass filters, 410nm to 690nm in 20nm steps
         * hardware positions of filters are in order of focus offset to reduce focuser slew time
         * focus positions added to compensate for focus shifts inflicted by the BPFs
         * */

        wavelengths_.push_back(570);
        wavelengths_.push_back(490);
        wavelengths_.push_back(610);
        wavelengths_.push_back(690);
        wavelengths_.push_back(550);
        wavelengths_.push_back(530);
        wavelengths_.push_back(670);
        wavelengths_.push_back(470);
        wavelengths_.push_back(510);
        wavelengths_.push_back(590);
        wavelengths_.push_back(630);
        wavelengths_.push_back(650);
        wavelengths_.push_back(430);
        wavelengths_.push_back(450);
        wavelengths_.push_back(410);

        filterpositions_.push_back(2);
        filterpositions_.push_back(3);
        filterpositions_.push_back(4);
        filterpositions_.push_back(5);
        filterpositions_.push_back(6);
        filterpositions_.push_back(7);
        filterpositions_.push_back(8);
        filterpositions_.push_back(9);
        filterpositions_.push_back(10);
        filterpositions_.push_back(11);
        filterpositions_.push_back(12);
        filterpositions_.push_back(13);
        filterpositions_.push_back(14);
        filterpositions_.push_back(15);
        filterpositions_.push_back(16);


        focuspositions_.push_back(63000);       // These numbers are points on the FLIFocuser where the lens was measured to come to focus.
        focuspositions_.push_back(64000);       // These are used to calculate offsets; the camera must be focused FIRST in order for this process to work.
        focuspositions_.push_back(65000);
        focuspositions_.push_back(65000);
        focuspositions_.push_back(65500);
        focuspositions_.push_back(66000);
        focuspositions_.push_back(66000);
        focuspositions_.push_back(68000);
        focuspositions_.push_back(68000);
        focuspositions_.push_back(68000);
        focuspositions_.push_back(68000);
        focuspositions_.push_back(68000);
        focuspositions_.push_back(70000);
        focuspositions_.push_back(70000);
        focuspositions_.push_back(72000);

        bandpass_width_ = 3;
    }
}
