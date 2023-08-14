// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGPU_GPU_RENDER_BUNDLE_ENCODER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGPU_GPU_RENDER_BUNDLE_ENCODER_H_

#include "third_party/blink/renderer/bindings/modules/v8/v8_gpu_index_format.h"
#include "third_party/blink/renderer/modules/webgpu/dawn_enum_conversions.h"
#include "third_party/blink/renderer/modules/webgpu/dawn_object.h"
#include "third_party/blink/renderer/modules/webgpu/gpu_programmable_pass_encoder.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/bindings/no_alloc_direct_call_host.h"

namespace blink {

class GPUBindGroup;
class GPURenderBundle;
class GPURenderBundleDescriptor;
class GPURenderBundleEncoderDescriptor;

class GPURenderBundleEncoder : public DawnObject<WGPURenderBundleEncoder>,
                               public GPUProgrammablePassEncoder,
                               public NoAllocDirectCallHost {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static GPURenderBundleEncoder* Create(
      GPUDevice* device,
      const GPURenderBundleEncoderDescriptor* webgpu_desc);
  explicit GPURenderBundleEncoder(
      GPUDevice* device,
      WGPURenderBundleEncoder render_bundle_encoder);

  // gpu_render_bundle_encoder.idl
  void setBindGroup(uint32_t index, DawnObject<WGPUBindGroup>* bindGroup) {
    GetProcs().renderBundleEncoderSetBindGroup(
        GetHandle(), index, bindGroup->GetHandle(), 0, nullptr);
  }
  void setBindGroup(uint32_t index,
                    GPUBindGroup* bindGroup,
                    const Vector<uint32_t>& dynamicOffsets);
  void setBindGroup(uint32_t index,
                    GPUBindGroup* bind_group,
                    const FlexibleUint32Array& dynamic_offsets_data,
                    uint64_t dynamic_offsets_data_start,
                    uint32_t dynamic_offsets_data_length,
                    ExceptionState& exception_state);
  void pushDebugGroup(String groupLabel) {
    std::string label = groupLabel.Utf8();
    GetProcs().renderBundleEncoderPushDebugGroup(GetHandle(), label.c_str());
  }
  void popDebugGroup() {
    GetProcs().renderBundleEncoderPopDebugGroup(GetHandle());
  }
  void insertDebugMarker(String markerLabel) {
    std::string label = markerLabel.Utf8();
    GetProcs().renderBundleEncoderInsertDebugMarker(GetHandle(), label.c_str());
  }
  void setPipeline(const DawnObject<WGPURenderPipeline>* pipeline) {
    GetProcs().renderBundleEncoderSetPipeline(GetHandle(),
                                              pipeline->GetHandle());
  }

  void setIndexBuffer(const DawnObject<WGPUBuffer>* buffer,
                      const V8GPUIndexFormat& format,
                      uint64_t offset,
                      uint64_t size) {
    GetProcs().renderBundleEncoderSetIndexBuffer(
        GetHandle(), buffer->GetHandle(), AsDawnEnum(format), offset, size);
  }
  void setVertexBuffer(uint32_t slot,
                       const DawnObject<WGPUBuffer>* buffer,
                       uint64_t offset,
                       uint64_t size) {
    GetProcs().renderBundleEncoderSetVertexBuffer(
        GetHandle(), slot, buffer->GetHandle(), offset, size);
  }
  void draw(uint32_t vertexCount,
            uint32_t instanceCount,
            uint32_t firstVertex,
            uint32_t firstInstance) {
    GetProcs().renderBundleEncoderDraw(GetHandle(), vertexCount, instanceCount,
                                       firstVertex, firstInstance);
  }
  void drawIndexed(uint32_t indexCount,
                   uint32_t instanceCount,
                   uint32_t firstIndex,
                   int32_t baseVertex,
                   uint32_t firstInstance) {
    GetProcs().renderBundleEncoderDrawIndexed(GetHandle(), indexCount,
                                              instanceCount, firstIndex,
                                              baseVertex, firstInstance);
  }
  void drawIndirect(const DawnObject<WGPUBuffer>* indirectBuffer,
                    uint64_t indirectOffset) {
    GetProcs().renderBundleEncoderDrawIndirect(
        GetHandle(), indirectBuffer->GetHandle(), indirectOffset);
  }
  void drawIndexedIndirect(const DawnObject<WGPUBuffer>* indirectBuffer,
                           uint64_t indirectOffset) {
    GetProcs().renderBundleEncoderDrawIndexedIndirect(
        GetHandle(), indirectBuffer->GetHandle(), indirectOffset);
  }

  GPURenderBundle* finish(const GPURenderBundleDescriptor* webgpu_desc);

 private:
  DISALLOW_COPY_AND_ASSIGN(GPURenderBundleEncoder);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGPU_GPU_RENDER_BUNDLE_ENCODER_H_
