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

#include "WebGPUEnums.h"
#include "WebGPUObject.h"

#include <wtf/Vector.h>

namespace WebCore {

// FIXME: WebGPU - Stub implementation - not implemented yet.
// class GPUDepthStencilDescriptor;
class GPUDepthStencilDescriptor : public RefCounted<GPUDepthStencilDescriptor> {
public:
    static RefPtr<GPUDepthStencilDescriptor> create() { return nullptr; }
    String label() const { return emptyString(); }
    void setLabel(const String&) { }
    bool depthWriteEnabled() const { return false; }
    void setDepthWriteEnabled(bool) { }
    void setDepthCompareFunction(GPUCompareFunction) { }
};
// FIXME: WebGPU - End stub.

class WebGPUDepthStencilDescriptor : public WebGPUObject {
public:
    virtual ~WebGPUDepthStencilDescriptor();
    static Ref<WebGPUDepthStencilDescriptor> create();

    String label() const;
    void setLabel(const String&);

    bool depthWriteEnabled() const;
    void setDepthWriteEnabled(bool);

    using CompareFunction = WebGPUCompareFunction;
    CompareFunction depthCompareFunction() const;
    void setDepthCompareFunction(CompareFunction);

    GPUDepthStencilDescriptor* depthStencilDescriptor() { return m_depthStencilDescriptor.get(); }

private:
    WebGPUDepthStencilDescriptor();

    WebGPUCompareFunction m_depthCompareFunction;
    RefPtr<GPUDepthStencilDescriptor> m_depthStencilDescriptor;
};

} // namespace WebCore

#endif
