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

#include "config.h"
#include "GPUBuffer.h"

#if ENABLE(WEBGPU)

#import "GPUDevice.h"
#import "Logging.h"

#import <Metal/Metal.h>

namespace WebCore {

GPUBuffer::GPUBuffer(GPUDevice* device, ArrayBufferView* data)
{
    LOG(WebGPU, "GPUBuffer::GPUBuffer()");

    if (!device || !device->platformDevice() || !data)
        return;

    m_buffer = (MTLBuffer*)[device->platformDevice() newBufferWithBytes:data->baseAddress() length:data->byteLength() options:MTLResourceOptionCPUCacheModeDefault];
}

unsigned long GPUBuffer::length() const
{
    if (!m_buffer)
        return 0;

    return [m_buffer length];
}

RefPtr<ArrayBuffer> GPUBuffer::contents()
{
    if (m_contents)
        return m_contents;

    if (!m_buffer)
        return nullptr;

    m_contents = ArrayBuffer::createFromBytes([m_buffer contents], [m_buffer length], [] (void*) { });
    return m_contents;
}

MTLBuffer* GPUBuffer::platformBuffer()
{
    return m_buffer.get();
}

} // namespace WebCore

#endif
