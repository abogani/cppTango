#ifndef DevicePipeBlobTestSuite_h
#define DevicePipeBlobTestSuite_h

#include <ctime>
#include <cstdio>
#include <iterator>
#include <memory>
#include <vector>

#include "cxx_common.h"

#undef SUITE_NAME
#define SUITE_NAME DevicePipeBlobTestSuite

class DevicePipeBlobTestSuite: public CxxTest::TestSuite
{
    protected:

    public:
        SUITE_NAME()
        {
            //
            // Arguments check -------------------------------------------------
            //

            CxxTest::TangoPrinter::validate_args();


            // Initialization --------------------------------------------------

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
        void test_operator_insertion()
        {
            //Set up the various pipes
            DevicePipeBlob pipe("test");

            DevicePipeBlob test1("test1");
            DevicePipeBlob test2("test2");
            DevicePipeBlob test3("test3");

            std::string test_data("test data");
            std::string scalar("Scalar");
            std::vector<std::string> names {"test1", "test2", "test3"};
            pipe.set_data_elt_names(names);

            //insert pipes into the main pipe
            pipe << test1 << test2 << test3;

            const DevString datum = Tango::string_dup(test_data);

            //insert datum in the middle element
            TS_ASSERT_THROWS_NOTHING(pipe["test2"] << datum);

            auto* data = pipe.get_insert_data();

            // check insertion is ok
            TS_ASSERT_EQUALS((*data)[1].value.string_att_value()[0].in(), test_data);
            TS_ASSERT_EQUALS((*data)[1].inner_blob_name.in(), scalar);
            TS_ASSERT_DIFFERS((*data)[0].inner_blob_name.in(), scalar);
            TS_ASSERT_DIFFERS((*data)[2].inner_blob_name.in(), scalar);
        }
};
#endif // DevicePipeBlobTestSuite_h
