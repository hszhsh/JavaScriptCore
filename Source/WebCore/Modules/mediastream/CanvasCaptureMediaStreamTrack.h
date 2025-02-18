/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if ENABLE(MEDIA_STREAM)

#include "HTMLCanvasElement.h"
#include "MediaStreamTrack.h"
#include <wtf/TypeCasts.h>

namespace WebCore {

class HTMLCanvasElement;
class Image;
class ScriptExecutionContext;

class CanvasCaptureMediaStreamTrack final : public MediaStreamTrack {
public:
    static Ref<CanvasCaptureMediaStreamTrack> create(ScriptExecutionContext&, Ref<HTMLCanvasElement>&&, std::optional<double>&& frameRequestRate);

    HTMLCanvasElement& canvas() { return m_canvas.get(); }
    void requestFrame() { m_source->requestFrame(); }

private:
    class Source final : public RealtimeMediaSource, private CanvasObserver {
    public:
        static Ref<Source> create(HTMLCanvasElement&, std::optional<double>&& frameRequestRate);
        
        void requestFrame() { m_shouldEmitFrame = true; }

    private:
        Source(HTMLCanvasElement&, std::optional<double>&&);

        // CanvasObserver API
        void canvasChanged(HTMLCanvasElement&, const FloatRect&) final;
        void canvasResized(HTMLCanvasElement&) final;
        void canvasDestroyed(HTMLCanvasElement&) final;

        // RealtimeMediaSource API
        void startProducingData() final;
        void stopProducingData()  final;
        bool isProducingData() const { return m_isProducingData; }
        RefPtr<RealtimeMediaSourceCapabilities> capabilities() const final { return nullptr; }
        const RealtimeMediaSourceSettings& settings() const final { return m_settings; }
        RefPtr<Image> currentFrameImage() final;
        void paintCurrentFrameInContext(GraphicsContext&, const FloatRect&) final;
        bool applySize(const IntSize&) final { return true; }

        void captureCanvas();
        void requestFrameTimerFired();

        bool m_isProducingData { false };
        bool m_shouldEmitFrame { true };
        std::optional<double> m_frameRequestRate;
        Timer m_requestFrameTimer;
        RealtimeMediaSourceSettings m_settings;
        HTMLCanvasElement* m_canvas;
        RefPtr<Image> m_currentImage;
    };

    CanvasCaptureMediaStreamTrack(ScriptExecutionContext&, Ref<HTMLCanvasElement>&&, Ref<Source>&&);

    bool isCanvas() const final { return true; }
    
    Ref<HTMLCanvasElement> m_canvas;
    Ref<Source> m_source;
};

}

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::CanvasCaptureMediaStreamTrack)
static bool isType(const WebCore::MediaStreamTrack& track) { return track.isCanvas(); }
SPECIALIZE_TYPE_TRAITS_END()

#endif // ENABLE(MEDIA_STREAM)
