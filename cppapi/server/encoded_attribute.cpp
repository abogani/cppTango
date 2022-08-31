///=============================================================================
//
// file :		encoded_attribute.cpp
//
// description :	Management of Tango::DevEncoded format
//
// project :		TANGO
//
// author(s) :		JL Pons
//
// Copyright (C) :      2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015
//                      European Synchrotron Radiation Facility
//                      BP 220, Grenoble 38043
//                      FRANCE
//
// This file is part of Tango.
//
// Tango is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Tango is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Tango.  If not, see <http://www.gnu.org/licenses/>.
//
//
//
//=============================================================================

#include <tango/server/encoded_attribute.h>

#ifdef TANGO_USE_JPEG
#include <iostream>
#endif

using namespace Tango;

#define SAFE_FREE(x) if(x) {free(x);x=NULL;}

// ----------------------------------------------------------------------------

EncodedAttribute::EncodedAttribute():manage_exclusion(false),ext(nullptr) {

    buffer_array = (unsigned char **)calloc(1,sizeof(unsigned char *));
    buffer_array[0] = NULL;
    buffSize_array = (std::size_t*)calloc(1,sizeof(std::size_t));
    buffSize_array[0] = 0;
    format = NULL;
    mutex_array = NULL;
    index = 0;
    buf_elt_nb = 1;
}

EncodedAttribute::EncodedAttribute(int si,bool excl):manage_exclusion(excl),ext(nullptr) {

    buffer_array = (unsigned char **)calloc(si,sizeof(unsigned char *));
    buffSize_array = (std::size_t*)calloc(si,sizeof(std::size_t));
    for (int i = 0;i < si;i++)
    {
        buffer_array[i] = NULL;
        buffSize_array[i] = 0;
    }
    format = NULL;
    index = 0;
    buf_elt_nb = si;

    if (manage_exclusion == true)
        mutex_array = new omni_mutex[si];
}

// ----------------------------------------------------------------------------

EncodedAttribute::~EncodedAttribute() {

    for (int i = 0;i < buf_elt_nb;i++)
        SAFE_FREE(buffer_array[i]);
    SAFE_FREE(buffer_array);
    SAFE_FREE(buffSize_array);

    if (mutex_array != NULL)
        delete [] mutex_array;
}

// ----------------------------------------------------------------------------

void EncodedAttribute::encode_jpeg_gray8(unsigned char *gray8,int width,int height,double quality) {

    if (manage_exclusion == true)
        mutex_array[index].lock();

    SAFE_FREE(buffer_array[index]);
    buffSize_array[index] = 0;
    format = (char *)JPEG_GRAY_8;
    jpeg_encode_gray8(width,height,gray8,quality,&(buffSize_array[index]),&(buffer_array[index]));
    INC_INDEX()
}

// ----------------------------------------------------------------------------

void EncodedAttribute::encode_jpeg_rgb32(unsigned char *rgb32,int width,int height,double quality) {

    if (manage_exclusion == true)
        mutex_array[index].lock();

    SAFE_FREE(buffer_array[index]);
    buffSize_array[index] = 0;
    format = (char *)JPEG_RGB;
    jpeg_encode_rgb32(width,height,rgb32,quality,&(buffSize_array[index]),&(buffer_array[index]));
    INC_INDEX()
}

// ----------------------------------------------------------------------------

void EncodedAttribute::encode_jpeg_rgb24(unsigned char *rgb24,int width,int height,double quality) {

    if (manage_exclusion == true)
        mutex_array[index].lock();

    SAFE_FREE(buffer_array[index]);
    buffSize_array[index] = 0;
    format = (char *)JPEG_RGB;
    jpeg_encode_rgb24(width,height,rgb24,quality,&(buffSize_array[index]),&(buffer_array[index]));
    INC_INDEX()
}

// ----------------------------------------------------------------------------

void EncodedAttribute::encode_gray8(unsigned char *gray8,int width,int height) {

    long unsigned int newSize = width*height + 4;

    if (manage_exclusion == true)
        mutex_array[index].lock();

    if( newSize!=buffSize_array[index] ) {
        SAFE_FREE(buffer_array[index]);
        buffer_array[index] = (unsigned char *)malloc(newSize);
        buffSize_array[index] = newSize;
    }

    format = (char *)GRAY_8;

    // Store image dimension (big endian)
    unsigned char *tmp_ptr = buffer_array[index];
    tmp_ptr[0] = (unsigned char)( (width>>8) & 0xFF );
    tmp_ptr[1] = (unsigned char)( width & 0xFF );
    tmp_ptr[2] = (unsigned char)( (height>>8) & 0xFF );
    tmp_ptr[3] = (unsigned char)( height & 0xFF );

    // Copy image
    memcpy(tmp_ptr+4,gray8,newSize-4);
    INC_INDEX()
}

