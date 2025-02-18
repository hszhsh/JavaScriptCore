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
#include "WebGPURenderPipelineDescriptor.h"

#if ENABLE(WEBGPU)

#include "GPUFunction.h"
/* FIXME: WebGPU - Not implemented yet.
#include "GPURenderPipelineColorAttachmentDescriptor.h"
#include "GPURenderPipelineDescriptor.h"
*/
#include "WebGPUFunction.h"
#include "WebGPURenderPipelineColorAttachmentDescriptor.h"
#include "WebGPURenderingContext.h"

namespace WebCore {

Ref<WebGPURenderPipelineDescriptor> WebGPURenderPipelineDescriptor::create()
{
    return adoptRef(*new WebGPURenderPipelineDescriptor());
}

WebGPURenderPipelineDescriptor::WebGPURenderPipelineDescriptor()
    : WebGPUObject()
{
    m_renderPipelineDescriptor = GPURenderPipelineDescriptor::create();
}

WebGPURenderPipelineDescriptor::~WebGPURenderPipelineDescriptor()
{
}

String WebGPURenderPipelineDescriptor::label() const
{
    if (!m_renderPipelineDescriptor)
        return emptyString();

    return m_renderPipelineDescriptor->label();
}

void WebGPURenderPipelineDescriptor::setLabel(const String& label)
{
    if (!m_renderPipelineDescriptor)
        return;

    m_renderPipelineDescriptor->setLabel(label);
}

RefPtr<WebGPUFunction> WebGPURenderPipelineDescriptor::vertexFunction() const
{
    return m_vertexFunction;
}

void WebGPURenderPipelineDescriptor::setVertexFunction(RefPtr<WebGPUFunction> newVertexFunction)
{
    if (!newVertexFunction)
        return;

    m_vertexFunction = newVertexFunction;

    if (m_renderPipelineDescriptor)
        m_renderPipelineDescriptor->setVertexFunction(newVertexFunction->function());
}

RefPtr<WebGPUFunction> WebGPURenderPipelineDescriptor::fragmentFunction() const
{
    return m_fragmentFunction;
}

void WebGPURenderPipelineDescriptor::setFragmentFunction(RefPtr<WebGPUFunction> newFragmentFunction)
{
    if (!newFragmentFunction)
        return;

    m_fragmentFunction = newFragmentFunction;

    if (m_renderPipelineDescriptor)
        m_renderPipelineDescriptor->setFragmentFunction(newFragmentFunction->function());
}

Vector<RefPtr<WebGPURenderPipelineColorAttachmentDescriptor>> WebGPURenderPipelineDescriptor::colorAttachments()
{
    if (!m_renderPipelineDescriptor)
        return Vector<RefPtr<WebGPURenderPipelineColorAttachmentDescriptor>>();

    if (!m_colorAttachmentDescriptors.size()) {
        Vector<RefPtr<GPURenderPipelineColorAttachmentDescriptor>> platformColorAttachments = m_renderPipelineDescriptor->colorAttachments();
        for (auto& attachment : platformColorAttachments)
            m_colorAttachmentDescriptors.append(WebGPURenderPipelineColorAttachmentDescriptor::create(this->context(), attachment.get()));
    }
    return m_colorAttachmentDescriptors;
}

unsigned long WebGPURenderPipelineDescriptor::depthAttachmentPixelFormat() const
{
    if (!m_renderPipelineDescriptor)
        return 0; // FIXME: probably a real value for unknown

    return m_renderPipelineDescriptor->depthAttachmentPixelFormat();

}

void WebGPURenderPipelineDescriptor::setDepthAttachmentPixelFormat(unsigned long newPixelFormat)
{
    if (!m_renderPipelineDescriptor)
        return;

    m_renderPipelineDescriptor->setDepthAttachmentPixelFormat(newPixelFormat);
}

void WebGPURenderPipelineDescriptor::reset()
{
    m_vertexFunction = nullptr;
    m_fragmentFunction = nullptr;
}

} // namespace WebCore

#endif
