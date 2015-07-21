#include "libtiff\tiffio.h"
#pragma once
#define TIFFTAG_XBINNING 40010
#define TIFFTAG_YBINNING 40011
#define TIFFTAG_NFILTERS 40012
#define TIFFTAG_NLIGHTS  40013
#define TIFFTAG_ILLUMINANT 40014
#define TIFFTAG_WTPTX 40015
#define TIFFTAG_WTPTY 40016
#define TIFFTAG_WTPTVAL 40019
#define TIFFTAG_WTPTMEASURED 40020
//
// This defines custom TIFF tags used by our application, see http://www.remotesensing.org/libtiff/addingtags.html for more information
//
static TIFFExtendProc _ParentExtender = NULL;
static const TIFFFieldInfo xtiffFieldInfo[] = {
    { TIFFTAG_XBINNING, 1, 1, TIFF_SHORT, FIELD_CUSTOM, 1, 0, "X Binning" },
    { TIFFTAG_YBINNING, 1, 1, TIFF_SHORT, FIELD_CUSTOM, 1, 0, "Y Binning" },
    { TIFFTAG_NFILTERS, 1, 1, TIFF_SHORT, FIELD_CUSTOM, 1, 0, "FiltersPerImage" },
    { TIFFTAG_NLIGHTS, 1, 1, TIFF_SHORT, FIELD_CUSTOM, 1, 0, "Lighsourcespermage" },
    { TIFFTAG_ILLUMINANT, 1, 1, TIFF_SHORT, FIELD_CUSTOM, 1, 0, "Illuminant" },
    { TIFFTAG_WTPTX, 1, 1, TIFF_SHORT, FIELD_CUSTOM, 1, 0, "Whitepoint x" },
    { TIFFTAG_WTPTY, 1, 1, TIFF_SHORT, FIELD_CUSTOM, 1, 0, "Whitepoint y" },
    { TIFFTAG_WTPTMEASURED, 1, 1, TIFF_FLOAT, FIELD_CUSTOM, 1, 0, "Measured White Point" },
    { TIFFTAG_WTPTVAL, 1, 1, TIFF_FLOAT, FIELD_CUSTOM, 1, 0, "Whitepoint value" },
};
static void
_XTIFFDefaultDirectory(TIFF *tif)
{
    /* Install the extended Tag field info */
    TIFFMergeFieldInfo(tif, xtiffFieldInfo, sizeof(xtiffFieldInfo) / sizeof(xtiffFieldInfo[0]));

    /* Since an XTIFF client module may have overridden
     * the default directory method, we call it now to
     * allow it to set up the rest of its own methods.
     */

    if (_ParentExtender)
        (*_ParentExtender)(tif);
}

static void _XTIFFInitialize(void)
{
    static int first_time=1;

    if (! first_time) return; /* Been there. Done that. */
    first_time = 0;

    /* Grab the inherited method and install */
    _ParentExtender = TIFFSetTagExtender(_XTIFFDefaultDirectory);
}

