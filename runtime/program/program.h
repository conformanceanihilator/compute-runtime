/*
 * Copyright (C) 2017-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "block_kernel_manager.h"
#include "elf/reader.h"
#include "kernel_info.h"
#include "runtime/api/cl_types.h"
#include "runtime/device/device.h"
#include "runtime/helpers/base_object.h"
#include "runtime/helpers/stdio.h"
#include "runtime/helpers/string_helpers.h"
#include "elf/writer.h"
#include "igfxfmid.h"
#include "patch_list.h"
#include <vector>
#include <string>
#include <map>

#define OCLRT_ALIGN(a, b) ((((a) % (b)) != 0) ? ((a) - ((a) % (b)) + (b)) : (a))

namespace OCLRT {
class Context;
class CompilerInterface;
class ExecutionEnvironment;
template <>
struct OpenCLObjectMapper<_cl_program> {
    typedef class Program DerivedType;
};

bool isSafeToSkipUnhandledToken(unsigned int token);

class Program : public BaseObject<_cl_program> {
  public:
    static const cl_ulong objectMagic = 0x5651C89100AAACFELL;

    // Create program from binary
    template <typename T = Program>
    static T *create(
        cl_context context,
        cl_uint numDevices,
        const cl_device_id *deviceList,
        const size_t *lengths,
        const unsigned char **binaries,
        cl_int *binaryStatus,
        cl_int &errcodeRet);

    // Create program from source
    template <typename T = Program>
    static T *create(
        cl_context context,
        cl_uint count,
        const char **strings,
        const size_t *lengths,
        cl_int &errcodeRet);

    template <typename T = Program>
    static T *create(
        const char *nullTerminatedString,
        Context *context,
        Device &device,
        bool isBuiltIn,
        cl_int *errcodeRet);

    template <typename T = Program>
    static T *createFromGenBinary(
        ExecutionEnvironment &executionEnvironment,
        Context *context,
        const void *binary,
        size_t size,
        bool isBuiltIn,
        cl_int *errcodeRet);

    template <typename T = Program>
    static T *createFromIL(Context *context,
                           const void *il,
                           size_t length,
                           cl_int &errcodeRet);

    Program(ExecutionEnvironment &executionEnvironment, Context *context, bool isBuiltIn);
    ~Program() override;

    Program(const Program &) = delete;
    Program &operator=(const Program &) = delete;

    cl_int build(cl_uint numDevices, const cl_device_id *deviceList, const char *buildOptions,
                 void(CL_CALLBACK *funcNotify)(cl_program program, void *userData),
                 void *userData, bool enableCaching);

    cl_int build(const cl_device_id device, const char *buildOptions, bool enableCaching,
                 std::unordered_map<std::string, BuiltinDispatchInfoBuilder *> &builtinsMap);

    cl_int build(const char *pKernelData, size_t kernelDataSize);

    MOCKABLE_VIRTUAL cl_int processGenBinary();

    cl_int compile(cl_uint numDevices, const cl_device_id *deviceList, const char *buildOptions,
                   cl_uint numInputHeaders, const cl_program *inputHeaders, const char **headerIncludeNames,
                   void(CL_CALLBACK *funcNotify)(cl_program program, void *userData),
                   void *userData);

    cl_int link(cl_uint numDevices, const cl_device_id *deviceList, const char *buildOptions,
                cl_uint numInputPrograms, const cl_program *inputPrograms,
                void(CL_CALLBACK *funcNotify)(cl_program program, void *userData),
                void *userData);

    size_t getNumKernels() const;
    const KernelInfo *getKernelInfo(const char *kernelName) const;
    const KernelInfo *getKernelInfo(size_t ordinal) const;

    cl_int getInfo(cl_program_info paramName, size_t paramValueSize,
                   void *paramValue, size_t *paramValueSizeRet);

    cl_int getBuildInfo(cl_device_id device, cl_program_build_info paramName,
                        size_t paramValueSize, void *paramValue, size_t *paramValueSizeRet) const;

    cl_build_status getBuildStatus() const {
        return buildStatus;
    }

    Context &getContext() const {
        return *context;
    }

    Context *getContextPtr() const {
        return context;
    }

    ExecutionEnvironment &peekExecutionEnvironment() const {
        return executionEnvironment;
    }

    const Device &getDevice(cl_uint deviceOrdinal) const {
        return *pDevice;
    }

    void setDevice(Device *device) { this->pDevice = device; }

    cl_uint getNumDevices() const {
        return 1;
    }

    MOCKABLE_VIRTUAL cl_int processElfBinary(const void *pBinary, size_t binarySize, uint32_t &binaryVersion);
    cl_int processSpirBinary(const void *pBinary, size_t binarySize, bool isSpirV);

    void setSource(const char *pSourceString);

    cl_int getSource(char *&pBinary, unsigned int &dataSize) const;

    cl_int getSource(std::string &binary) const;

    void storeGenBinary(const void *pSrc, const size_t srcSize);

    char *getGenBinary(size_t &genBinarySize) const {
        genBinarySize = this->genBinarySize;
        return this->genBinary;
    }

    void storeIrBinary(const void *pSrc, const size_t srcSize, bool isSpirV);

    void storeDebugData(const void *pSrc, const size_t srcSize);
    void processDebugData();

    void updateBuildLog(const Device *pDevice, const char *pErrorString, const size_t errorStringSize);

    const char *getBuildLog(const Device *pDevice) const;

    cl_uint getProgramBinaryType() const {
        return programBinaryType;
    }

    bool getIsSpirV() const {
        return isSpirV;
    }

    size_t getProgramScopePatchListSize() const {
        return programScopePatchListSize;
    }

    GraphicsAllocation *getConstantSurface() const {
        return constantSurface;
    }

    GraphicsAllocation *getGlobalSurface() const {
        return globalSurface;
    }

    BlockKernelManager *getBlockKernelManager() const {
        return blockKernelManager;
    }

    void allocateBlockPrivateSurfaces();
    void freeBlockResources();
    void cleanCurrentKernelInfo();

    const std::string &getOptions() const { return options; }

    const std::string &getInternalOptions() const { return internalOptions; }

    bool getAllowNonUniform() const {
        return allowNonUniform;
    }
    bool getIsBuiltIn() const {
        return isBuiltIn;
    }
    uint32_t getProgramOptionVersion() const {
        return programOptionVersion;
    }

    void enableKernelDebug() {
        kernelDebugEnabled = true;
    }

    static bool isValidLlvmBinary(const void *pBinary, size_t binarySize);
    static bool isValidSpirvBinary(const void *pBinary, size_t binarySize);
    bool isKernelDebugEnabled() {
        return kernelDebugEnabled;
    }

    char *getDebugData() {
        return debugData;
    }

    size_t getDebugDataSize() {
        return debugDataSize;
    }

  protected:
    Program(ExecutionEnvironment &executionEnvironment);

    MOCKABLE_VIRTUAL bool isSafeToSkipUnhandledToken(unsigned int token) const;

    MOCKABLE_VIRTUAL cl_int createProgramFromBinary(const void *pBinary, size_t binarySize);

    bool optionsAreNew(const char *options) const;

    cl_int processElfHeader(const CLElfLib::SElf64Header *pElfHeader,
                            cl_program_binary_type &binaryType, uint32_t &numSections);

    void getProgramCompilerVersion(SProgramBinaryHeader *pSectionData, uint32_t &binaryVersion) const;

    cl_int resolveProgramBinary();

    cl_int parseProgramScopePatchList();

    MOCKABLE_VIRTUAL cl_int rebuildProgramFromIr();

    cl_int parsePatchList(KernelInfo &pKernelInfo);

    size_t processKernel(const void *pKernelBlob, cl_int &retVal);

    void storeBinary(char *&pDst, size_t &dstSize, const void *pSrc, const size_t srcSize);

    bool validateGenBinaryDevice(GFXCORE_FAMILY device) const;
    bool validateGenBinaryHeader(const iOpenCL::SProgramBinaryHeader *pGenBinaryHeader) const;

    std::string getKernelNamesString() const;

    void separateBlockKernels();

    void updateNonUniformFlag();
    void updateNonUniformFlag(const Program **inputProgram, size_t numInputPrograms);

    void extractInternalOptions(std::string &options);
    MOCKABLE_VIRTUAL void applyAdditionalOptions();

    MOCKABLE_VIRTUAL bool appendKernelDebugOptions();
    void notifyDebuggerWithSourceCode(std::string &filename);

    static const std::string clOptNameClVer;
    static const std::string clOptNameUniformWgs;
    // clang-format off
    cl_program_binary_type    programBinaryType;
    bool                      isSpirV = false;
    CLElfLib::ElfBinaryStorage elfBinary;
    size_t                    elfBinarySize;

    char*                     genBinary;
    size_t                    genBinarySize;

    char*                     irBinary;
    size_t                    irBinarySize;

    char*                     debugData;
    size_t                    debugDataSize;

    std::vector<KernelInfo*>  kernelInfoArray;
    std::vector<KernelInfo*>  parentKernelInfoArray;
    std::vector<KernelInfo*>  subgroupKernelInfoArray;
    BlockKernelManager *      blockKernelManager;

    const void*               programScopePatchList;
    size_t                    programScopePatchListSize;

    GraphicsAllocation*       constantSurface;
    GraphicsAllocation*       globalSurface;

    size_t                    globalVarTotalSize;

    cl_build_status           buildStatus;
    bool                      isCreatedFromBinary;
    bool                      isProgramBinaryResolved;

    std::string               sourceCode;
    std::string               options;
    std::string               internalOptions;
    static const std::vector<std::string> internalOptionsToExtract;
    std::string               hashFileName;
    std::string               hashFilePath;

    uint32_t                  programOptionVersion;
    bool                      allowNonUniform;

    std::map<const Device*, std::string>  buildLog;

    ExecutionEnvironment&     executionEnvironment;
    Context*                  context;
    Device*                   pDevice;
    cl_uint                   numDevices;

    bool                      isBuiltIn;
    bool                      kernelDebugEnabled = false;
    friend class OfflineCompiler;
    // clang-format on
};
} // namespace OCLRT
