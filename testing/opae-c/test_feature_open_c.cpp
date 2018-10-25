// Copyright(c) 2017-2018, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote  products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifdef __cplusplus

extern "C" {
#endif

#include <json-c/json.h>
#include <opae/fpga.h>
#include <uuid/uuid.h>
#include "opae_int.h"
#include "safe_string/safe_string.h"

#include "types_int.h"
#include "xfpga.h"
#include "intel-fpga.h"

#ifdef __cplusplus
}
#endif

#include <array>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "test_system.h"
#include "test_opae_c.h"
#include <opae/access.h>
#include <opae/mmio.h>
#include <linux/ioctl.h>

using namespace opae::testing;

/**
 * @test       opaec
 * @brief      Tests: feature_enum_c
 * @details    When fpgaFeatureEnumerate() is called with a valid param,<br>
 *             then it enumerates the accelerateor's DFH to find specific<br>
 *             type of BBB specified in the feature filter<br>
 */

int mmio_ioctl(mock_object * m, int request, va_list argp) {
	int retval = -1;
	errno = EINVAL;
	UNUSED_PARAM(m);
	UNUSED_PARAM(request);
	struct fpga_port_region_info *rinfo = va_arg(argp, struct fpga_port_region_info *);
	if (!rinfo) {
		FPGA_MSG("rinfo is NULL");
		goto out_EINVAL;
	}
	if (rinfo->argsz != sizeof(*rinfo)) {
		FPGA_MSG("wrong structure size");
		goto out_EINVAL;
	}
	if (rinfo->index > 1) {
		FPGA_MSG("unsupported MMIO index");
		goto out_EINVAL;
	}
	if (rinfo->padding != 0) {
		FPGA_MSG("unsupported padding");
		goto out_EINVAL;
	}
	rinfo->flags = FPGA_REGION_READ | FPGA_REGION_WRITE | FPGA_REGION_MMAP;
	rinfo->size = 0x40000;
	rinfo->offset = 0;
	retval = 0;
	errno = 0;
out:
	return retval;

out_EINVAL:
	retval = -1;
	errno = EINVAL;
}

