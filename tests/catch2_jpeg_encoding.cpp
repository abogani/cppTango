#include "catch2_common.h"

#include <ctime>
#include <cstdio>
#include <iterator>
#include <fstream>
#include <memory>
#include <vector>

// On those tests we encode and decode images from and to raw and jpeg formats.
// These transformations are dependant on the jpeg implementation used.
// Nevertheless we do a binary comparison of the data from those images to check a
// proper conversion was done.
// In case an update of the jpeg library breaks this test, one has to check manually
// that the image generated is conform to the source one.
namespace details
{
constexpr static std::size_t zero = 0;

std::vector<unsigned char> load_file_and_conv(const std::string &file)
{
    std::ifstream read_file(file, std::ios::binary);
    REQUIRE(read_file.is_open());

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
    REQUIRE(length > 1);
    for(std::size_t i = 0; i < length - 1; ++i)
    {
        if(buffer[i] == 0xFF && buffer[i + 1] == 0xDA)
        {
            return i;
        }
    }

    INFO("Expected finding start of a jpeg file");
    REQUIRE(false);
    // workaround compiler not recognizing that TS_FAIL never returns
    return zero;
}

class jpeg_encoder
{
  public:
    jpeg_encoder()
    {
        // Initialization --------------------------------------------------

        std::string resource_path = TANGO_TEST_CATCH2_RESOURCE_PATH;

        // Load all the data needed for the test
        raw_24bits = load_file_and_conv(resource_path + "/peppers.data");
        raw_32bits = load_file_and_conv(resource_path + "/peppers_alpha.data");
        raw_8bits = load_file_and_conv(resource_path + "/peppers_gray.data");
#ifdef JCS_EXTENSIONS
        jpeg_rgb = load_file_and_conv(resource_path + "/peppers.jpeg");
#else
        jpeg_rgb = load_file_and_conv(resource_path + "/peppers-9.jpeg");
#endif
        jpeg_gray = load_file_and_conv(resource_path + "/peppers_gray.jpeg");

        encoder = std::make_unique<Tango::EncodedAttribute>();
    }

    ~jpeg_encoder()
    {
        //
        // Clean up --------------------------------------------------------
        //
    }

    std::unique_ptr<Tango::EncodedAttribute> encoder;
    std::vector<unsigned char> raw_8bits;
    std::vector<unsigned char> raw_24bits;
    std::vector<unsigned char> raw_32bits;

    std::vector<unsigned char> jpeg_rgb;
    std::vector<unsigned char> jpeg_gray;
};
} // namespace details

