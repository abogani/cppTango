#ifndef JPEGEncodedTestSuite_h
#define JPEGEncodedTestSuite_h

#include <ctime>
#include <cstdio>
#include <iterator>
#include <memory>
#include <vector>

#ifdef TANGO_USE_JPEG
  // Needed to get the JCS_EXTENSIONS define
  #include <jpeglib.h>
#endif

#include "cxx_common.h"

#undef SUITE_NAME
#define SUITE_NAME JPEGEncodedTestSuite

// On those tests we encode and decode images from and to raw and jpeg formats.
// These transformations are dependant on the jpeg implementation used.
// Nevertheless we do a binary comparison of the data from those images to check a
// proper conversion was done.
// In case an update of the jpeg library breaks this test, one has to check manually
// that the image generated is conform to the source one.
class JPEGEncodedTestSuite : public CxxTest::TestSuite
{
    constexpr static std::size_t zero = 0;

  protected:
    std::unique_ptr<Tango::EncodedAttribute> encoder;

    std::vector<unsigned char> raw_8bits;
    std::vector<unsigned char> raw_24bits;
    std::vector<unsigned char> raw_32bits;

    std::vector<unsigned char> jpeg_rgb;
    std::vector<unsigned char> jpeg_gray;

    std::vector<unsigned char> load_file(const std::string &file)
    {
        std::ifstream read_file(file, std::ios::binary);
        assert(read_file.is_open());

        auto signed_vec = std::vector<char>{std::istreambuf_iterator<char>(read_file), {}};

        // workaround UBSAN integer-implicit-sign-change error
        const size_t length = signed_vec.size();

        std::vector<unsigned char> unsigned_vec(length);
        memcpy(unsigned_vec.data(), reinterpret_cast<unsigned char *>(signed_vec.data()), length);

        return unsigned_vec;
    }

    template <typename T>
    std::size_t find_jpeg_start(const T *buffer, std::size_t length)
    {
        assert(length > 1);
        for(std::size_t i = 0; i < length - 1; ++i)
        {
            if(buffer[i] == 0xFF && buffer[i + 1] == 0xDA)
            {
                return i;
            }
        }
        assert(false);
    }

  public:
    SUITE_NAME()
    {
        //
        // Arguments check -------------------------------------------------
        //

        CxxTest::TangoPrinter::validate_args();

        // Initialization --------------------------------------------------

        std::string resource_path = CxxTest::TangoPrinter::get_param("refpath");

        // Load all the data needed for the test
        raw_24bits = load_file(resource_path + "/peppers.data");
        raw_32bits = load_file(resource_path + "/peppers_alpha.data");
        raw_8bits = load_file(resource_path + "/peppers_gray.data");
#ifdef JCS_EXTENSIONS
        jpeg_rgb = load_file(resource_path + "/peppers.jpeg");
#else
        jpeg_rgb = load_file(resource_path + "/peppers-9.jpeg");
#endif
        jpeg_gray = load_file(resource_path + "/peppers_gray.jpeg");

        encoder = std::make_unique<Tango::EncodedAttribute>();
    }

    virtual ~SUITE_NAME()
    {
        //
        // Clean up --------------------------------------------------------
        //
    }

    static SUITE_NAME *createSuite()
    {
        return new SUITE_NAME();
    }

    static void destroySuite(SUITE_NAME *suite)
    {
        delete suite;
    }