// ----------------------------------------------------------------------------

void EncodedAttribute::encode_gray16(unsigned short *gray16,int width,int height) {

    long unsigned int newSize = width*height*2 + 4;

    if (manage_exclusion == true)
        mutex_array[index].lock();

    if( newSize!=buffSize_array[index] ) {
        SAFE_FREE(buffer_array[index]);
        buffer_array[index] = (unsigned char *)malloc(newSize);
        buffSize_array[index] = newSize;
    }

    format = (char *)GRAY_16;

    // Store image dimension (big endian)
    unsigned char *tmp_ptr = buffer_array[index];
    tmp_ptr[0] = (unsigned char)( (width>>8) & 0xFF );
    tmp_ptr[1] = (unsigned char)( width & 0xFF );
    tmp_ptr[2] = (unsigned char)( (height>>8) & 0xFF );
    tmp_ptr[3] = (unsigned char)( height & 0xFF );

    // Store image (big endian)
    int srcIdx = 0;
    int dstIdx = 4;
    for(int j=0;j<height;j++) {
        for(int i=0;i<width;i++) {
            unsigned short s = gray16[srcIdx++];
            tmp_ptr[dstIdx++] = (unsigned char)(s >> 8);
            tmp_ptr[dstIdx++] = (unsigned char)(s & 0xFF);
        }
    }
    INC_INDEX()
}

// ----------------------------------------------------------------------------

void EncodedAttribute::encode_rgb24(unsigned char *rgb24,int width,int height) {

    long unsigned int newSize = width*height*3 + 4;

    if (manage_exclusion == true)
        mutex_array[index].lock();

    if( newSize!=buffSize_array[index] ) {
        SAFE_FREE(buffer_array[index]);
        buffer_array[index] = (unsigned char *)malloc(newSize);
        buffSize_array[index] = newSize;
    }

    format = (char *)RGB_24;

    // Store image dimension (big endian)
    unsigned char *tmp_ptr = buffer_array[index];
    tmp_ptr[0] = (unsigned char)( (width>>8) & 0xFF );
    tmp_ptr[1] = (unsigned char)( width & 0xFF );
    tmp_ptr[2] = (unsigned char)( (height>>8) & 0xFF );
    tmp_ptr[3] = (unsigned char)( height & 0xFF );

    // Copy image
    memcpy(tmp_ptr+4,rgb24,newSize-4);

}

// ----------------------------------------------------------------------------

void EncodedAttribute::decode_rgb32(DeviceAttribute *attr,int *width,int *height,unsigned char **rgb32)
{
    if (attr->is_empty())
    {
        TANGO_THROW_EXCEPTION(API_WrongFormat, "Attribute contains no data");
    }

    DevVarEncodedArray_var &encDataSeq = attr->get_Encoded_data();
    if (encDataSeq.operator->() == NULL)
    {
        TANGO_THROW_API_EXCEPTION(ApiDataExcept, API_IncompatibleAttrArgumentType, "Cannot extract, data in DeviceAttribute object is not DevEncoded");
    }

    std::string local_format(encDataSeq.in()[0].encoded_format);

    int isRGB  = (strcmp(local_format.c_str() ,RGB_24 ) == 0);
    int isJPEG = (strcmp(local_format.c_str() ,JPEG_RGB ) == 0);

    if( !isRGB && !isJPEG )
    {
        TANGO_THROW_EXCEPTION(API_WrongFormat, "Not a color format");
    }

    unsigned char *rawBuff = NULL;

    DevVarEncodedArray &encData = encDataSeq.inout();
    DevVarCharArray &encBuff = encData[0].encoded_data;
    std::size_t size = encBuff.length();
    rawBuff = encBuff.get_buffer(false);

    if( isRGB )
    {

        // Get width and height
        int wh = ((int)rawBuff[0] & 0xFF);
        int wl = ((int)rawBuff[1] & 0xFF);
        wh = wh << 8;
        int iWidth = wh | wl;

        int hh = ((int)rawBuff[2] & 0xFF);
        int hl = ((int)rawBuff[3] & 0xFF);
        hh = hh << 8;
        int iHeight = hh | hl;

        unsigned char *data = new unsigned char[iWidth*iHeight*4];

        // Convert to RGB32
        int srcIdx = 4;
        int dstIdx = 0;
        for(int j=0;j<iHeight;j++)
        {
            for(int i=0;i<iWidth;i++)
            {
                data[dstIdx++] = rawBuff[srcIdx++]; // R
                data[dstIdx++] = rawBuff[srcIdx++]; // G
                data[dstIdx++] = rawBuff[srcIdx++]; // B
                data[dstIdx++] = 0;
            }
        }

        *rgb32  = data;
        *width  = iWidth;
        *height = iHeight;

        return;
    }

    if( isJPEG )
    {
        jpeg_decode(size,&(rawBuff[0]),width,height,*rgb32);

        return;
    }
}

