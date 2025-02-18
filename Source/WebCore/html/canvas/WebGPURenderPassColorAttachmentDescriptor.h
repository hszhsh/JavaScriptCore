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

#include "WebGPUObject.h"
#include "WebGPURenderPassAttachmentDescriptor.h"

#include <wtf/Vector.h>

namespace WebCore {

// FIXME: WebGPU - Stub implementation - not implemented yet.
// class GPURenderPassColorAttachmentDescriptor;
class GPURenderPassColorAttachmentDescriptor : public GPURenderPassAttachmentDescriptor {
public:
    Vector<float> clearColor() const { return Vector<float>(); }
    void setClearColor(const Vector<float>&) { }
};
// FIXME: WebGPU - End stub.

class WebGPURenderPassColorAttachmentDescriptor : public WebGPURenderPassAttachmentDescriptor {
public:
    virtual ~WebGPURenderPassColorAttachmentDescriptor();
    static Ref<WebGPURenderPassColorAttachmentDescriptor> create(WebGPURenderingContext*, GPURenderPassColorAttachmentDescriptor*);

    Vector<float> clearColor() const;
    void setClearColor(const Vector<float>&);

    GPURenderPassColorAttachmentDescriptor* renderPassColorAttachmentDescriptor() const;

    bool isColorAttachmentDescriptor() const override { return true; }

private:
    WebGPURenderPassColorAttachmentDescriptor(WebGPURenderingContext*, GPURenderPassColorAttachmentDescriptor*);

};
    
} // namespace WebCore

#endif