//
// Tests -------------------------------------------------------
//
// Check the encoding functions
SCENARIO("Images can be converted to and from jpeg")
{
    GIVEN("An encoder and some images, raw and jpeg")
    {
        details::jpeg_encoder test;

        if(test.encoder->is_feature_supported(Tango::EncodedAttribute::Feature::JPEG))
        {
            WHEN("Converting a raw black and white image to jpeg")
            {
                REQUIRE_NOTHROW(test.encoder->encode_jpeg_gray8(test.raw_8bits.data(), 512, 512, 100));
                THEN("A jpeg file is created")
                {
                    std::size_t offset = details::find_jpeg_start(test.encoder->get_data(), test.encoder->get_size());
                    REQUIRE(offset != details::zero);
                }
            }

            WHEN("Converting a raw color image to jpeg")
            {
                REQUIRE_NOTHROW(test.encoder->encode_jpeg_rgb24(test.raw_24bits.data(), 512, 512, 100));
                THEN("A jpeg file is created")
                {
                    std::size_t offset = details::find_jpeg_start(test.encoder->get_data(), test.encoder->get_size());
                    REQUIRE(offset != details::zero);
                }
            }
            if(test.encoder->is_feature_supported(Tango::EncodedAttribute::Feature::JPEG_WITH_ALPHA))
            {
                WHEN("Converting a raw color image with alpha to jpeg")
                {
                    REQUIRE_NOTHROW(test.encoder->encode_jpeg_rgb32(test.raw_32bits.data(), 512, 512, 100));
                    THEN("A jpeg file is created")
                    {
                        std::size_t offset =
                            details::find_jpeg_start(test.encoder->get_data(), test.encoder->get_size());
                        REQUIRE(offset != details::zero);
                    }
                }
            }
            else
            {
                WHEN("Converting a raw color image with alpha to jpeg while the jpeg library does not support it")
                {
                    THEN("An exception is thrown")
                    {
                        using namespace TangoTest::Matchers;

                        REQUIRE_THROWS_MATCHES(test.encoder->encode_jpeg_rgb32(test.raw_32bits.data(), 512, 512, 100),
                                               Tango::DevFailed,
                                               FirstErrorMatches(Reason(Tango::API_UnsupportedFeature)));
                    }
                }
            }
            WHEN("Converting a black and white image to jpeg with incorrect parameters")
            {
                THEN("An exception is thrown")
                {
                    using namespace TangoTest::Matchers;

                    REQUIRE_THROWS_MATCHES(test.encoder->encode_jpeg_gray8(test.raw_8bits.data(), 0, 0, 100),
                                           Tango::DevFailed,
                                           FirstErrorMatches(Reason(Tango::API_EncodeErr)));
                }
            }
            WHEN("Converting a color image to jpeg with incorrect parameters")
            {
                THEN("An exception is thrown")
                {
                    using namespace TangoTest::Matchers;

                    REQUIRE_THROWS_MATCHES(test.encoder->encode_jpeg_rgb24(test.raw_24bits.data(), 0, 0, 100),
                                           Tango::DevFailed,
                                           FirstErrorMatches(Reason(Tango::API_EncodeErr)));
                }
            }
            if(test.encoder->is_feature_supported(Tango::EncodedAttribute::Feature::JPEG_WITH_ALPHA))
            {
                WHEN("Converting a color image with alpha to jpeg with incorrect parameters")
                {
                    THEN("An exception is thrown")
                    {
                        using namespace TangoTest::Matchers;

                        REQUIRE_THROWS_MATCHES(test.encoder->encode_jpeg_rgb32(test.raw_32bits.data(), 0, 0, 100),
                                               Tango::DevFailed,
                                               FirstErrorMatches(Reason(Tango::API_EncodeErr)));
                    }
                }
            }
        }
        else
        {
            WHEN("Converting a black and white image to jpeg without a jpeg library")
            {
                THEN("An exception is thrown")
                {
                    using namespace TangoTest::Matchers;

                    REQUIRE_THROWS_MATCHES(test.encoder->encode_jpeg_gray8(test.raw_8bits.data(), 512, 512, 100),
                                           Tango::DevFailed,
                                           FirstErrorMatches(Reason(Tango::API_UnsupportedFeature)));
                }
            }
            WHEN("Converting a color image to jpeg without a jpeg library")
            {
                THEN("An exception is thrown")
                {
                    using namespace TangoTest::Matchers;

                    REQUIRE_THROWS_MATCHES(test.encoder->encode_jpeg_rgb24(test.raw_24bits.data(), 512, 512, 100),
                                           Tango::DevFailed,
                                           FirstErrorMatches(Reason(Tango::API_UnsupportedFeature)));
                }
            }
            WHEN("Converting a color image with alpha to jpeg without a jpeg library")
            {
                THEN("An exception is thrown")
                {
                    using namespace TangoTest::Matchers;

                    REQUIRE_THROWS_MATCHES(test.encoder->encode_jpeg_rgb32(test.raw_32bits.data(), 512, 512, 100),
                                           Tango::DevFailed,
                                           FirstErrorMatches(Reason(Tango::API_UnsupportedFeature)));
                }
            }
        }
        Tango::DeviceAttribute da_rgb, da_gray, da_error;
        Tango::DevEncoded att_de_rgb, att_de_gray, att_de_error;
        int width, height;
        unsigned char *color_buffer = nullptr;
        unsigned char *gray_buffer = nullptr;

        att_de_rgb.encoded_format = "JPEG_RGB";
        att_de_rgb.encoded_data.length(test.jpeg_rgb.size());
        Tango::DevVarCharArray data_rgb(test.jpeg_rgb.size(), test.jpeg_rgb.size(), test.jpeg_rgb.data(), false);
        att_de_rgb.encoded_data = data_rgb;

        da_rgb << att_de_rgb;

        att_de_gray.encoded_format = "JPEG_GRAY8";
        Tango::DevVarCharArray data_gray(test.jpeg_gray.size(), test.jpeg_gray.size(), test.jpeg_gray.data(), false);
        att_de_gray.encoded_data = data_gray;

        da_gray << att_de_gray;

        att_de_error.encoded_format = "JPEG_GRAY8";
        Tango::DevVarCharArray data_gray_error(
            test.raw_8bits.size(), test.raw_8bits.size(), test.raw_8bits.data(), false);
        att_de_error.encoded_data = data_gray_error;

        da_error << att_de_error;
        if(test.encoder->is_feature_supported(Tango::EncodedAttribute::Feature::JPEG))
        {
            WHEN("Converting a color jpeg image to a raw one")
            {
                REQUIRE_NOTHROW(test.encoder->decode_rgb32(&da_rgb, &width, &height, &color_buffer));
                THEN("An image of the proper size is produced")
                {
                    REQUIRE(width == 512);
                    REQUIRE(height == 512);
                }
            }
            WHEN("Converting a black and white jpeg image to a raw one")
            {
                REQUIRE_NOTHROW(test.encoder->decode_gray8(&da_gray, &width, &height, &gray_buffer));
                THEN("An image of the proper size is produced")
                {
                    REQUIRE(width == 512);
                    REQUIRE(height == 512);
                }
            }
            WHEN("Converting a color jpeg image to a raw one without the proper parameters")
            {
                THEN("An exception is thrown")
                {
                    using namespace TangoTest::Matchers;

                    REQUIRE_THROWS_MATCHES(test.encoder->decode_rgb32(&da_error, &width, &height, &color_buffer),
                                           Tango::DevFailed,
                                           FirstErrorMatches(Reason(Tango::API_WrongFormat)));
                }
            }
            WHEN("Converting a black and white jpeg image to a raw one")
            {
                THEN("An exception is thrown")
                {
                    using namespace TangoTest::Matchers;

                    REQUIRE_THROWS_MATCHES(test.encoder->decode_gray8(&da_error, &width, &height, &color_buffer),
                                           Tango::DevFailed,
                                           FirstErrorMatches(Reason(Tango::API_DecodeErr)));
                }
            }
        }
        else
        {
            WHEN("Converting a color jpeg image to a raw one without a jpeg library")
            {
                THEN("An exception is thrown")
                {
                    using namespace TangoTest::Matchers;

                    REQUIRE_THROWS_MATCHES(test.encoder->decode_rgb32(&da_rgb, &width, &height, &color_buffer),
                                           Tango::DevFailed,
                                           FirstErrorMatches(Reason(Tango::API_UnsupportedFeature)));
                }
            }
            WHEN("Converting a black and white jpeg image to a raw one without a jpeg library")
            {
                THEN("An exception is thrown")
                {
                    using namespace TangoTest::Matchers;

                    REQUIRE_THROWS_MATCHES(test.encoder->decode_gray8(&da_gray, &width, &height, &gray_buffer),
                                           Tango::DevFailed,
                                           FirstErrorMatches(Reason(Tango::API_UnsupportedFeature)));
                }
            }
        }
        delete[] color_buffer;
        delete[] gray_buffer;
    }
}