// ----------------------------------------------------------------------------

void EncodedAttribute::decode_gray8(DeviceAttribute *attr,int *width,int *height,unsigned char **gray8)
{
    if (attr->is_empty())
    {
        TANGO_THROW_EXCEPTION(API_WrongFormat, "Attribute contains no data");
    }

    DevVarEncodedArray_var &encDataSeq = attr->get_Encoded_data();
    if (encDataSeq.operator->() == NULL)
    {
        TANGO_THROW_API_EXCEPTION(ApiDataExcept, API_IncompatibleAttrArgumentType, "Cannot extract, data in DeviceAttribute object is not DevEncoded");
    }

    std::string local_format(encDataSeq.in()[0].encoded_format);

    int isGrey  = (strcmp(local_format.c_str() ,GRAY_8 ) == 0);
    int isJPEG = (strcmp(local_format.c_str() ,JPEG_GRAY_8 ) == 0);

    if( !isGrey && !isJPEG )
    {
        TANGO_THROW_EXCEPTION(API_WrongFormat, "Not a grayscale 8bit format");
    }

    unsigned char *rawBuff = NULL;

    DevVarEncodedArray &encData = encDataSeq.inout();
    DevVarCharArray &encBuff = encData[0].encoded_data;
    std::size_t size = encBuff.length();
    rawBuff = encBuff.get_buffer(false);

    if( isGrey )
    {

        // Get width and height
        int wh = ((int)rawBuff[0] & 0xFF);
        int wl = ((int)rawBuff[1] & 0xFF);
        wh = wh << 8;
        int iWidth = wh | wl;

        int hh = ((int)rawBuff[2] & 0xFF);
        int hl = ((int)rawBuff[3] & 0xFF);
        hh = hh << 8;
        int iHeight = hh | hl;

        unsigned char *data = new unsigned char[iWidth*iHeight];
        memcpy(data,&(rawBuff[4]),iWidth*iHeight);

        *gray8  = data;
        *width  = iWidth;
        *height = iHeight;

        return;
    }

    if( isJPEG )
    {
        jpeg_decode(size,&(rawBuff[0]),width,height,*gray8);

        return;
    }

}

// ----------------------------------------------------------------------------

void EncodedAttribute::decode_gray16(DeviceAttribute *attr,int *width,int *height,unsigned short **gray16)
{
    if (attr->is_empty())
    {
        TANGO_THROW_EXCEPTION(API_WrongFormat, "Attribute contains no data");
    }

    DevVarEncodedArray_var &encDataSeq = attr->get_Encoded_data();
    if (encDataSeq.operator->() == NULL)
    {
        TANGO_THROW_API_EXCEPTION(ApiDataExcept, API_IncompatibleAttrArgumentType, "Cannot extract, data in DeviceAttribute object is not DevEncoded");
    }

    std::string local_format(encDataSeq.in()[0].encoded_format);

    int isGrey  = (strcmp(local_format.c_str() ,GRAY_16 ) == 0);

    if( !isGrey )
    {
        TANGO_THROW_EXCEPTION(API_WrongFormat, "Not a grayscale 16 bits format");
    }

    unsigned char *rawBuff = NULL;

    DevVarEncodedArray &encData = encDataSeq.inout();
    DevVarCharArray &encBuff = encData[0].encoded_data;
    rawBuff = encBuff.get_buffer(false);

    if( isGrey )
    {

        // Get width and height
        int wh = ((int)rawBuff[0] & 0xFF);
        int wl = ((int)rawBuff[1] & 0xFF);
        wh = wh << 8;
        int iWidth = wh | wl;

        int hh = ((int)rawBuff[2] & 0xFF);
        int hl = ((int)rawBuff[3] & 0xFF);
        hh = hh << 8;
        int iHeight = hh | hl;

        unsigned short *data = new unsigned short[ iWidth*iHeight*2 ];

        int srcIdx = 4;
        int dstIdx = 0;
        for(int j=0;j<iHeight;j++)
        {
            for(int i=0;i<iWidth;i++)
            {
                unsigned short hh = ((unsigned short)rawBuff[srcIdx++] & 0xFF);
                unsigned short hl = ((unsigned short)rawBuff[srcIdx++] & 0xFF);
                data[dstIdx++] = (hh << 8) | hl;
            }
        }

        *gray16 = data;
        *width  = iWidth;
        *height = iHeight;
    }
}

