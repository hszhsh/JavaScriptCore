/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebQuickLookHandleClient.h"

#if USE(QUICK_LOOK)

#include "WebPageProxyMessages.h"
#include "WebProcess.h"
#include <WebCore/QuickLook.h>
#include <wtf/Function.h>
#include <wtf/HashMap.h>
#include <wtf/NeverDestroyed.h>

namespace WebKit {

using PasswordCallbackMap = HashMap<uint64_t, Function<void(const String&)>>;
static PasswordCallbackMap& passwordCallbacks()
{
    static NeverDestroyed<PasswordCallbackMap> callbackMap;
    return callbackMap.get();
}

WebQuickLookHandleClient::WebQuickLookHandleClient(const String& fileName, const String& uti, uint64_t pageID)
    : m_fileName { fileName }
    , m_uti { uti }
    , m_pageID { pageID }
{
    WebProcess::singleton().send(Messages::WebPageProxy::DidStartLoadForQuickLookDocumentInMainFrame(m_fileName, m_uti), m_pageID);
}

WebQuickLookHandleClient::~WebQuickLookHandleClient()
{
    passwordCallbacks().remove(m_pageID);
}

void WebQuickLookHandleClient::didReceiveDataArray(CFArrayRef dataArray)
{
    CFArrayApplyFunction(dataArray, CFRangeMake(0, CFArrayGetCount(dataArray)), [](const void* value, void* context) {
        ASSERT(CFGetTypeID(value) == CFDataGetTypeID());
        static_cast<QuickLookDocumentData*>(context)->append((CFDataRef)value);
    }, &m_data);    
}

void WebQuickLookHandleClient::didFinishLoading()
{
    WebProcess::singleton().send(Messages::WebPageProxy::DidFinishLoadForQuickLookDocumentInMainFrame(m_data), m_pageID);
    m_data.clear();
}

void WebQuickLookHandleClient::didFail()
{
    m_data.clear();
}

void WebQuickLookHandleClient::didRequestPassword(Function<void(const String&)>&& completionHandler)
{
    ASSERT(!passwordCallbacks().contains(m_pageID));
    passwordCallbacks().add(m_pageID, WTFMove(completionHandler));
    WebProcess::singleton().send(Messages::WebPageProxy::DidRequestPasswordForQuickLookDocumentInMainFrame(m_fileName), m_pageID);
}

void WebQuickLookHandleClient::didReceivePassword(const String& password, uint64_t pageID)
{
    ASSERT(passwordCallbacks().contains(pageID));
    auto completionHandler = passwordCallbacks().take(pageID);
    completionHandler(password);
}

} // namespace WebKit

#endif // USE(QUICK_LOOK)
