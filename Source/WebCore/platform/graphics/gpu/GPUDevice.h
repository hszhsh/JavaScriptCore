/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if ENABLE(WEBGPU)

#include "PlatformLayer.h"
#include <runtime/ArrayBufferView.h>
#include <wtf/RefCounted.h>

#if USE(CA)
#include "PlatformCALayer.h"
#endif

#if PLATFORM(COCOA)
typedef struct objc_object* id;
OBJC_CLASS CALayer;
OBJC_CLASS WebGPULayer;
#else
class WebGPULayer;
typedef void PlatformGPUDevice;
#endif

namespace WebCore {

class GPUBuffer;
// FIXME: WebGPU - Stub implementation - not implemented yet.
class GPUCommandQueue;
class GPUDrawable;
// FIXME: WebGPU - End stub.
class GPULibrary;
class GPUTexture;
class GPUTextureDescriptor;

class GPUDevice : public RefCounted<GPUDevice> {
public:
    WEBCORE_EXPORT static RefPtr<GPUDevice> create();
    WEBCORE_EXPORT ~GPUDevice();

    void reshape(int width, int height);

#if PLATFORM(COCOA)
    CALayer* platformLayer() const { return reinterpret_cast<CALayer*>(m_layer.get()); }
    WEBCORE_EXPORT id platformDevice();
#endif

    WebGPULayer* layer() { return m_layer.get(); }

    WEBCORE_EXPORT RefPtr<GPULibrary> createLibrary(const String& sourceCode);
    WEBCORE_EXPORT RefPtr<GPUBuffer> createBufferFromData(ArrayBufferView* data);
    WEBCORE_EXPORT RefPtr<GPUTexture> createTexture(GPUTextureDescriptor*);

    // FIXME: WebGPU - Stub implementation - not implemented yet.
    GPUCommandQueue* createCommandQueue() { return nullptr; }
    GPUDrawable* getFramebuffer() { return nullptr; }
    void markLayerComposited() { }
    // FIXME: WebGPU - End stub.

private:
    GPUDevice();

    RetainPtr<WebGPULayer> m_layer;
#if PLATFORM(COCOA)
    RetainPtr<id> m_device;
#endif
};

} // namespace WebCore

#endif