#ifdef TANGO_USE_JPEG
namespace
{
    enum class color_space
    {
        RGB,
        RGBA,
        GRAY
    };

    template<typename JpegCompressDecompressStruct>
    [[noreturn]] void jpeg_throw_exception(const std::string&);

    template<>
    [[noreturn]] void jpeg_throw_exception<jpeg_compress_struct>(const std::string& msg)
    {
        TANGO_THROW_EXCEPTION(API_EncodeErr, msg);
    }

    template<>
    [[noreturn]] void jpeg_throw_exception<jpeg_decompress_struct>(const std::string& msg)
    {
        TANGO_THROW_EXCEPTION(API_DecodeErr, msg);
    }

    template<typename JpegCompressDecompressStruct>
    void jpeg_destroy(JpegCompressDecompressStruct* cinfo_ptr);

    template<>
    void jpeg_destroy(jpeg_compress_struct* cinfo_ptr)
    {
        jpeg_destroy_compress(cinfo_ptr);
    }

    template<>
    void jpeg_destroy(jpeg_decompress_struct* cinfo_ptr)
    {
        jpeg_destroy_decompress(cinfo_ptr);
    }

    template <typename JpegCompressDecompressStruct>
    [[noreturn]] void handle_jpeg_error(j_common_ptr cinfo_ptr)
    {
        assert(cinfo_ptr->err->msg_code <= cinfo_ptr->err->last_jpeg_message);
        std::string err_msg =
            std::string{"libjpeg error "} +
            std::to_string(cinfo_ptr->err->msg_code) + ": " +
            cinfo_ptr->err->jpeg_message_table[cinfo_ptr->err->msg_code];
        // The cast here is safe, there are no instances of jpeg_common_struct,
        // it is just a dummy type used to indicate that the pointed value can
        // be either a jpeg_compress_struct or a jpeg_decompress_struct. We do
        // not dereference j_common_ptr as it would result in UB.
        jpeg_destroy(reinterpret_cast<JpegCompressDecompressStruct*>(cinfo_ptr));
        jpeg_throw_exception<JpegCompressDecompressStruct>(err_msg);
    }


        // ----------------------------------------------------------------
    static void jpeg_encode_rgb(int width,int height,unsigned char *rgb,double quality,
            std::size_t* jpegSize,unsigned char **jpegData, color_space type) {

        jpeg_compress_struct cinfo{};
        jpeg_error_mgr jerr{};
        /* We set up the normal JPEG error routines, then override error_exit. */
        cinfo.err = jpeg_std_error(&jerr);
        jerr.error_exit = &handle_jpeg_error<jpeg_compress_struct>;

        jpeg_create_compress(&cinfo);

        unsigned long size;
        jpeg_mem_dest(&cinfo, jpegData, &size);

        cinfo.image_width = width;      /* image width and height, in pixels */
        cinfo.image_height = height;
        switch(type)
        {
            case color_space::RGB:
                {
                    cinfo.input_components = 3;
                    cinfo.in_color_space = JCS_RGB;
                    break;
                }
            case color_space::RGBA:
                {
#ifdef JCS_EXTENSIONS
                    cinfo.input_components = 4;
                    cinfo.in_color_space = JCS_EXT_RGBA;
#else
                    TANGO_THROW_API_EXCEPTION(ApiNonSuppExcept, API_UnsupportedFeature, "JPEG implementation does not support alpha channel");
#endif
                    break;
                }
            case color_space::GRAY:
                {
                    cinfo.input_components = 1;
                    cinfo.in_color_space = JCS_GRAYSCALE;
                    break;
                }
        }

        jpeg_set_defaults(&cinfo);

        jpeg_set_quality(&cinfo, quality, TRUE);

        jpeg_start_compress(&cinfo, TRUE);

        unsigned char* row_ptr[1];
        while (cinfo.next_scanline < cinfo.image_height) {
            row_ptr[0] = &rgb[cinfo.next_scanline*width*cinfo.input_components];
            jpeg_write_scanlines(&cinfo, row_ptr, 1);
        }

        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);