    //
    // Tests -------------------------------------------------------
    //
    // Check the encoding functions
    void test_jpeg_encoding()
    {
#ifdef TANGO_USE_JPEG
        TS_ASSERT_THROWS_NOTHING(encoder->encode_jpeg_gray8(raw_8bits.data(), 512, 512, 100));
        std::size_t offset = find_jpeg_start(encoder->get_data(), encoder->get_size());
        TS_ASSERT_DIFFERS(offset, zero);

        TS_ASSERT_THROWS_NOTHING(encoder->encode_jpeg_rgb24(raw_24bits.data(), 512, 512, 100));

        offset = find_jpeg_start(encoder->get_data(), encoder->get_size());
        TS_ASSERT_DIFFERS(offset, zero);

  #ifdef JCS_EXTENSIONS
        TS_ASSERT_THROWS_NOTHING(encoder->encode_jpeg_rgb32(raw_32bits.data(), 512, 512, 100));

        offset = find_jpeg_start(encoder->get_data(), encoder->get_size());
        TS_ASSERT_DIFFERS(offset, zero);
  #else
        TS_ASSERT_THROWS_ASSERT(encoder->encode_jpeg_rgb32(raw_32bits.data(), 512, 512, 100),
                                Tango::DevFailed & e,
                                TS_ASSERT_EQUALS(std::string(e.errors[0].reason.in()), Tango::API_UnsupportedFeature));
  #endif
        // Check if it throws errors
        TS_ASSERT_THROWS_ASSERT(encoder->encode_jpeg_gray8(raw_8bits.data(), 0, 0, 100),
                                Tango::DevFailed & e,
                                TS_ASSERT_EQUALS(std::string(e.errors[0].reason.in()), Tango::API_EncodeErr));
        TS_ASSERT_THROWS_ASSERT(encoder->encode_jpeg_rgb24(raw_8bits.data(), 0, 0, 100),
                                Tango::DevFailed & e,
                                TS_ASSERT_EQUALS(std::string(e.errors[0].reason.in()), Tango::API_EncodeErr));
  #ifdef JCS_EXTENSIONS
        TS_ASSERT_THROWS_ASSERT(encoder->encode_jpeg_rgb32(raw_8bits.data(), 0, 0, 100),
                                Tango::DevFailed & e,
                                TS_ASSERT_EQUALS(std::string(e.errors[0].reason.in()), Tango::API_EncodeErr));
  #endif

#else

        TS_ASSERT_THROWS_ASSERT(encoder->encode_jpeg_gray8(raw_8bits.data(), 512, 512, 100),
                                Tango::DevFailed & e,
                                TS_ASSERT_EQUALS(std::string(e.errors[0].reason.in()), Tango::API_UnsupportedFeature));
        TS_ASSERT_THROWS_ASSERT(encoder->encode_jpeg_rgb24(raw_24bits.data(), 512, 512, 100),
                                Tango::DevFailed & e,
                                TS_ASSERT_EQUALS(std::string(e.errors[0].reason.in()), Tango::API_UnsupportedFeature));
        TS_ASSERT_THROWS_ASSERT(encoder->encode_jpeg_rgb32(raw_32bits.data(), 512, 512, 100),
                                Tango::DevFailed & e,
                                TS_ASSERT_EQUALS(std::string(e.errors[0].reason.in()), Tango::API_UnsupportedFeature));
#endif
    }

    // Check the decoding functions
    void test_jpeg_decoding()
    {
        DeviceAttribute da_rgb, da_gray, da_error;
        Tango::DevEncoded att_de_rgb, att_de_gray, att_de_error;
        int width, height;
        unsigned char *color_buffer = nullptr;
        unsigned char *gray_buffer = nullptr;

        att_de_rgb.encoded_format = "JPEG_RGB";
        att_de_rgb.encoded_data.length(jpeg_rgb.size());
        Tango::DevVarCharArray data_rgb(jpeg_rgb.size(), jpeg_rgb.size(), jpeg_rgb.data(), false);
        att_de_rgb.encoded_data = data_rgb;

        da_rgb << att_de_rgb;

        att_de_gray.encoded_format = "JPEG_GRAY8";
        Tango::DevVarCharArray data_gray(jpeg_gray.size(), jpeg_gray.size(), jpeg_gray.data(), false);
        att_de_gray.encoded_data = data_gray;

        da_gray << att_de_gray;

        att_de_error.encoded_format = "JPEG_GRAY8";
        Tango::DevVarCharArray data_gray_error(raw_8bits.size(), raw_8bits.size(), raw_8bits.data(), false);
        att_de_error.encoded_data = data_gray_error;

        da_error << att_de_error;

#ifdef TANGO_USE_JPEG
        // Decode jpeg images
        TS_ASSERT_THROWS_NOTHING(encoder->decode_rgb32(&da_rgb, &width, &height, &color_buffer));

        TS_ASSERT_EQUALS(width, 512);
        TS_ASSERT_EQUALS(height, 512);

        TS_ASSERT_THROWS_NOTHING(encoder->decode_gray8(&da_gray, &width, &height, &gray_buffer));
        TS_ASSERT_EQUALS(width, 512);
        TS_ASSERT_EQUALS(height, 512);

        // Check if it throws errors
        TS_ASSERT_THROWS_ASSERT(encoder->decode_gray8(&da_error, &width, &height, &color_buffer),
                                Tango::DevFailed & e,
                                TS_ASSERT_EQUALS(string(e.errors[0].reason.in()), Tango::API_DecodeErr));
        TS_ASSERT_THROWS_ASSERT(encoder->decode_rgb32(&da_error, &width, &height, &color_buffer),
                                Tango::DevFailed & e,
                                TS_ASSERT_EQUALS(string(e.errors[0].reason.in()), Tango::API_WrongFormat));

#else
        TS_ASSERT_THROWS_ASSERT(encoder->decode_rgb32(&da_rgb, &width, &height, &color_buffer),
                                Tango::DevFailed & e,
                                TS_ASSERT_EQUALS(string(e.errors[0].reason.in()), Tango::API_UnsupportedFeature));
        TS_ASSERT_THROWS_ASSERT(encoder->decode_gray8(&da_gray, &width, &height, &gray_buffer),
                                Tango::DevFailed & e,
                                TS_ASSERT_EQUALS(string(e.errors[0].reason.in()), Tango::API_UnsupportedFeature));
#endif
        delete[] color_buffer;
        delete[] gray_buffer;
    }
};
#endif // JPEGEncodedTestSuite_h
