/*
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "runtime/gmm_helper/gmm_helper.h"
#include "runtime/mem_obj/buffer.h"
#include "runtime/helpers/kernel_commands.h"
#include "unit_tests/fixtures/device_fixture.h"
#include "unit_tests/mocks/mock_kernel.h"
#include "test.h"

using namespace OCLRT;

using Gen9KernelTest = Test<DeviceFixture>;
GEN9TEST_F(Gen9KernelTest, givenKernelWhenCanTransformImagesIsCalledThenReturnsTrue) {
    MockKernelWithInternals mockKernel(*pDevice);
    auto retVal = mockKernel.mockKernel->Kernel::canTransformImages();
    EXPECT_TRUE(retVal);
}
using Gen9KernelCommandsTest = testing::Test;
GEN9TEST_F(Gen9KernelCommandsTest, givenGen9PlatformWhenDoBindingTablePrefetchIsCalledThenReturnsTrue) {
    EXPECT_TRUE(KernelCommandsHelper<FamilyType>::doBindingTablePrefetch());
}

GEN9TEST_F(Gen9KernelCommandsTest, givenBufferThatIsNotZeroCopyWhenSurfaceStateisSetThenL3IsTurnedOn) {
    MockContext context;

    auto retVal = CL_SUCCESS;
    char ptr[16u] = {};

    std::unique_ptr<Buffer> buffer(Buffer::create(
        &context,
        CL_MEM_USE_HOST_PTR,
        16u,
        ptr,
        retVal));

    EXPECT_FALSE(buffer->isMemObjZeroCopy());

    using RENDER_SURFACE_STATE = typename SKLFamily::RENDER_SURFACE_STATE;
    RENDER_SURFACE_STATE surfaceState = {};

    auto gmmHelper = context.getDevice(0)->getExecutionEnvironment()->getGmmHelper();
    gmmHelper->setSimplifiedMocsTableUsage(true);

    buffer->setArgStateful(&surfaceState, false);
    //make sure proper mocs is selected
    EXPECT_EQ(2u, surfaceState.getMemoryObjectControlStateIndexToMocsTables());
}