        // Set it up after the call to jpeg_finish_compress
        // cause this is where it is actually set.
        // Depending on the implementation, if done before it will return 4096 all the time
        *jpegSize = size;
    }
}
// --------------------------------------------------------------------------

void EncodedAttribute::jpeg_encode_rgb32(int width,int height,unsigned char *rgb32,double quality,
        std::size_t* jpegSize,unsigned char **jpegData) {
    jpeg_encode_rgb(width, height, rgb32, quality, jpegSize, jpegData, color_space::RGBA);
}

void EncodedAttribute::jpeg_encode_rgb24(int width,int height,unsigned char *rgb24,double quality,
        std::size_t* jpegSize,unsigned char **jpegData) {
    jpeg_encode_rgb(width, height, rgb24, quality, jpegSize, jpegData, color_space::RGB);
}

// --------------------------------------------------------------------------

void EncodedAttribute::jpeg_encode_gray8(int width,int height,unsigned char *gray8,double quality,
        std::size_t* jpegSize,unsigned char **jpegData) {
    jpeg_encode_rgb(width, height, gray8, quality, jpegSize, jpegData, color_space::GRAY);
}

// --------------------------------------------------------------------------

void EncodedAttribute::jpeg_decode(std::size_t jpegSize, unsigned char *jpegData,
        int *width,int *height,unsigned char*& frame) {

    jpeg_decompress_struct cinfo{};
    jpeg_error_mgr jerr{};
    /* We set up the normal JPEG error routines, then override error_exit. */
    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = &handle_jpeg_error<jpeg_decompress_struct>;

    jpeg_create_decompress(&cinfo);

    jpeg_mem_src(&cinfo, jpegData, (unsigned long)jpegSize);
    jpeg_read_header(&cinfo, TRUE);
    // Check if the image is gray or RGB
    // For RGB images, outputs to RGBA if supported by the jpeg implementation(which is the case for libjpeg-turbo)
    // outputs to RGB otherwise.
    if(cinfo.num_components == 3)
    {
#ifdef JCS_EXTENSIONS
        cinfo.out_color_space = JCS_EXT_RGBA;
#else
        cinfo.out_color_space = JCS_RGB;
#endif
    }

    jpeg_start_decompress(&cinfo);

    *width = cinfo.output_width;
    *height = cinfo.output_height;
    int num_components = cinfo.output_components;

    int row_stride = cinfo.output_width * num_components;
    frame = new unsigned char[row_stride * cinfo.output_height];

    unsigned char *buffer_array[1];
    while (cinfo.output_scanline < cinfo.output_height)
    {
        buffer_array[0] = frame + cinfo.output_scanline * row_stride;
        jpeg_read_scanlines(&cinfo, buffer_array, 1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
}

#else

void EncodedAttribute::jpeg_encode_rgb32(int, int, unsigned char*,
        double, std::size_t*, unsigned char**)
{
    TANGO_THROW_API_EXCEPTION(ApiNonSuppExcept, API_UnsupportedFeature, "Tango was built without jpeg support");
}

void EncodedAttribute::jpeg_encode_rgb24(int, int, unsigned char*,
        double, std::size_t*, unsigned char**)
{
    TANGO_THROW_API_EXCEPTION(ApiNonSuppExcept, API_UnsupportedFeature, "Tango was built without jpeg support");
}

void EncodedAttribute::jpeg_encode_gray8(int, int, unsigned char*,
        double, std::size_t*, unsigned char**)
{
    TANGO_THROW_API_EXCEPTION(ApiNonSuppExcept, API_UnsupportedFeature, "Tango was built without jpeg support");
}

void EncodedAttribute::jpeg_decode(std::size_t, unsigned char*,
        int*, int*, unsigned char*&)
{
    TANGO_THROW_API_EXCEPTION(ApiNonSuppExcept, API_UnsupportedFeature, "Tango was built without jpeg support");
}

#endif // TANGO_USE_JPEG