class feature_open_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  feature_open_c_p() : filter_(nullptr), tokens_{{nullptr, nullptr}} {}

  virtual void SetUp() override {

	ASSERT_TRUE(test_platform::exists(GetParam()));
	platform_ = test_platform::get(GetParam());
	system_ = test_system::instance();
	system_->initialize();
	system_->prepare_syfs(platform_);
	invalid_device_ = test_device::unknown();

	ASSERT_EQ(fpgaInitialize(NULL), FPGA_OK);
	ASSERT_EQ(fpgaGetProperties(nullptr, &filter_), FPGA_OK);
	ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
	num_matches_ = 0;
	ASSERT_EQ(fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
		  &num_matches_),
		  FPGA_OK);
	EXPECT_EQ(num_matches_, platform_.devices.size());
	accel_ = nullptr;
	ASSERT_EQ(fpgaOpen(tokens_[0], &accel_, 0), FPGA_OK);
	system_->register_ioctl_handler(FPGA_PORT_GET_REGION_INFO, mmio_ioctl);
	which_mmio_ = 0;
	uint64_t *mmio_ptr = nullptr;
	EXPECT_EQ(fpgaMapMMIO(accel_, which_mmio_, &mmio_ptr), FPGA_OK);
	EXPECT_NE(mmio_ptr, nullptr);

	feature_filter_.type = DMA;     // TODO: 
	fpga_guid guid = {0xE7, 0xE3, 0xE9, 0x58, 0xF2, 0xE8, 0x73, 0x9D, 
					0xE0, 0x4C, 0x48, 0xC1, 0x58, 0x69, 0x81, 0x87 };  // TODO: replace with DMA guid
	memcpy_s(feature_filter_.guid, sizeof(fpga_guid), guid, sizeof(fpga_guid));

  }

  void DestroyTokens() {
    for (auto &t : tokens_) {
      if (t) {
        EXPECT_EQ(fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }

/*	for (auto &t : ftokens_) {
		if (t) {
			EXPECT_EQ(fpgaFeatureTokenDestroy(&t), FPGA_OK);
			t = nullptr;
		}
	} */
    num_matches_ = 0;
  }

  virtual void TearDown() override {
    DestroyTokens();
    if (filter_ != nullptr) {
      EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    }
    system_->finalize();
  }

  uint32_t which_mmio_;
  std::array<fpga_feature_token, 2> ftokens_;
  fpga_handle accel_;
  fpga_feature_properties feature_filter_;
  fpga_feature_handle feature_h;

  fpga_properties filter_;
  std::array<fpga_token, 2> tokens_;
  uint32_t num_matches_;
  test_platform platform_;
  test_device invalid_device_;
  test_system *system_;
};


TEST_P(feature_open_c_p, test_feature_mmio_setup) {
	uint64_t* mmio_ptr = NULL;

	struct DFH dfh ;
	dfh.id = 0x1;
	dfh.revision = 0;
	dfh.next_header_offset = 0x100;
	dfh.eol = 1;
	dfh.reserved = 0;
	dfh.type = 0x1;

	uint64_t offset;
	printf("------dfh.csr = %lx \n", dfh.csr);
	EXPECT_EQ(FPGA_OK, fpgaWriteMMIO64(accel_, which_mmio_, 0x0, dfh.csr));

	EXPECT_EQ(FPGA_OK, fpgaWriteMMIO64(accel_, which_mmio_, 0x8, 0xf89e433683f9040b));
	EXPECT_EQ(FPGA_OK,fpgaWriteMMIO64(accel_, which_mmio_, 0x10, 0xd8424dc4a4a3c413));

	struct DFH dfh_bbb = { 0 };

	dfh_bbb.type = 0x2;
	dfh_bbb.id = 0x2;
	dfh_bbb.revision = 0;
	dfh_bbb.next_header_offset = 0x000;
	dfh_bbb.eol = 1;
	dfh_bbb.reserved = 0;
	printf("------dfh_bbb.csr = %lx \n", dfh_bbb.csr);
		

	EXPECT_EQ(FPGA_OK, fpgaWriteMMIO64(accel_, which_mmio_, 0x100, dfh_bbb.csr));

	EXPECT_EQ(FPGA_OK, fpgaWriteMMIO64(accel_, which_mmio_, 0x108, 0x9D73E8F258E9E3E7));
	EXPECT_EQ(FPGA_OK, fpgaWriteMMIO64(accel_, which_mmio_, 0x110, 0x87816958C1484CE0));
	printf("Before featureEnumerate\n");

	EXPECT_EQ(fpgaFeatureEnumerate(accel_, &feature_filter_, ftokens_.data(),
		ftokens_.size(), &num_matches_), FPGA_OK);
	printf("num_matches for feature enumeration ftoken.size = \n", ftokens_.size());
	EXPECT_EQ(fpgaFeatureOpen(ftokens_[0], 0, &feature_h), FPGA_OK);
	printf("test done\n");
}

TEST_P(feature_open_c_p, nulltokens) {
  EXPECT_EQ(fpgaFeatureOpen(nullptr, 0, &feature_h),
	  FPGA_INVALID_PARAM);
 }

TEST_P(feature_open_c_p, nullhandle) {
  EXPECT_EQ(fpgaFeatureOpen(ftokens_[0], 0, nullptr),
	  FPGA_INVALID_PARAM);
}
/*
TEST_P(feature_open_c_p, mallocfail) {
  system_->invalidate_malloc(0, "fpgaFeatureOpen");
  ASSERT_EQ(fpgaFeatureOpen(ftokens_[0], 0, &feature_h),
	  FPGA_NO_MEMORY);
  EXPECT_EQ(feature_h, nullptr);
} */

INSTANTIATE_TEST_CASE_P(feature_open_c, feature_open_c_p,
                        ::testing::ValuesIn(test_platform::keys(true)));
