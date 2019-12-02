// Copyright (C) 2019. Huawei Technologies Co., Ltd. All rights reserved.

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE 
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR 
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#include <string.h>

#include "tensor_computing.h"
#include "utils.h"

int main(int argc, char *argv[]){
    CHECK_REQUIREMENT(argc == 15);
    // in data
    U32 in = atoi(argv[1]);
    U32 ic = atoi(argv[2]);
    U32 ih = atoi(argv[3]);
    U32 iw = atoi(argv[4]);
    // weight
    U32 fn = atoi(argv[5]);
    U32 fc = atoi(argv[6]);
    U32 fh = atoi(argv[7]);
    U32 fw = atoi(argv[8]);
    // stride & padding
    U32 stride  = atoi(argv[9]);
    U32 padding = atoi(argv[10]);
    // output
    U32 on = atoi(argv[11]);
    U32 oc = atoi(argv[12]);
    U32 oh = atoi(argv[13]);
    U32 ow = atoi(argv[14]);

    CHECK_REQUIREMENT(in == 1 && on == 1);

    DataType dt = DT_F16;
    ActivationMode dw_am = ACTIVATION_NULL;
    ActivationMode pw_am = ACTIVATION_NULL;

    TensorDesc inputDesc, filterDesc, outputDesc, biasDesc;
    ConvolutionDesc convDesc;
    if (ic == 3 || ic == 1) {
        inputDesc = tensor4df(dt, DF_NCHW, in, ic, ih, iw);
    } else {
        inputDesc = tensor4df(dt, DF_NCHWC8, in, ic, ih, iw);
    }
    filterDesc = tensor4df(dt, DF_CHW_NC, oc, ic, fh, fw);
    biasDesc = tensor1d(dt, ic+oc);
    convDesc.stride = stride;
    convDesc.padding = padding;

    // setup input, filter, bias
    F16 *input  = ut_input_v<F16>(in*ic*ih*iw, UT_INIT_RANDOM);
    F16 *filter = ut_input_v<F16>(fc*fh*fw + fn*fc, UT_INIT_RANDOM);
    F16 *bias   = ut_input_v<F16>(ic+oc, UT_INIT_RANDOM);
    F16 *input_ref  = ut_input_v<F16>(in*ic*ih*iw, UT_INIT_ZERO);
    F16 *filter_ref = ut_input_v<F16>(fc*fh*fw + fn*fc, UT_INIT_ZERO);
    F16 *bias_ref   = ut_input_v<F16>(ic+oc, UT_INIT_ZERO);
    memcpy(input_ref,  input,  sizeof(F16)*in*ic*ih*iw);
    memcpy(filter_ref, filter, sizeof(F16)*(fc*fh*fw + fn*fc));
    memcpy(bias_ref,   bias,   sizeof(F16)*(ic+oc));

    // setup output, bias
    U32 outputBytes;
    CHECK_STATUS(depthwise_convolution_infer_output_size(inputDesc, filterDesc, convDesc, &outputDesc, dt, &outputBytes));
    U32 output_size = outputBytes / sizeof(F16);
    F16 *output     = ut_input_v<F16>(output_size, UT_INIT_ZERO);
    F16 *output_ref = ut_input_v<F16>(output_size, UT_INIT_ZERO);

    // setup alg
    ConvolutionPolicy policy = CONVOLUTION_FASTEST;
    DepthwiseConvolutionForwardAlgorithm alg;
    CHECK_STATUS(depthwise_convolution_infer_forward_algorithm(inputDesc, filterDesc, outputDesc, convDesc, policy, &alg, dt, UT_ARCH));

    // setup tmp
    U32 tmpBytes;
    CHECK_STATUS(depthwise_convolution_infer_forward_tmp_bytes(inputDesc, filterDesc, outputDesc, convDesc, alg, &tmpBytes, UT_ARCH));
    F16 *tmp     = ut_input_v<F16>(tmpBytes/sizeof(F16), UT_INIT_ZERO);

    // setup filter trans
    U32 ftmBytes;
    CHECK_STATUS(depthwise_convolution_transform_filter_bytes(filterDesc, alg, &ftmBytes, UT_ARCH));
    F16 *ftm     = ut_input_v<F16>(ftmBytes/sizeof(F16), UT_INIT_ZERO);
    // trans filter
    TensorDesc ftmDesc;
    CHECK_STATUS(depthwise_convolution_transform_filter(filterDesc, filter, alg, &ftmDesc, ftm, UT_ARCH));

    if (UT_CHECK) {
        CHECK_STATUS(depthwise_convolution(inputDesc, input,
                                           ftmDesc, ftm,
                                           convDesc, alg,
                                           biasDesc, bias,
                                           tmpBytes, tmp,
                                           outputDesc, output,
                                           dw_am, pw_am,
                                           UT_ARCH));

        // naive implement
        CHECK_STATUS(depthwise_convolution(inputDesc, input_ref,
                                           filterDesc, filter_ref,
                                           convDesc, alg,
                                           biasDesc, bias_ref,
                                           tmpBytes, tmp,
                                           outputDesc, output_ref,
                                           dw_am, pw_am,
                                           CPU_GENERAL));

        // check
        ut_check_v<F16>(output, output_ref, output_size, F16(0.1), __FILE__, __LINE__);
    }

    // benchmark
    double time_start = ut_time_ms();
    for(int iter = 0; iter < UT_LOOPS; iter++){
        CHECK_STATUS(depthwise_convolution(inputDesc, input,
                                           ftmDesc, ftm,
                                           convDesc, alg,
                                           biasDesc, bias,
                                           tmpBytes, tmp,
                                           outputDesc, output,
                                           dw_am, pw_am,
                                           UT_ARCH));
    }
    double time_end = ut_time_ms();
    double time = (time_end - time_start) / UT_LOOPS;

    // log performance data
    char buffer[150];
    char params[120];
    sprintf(params, "(%u %u %u %u)+(%u %u %u %u)/(%u %u)=(%u %u %u %u)",
                    in, ic, ih, iw,
                    fn, fc, fh, fw,
                    stride, padding,
                    on, oc, oh, ow);
    sprintf(buffer, "%20s, %80s", "DepthwiseConvolution", params);
    double ops = 2.0 * in * ic * ih * iw * fh * fw + in * ic * oh * ow +
                 2.0 * on * oc * oh * ow * ic + on * oc * oh * ow;
    ut_log<F16>(buffer, ops, time);

    free(input);
    free(filter);
    free(bias);
    free(output);
    free(input_ref);
    free(filter_ref);
    free(bias_ref);
    free(output_ref);
    free(tmp);
    free(ftm);

    return 0;
}