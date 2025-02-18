/*
 * Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CSSAnimationController.h"

#include "AnimationBase.h"
#include "AnimationEvent.h"
#include "CSSAnimationControllerPrivate.h"
#include "CSSPropertyAnimation.h"
#include "CSSPropertyParser.h"
#include "CompositeAnimation.h"
#include "EventNames.h"
#include "Frame.h"
#include "FrameView.h"
#include "Logging.h"
#include "PseudoElement.h"
#include "RenderView.h"
#include "TransitionEvent.h"
#include "WebKitAnimationEvent.h"
#include "WebKitTransitionEvent.h"
#include <wtf/CurrentTime.h>

namespace WebCore {

// Allow a little more than 60fps to make sure we can at least hit that frame rate.
static const Seconds animationTimerDelay { 0.015 };
// Allow a little more than 30fps to make sure we can at least hit that frame rate.
static const Seconds animationTimerThrottledDelay { 0.030 };

class AnimationPrivateUpdateBlock {
public:
    AnimationPrivateUpdateBlock(CSSAnimationControllerPrivate& animationController)
        : m_animationController(animationController)
    {
        m_animationController.beginAnimationUpdate();
    }
    
    ~AnimationPrivateUpdateBlock()
    {
        m_animationController.endAnimationUpdate();
    }
    
    CSSAnimationControllerPrivate& m_animationController;
};

CSSAnimationControllerPrivate::CSSAnimationControllerPrivate(Frame& frame)
    : m_animationTimer(*this, &CSSAnimationControllerPrivate::animationTimerFired)
    , m_updateStyleIfNeededDispatcher(*this, &CSSAnimationControllerPrivate::updateStyleIfNeededDispatcherFired)
    , m_frame(frame)
    , m_beginAnimationUpdateCount(0)
    , m_waitingForAsyncStartNotification(false)
    , m_allowsNewAnimationsWhileSuspended(false)
{
}

CSSAnimationControllerPrivate::~CSSAnimationControllerPrivate()
{
}

CompositeAnimation& CSSAnimationControllerPrivate::ensureCompositeAnimation(RenderElement& renderer)
{
    auto result = m_compositeAnimations.add(&renderer, nullptr);
    if (result.isNewEntry) {
        result.iterator->value = CompositeAnimation::create(*this);
        renderer.setIsCSSAnimating(true);
    }

    if (animationsAreSuspendedForDocument(&renderer.document()))
        result.iterator->value->suspendAnimations();

    return *result.iterator->value;
}

bool CSSAnimationControllerPrivate::clear(RenderElement& renderer)
{
    LOG(Animations, "CSSAnimationControllerPrivate %p clear: %p", this, &renderer);

    ASSERT(renderer.isCSSAnimating());
    ASSERT(m_compositeAnimations.contains(&renderer));

    Element* element = renderer.element();

    m_eventsToDispatch.removeAllMatching([element] (const EventToDispatch& info) {
        return info.element.ptr() == element;
    });

    m_elementChangesToDispatch.removeAllMatching([element](auto& currentElement) {
        return currentElement.ptr() == element;
    });
    
    // Return false if we didn't do anything OR we are suspended (so we don't try to
    // do a invalidateStyleForSubtree() when suspended).
    RefPtr<CompositeAnimation> animation = m_compositeAnimations.take(&renderer);
    ASSERT(animation);
    renderer.setIsCSSAnimating(false);
    animation->clearRenderer();
    return animation->isSuspended();
}

double CSSAnimationControllerPrivate::updateAnimations(SetChanged callSetChanged/* = DoNotCallSetChanged*/)
{
    AnimationPrivateUpdateBlock updateBlock(*this);
    double timeToNextService = -1;
    bool calledSetChanged = false;

    for (auto& compositeAnimation : m_compositeAnimations) {
        CompositeAnimation& animation = *compositeAnimation.value;
        if (!animation.isSuspended() && animation.hasAnimations()) {
            double t = animation.timeToNextService();
            if (t != -1 && (t < timeToNextService || timeToNextService == -1))
                timeToNextService = t;
            if (!timeToNextService) {
                if (callSetChanged != CallSetChanged)
                    break;
                Element* element = compositeAnimation.key->element();
                ASSERT(element);
                ASSERT(element->document().pageCacheState() == Document::NotInPageCache);
                element->invalidateStyleAndLayerComposition();
                calledSetChanged = true;
            }
        }
    }

    if (calledSetChanged)
        m_frame.document()->updateStyleIfNeeded();

    return timeToNextService;
}

void CSSAnimationControllerPrivate::updateAnimationTimerForRenderer(RenderElement& renderer)
{
    double timeToNextService = 0;

    const CompositeAnimation* compositeAnimation = m_compositeAnimations.get(&renderer);
    if (!compositeAnimation->isSuspended() && compositeAnimation->hasAnimations())
        timeToNextService = compositeAnimation->timeToNextService();

    if (m_animationTimer.isActive() && (m_animationTimer.repeatInterval() || m_animationTimer.nextFireInterval() <= timeToNextService))
        return;

    m_animationTimer.startOneShot(timeToNextService);
}

void CSSAnimationControllerPrivate::updateAnimationTimer(SetChanged callSetChanged/* = DoNotCallSetChanged*/)
{
    double timeToNextService = updateAnimations(callSetChanged);

    LOG(Animations, "updateAnimationTimer: timeToNextService is %.2f", timeToNextService);

    // If we want service immediately, we start a repeating timer to reduce the overhead of starting
    if (!timeToNextService) {
        auto* page = m_frame.page();
        bool shouldThrottle = page && page->isLowPowerModeEnabled();
        Seconds delay = shouldThrottle ? animationTimerThrottledDelay : animationTimerDelay;

        if (!m_animationTimer.isActive() || m_animationTimer.repeatInterval() != delay.value())
            m_animationTimer.startRepeating(delay);
        return;
    }

    // If we don't need service, we want to make sure the timer is no longer running
    if (timeToNextService < 0) {
        if (m_animationTimer.isActive())
            m_animationTimer.stop();
        return;
    }

    // Otherwise, we want to start a one-shot timer so we get here again
    m_animationTimer.startOneShot(timeToNextService);
}

void CSSAnimationControllerPrivate::updateStyleIfNeededDispatcherFired()
{
    fireEventsAndUpdateStyle();
}

void CSSAnimationControllerPrivate::fireEventsAndUpdateStyle()
{
    // Protect the frame from getting destroyed in the event handler
    Ref<Frame> protector(m_frame);

    bool updateStyle = !m_eventsToDispatch.isEmpty() || !m_elementChangesToDispatch.isEmpty();

    // fire all the events
    Vector<EventToDispatch> eventsToDispatch = WTFMove(m_eventsToDispatch);
    for (auto& event : eventsToDispatch) {
        Element& element = event.element;
        if (event.eventType == eventNames().transitionendEvent)
            element.dispatchEvent(TransitionEvent::create(event.eventType, event.name, event.elapsedTime, PseudoElement::pseudoElementNameForEvents(element.pseudoId())));
        else
            element.dispatchEvent(AnimationEvent::create(event.eventType, event.name, event.elapsedTime));
    }

    for (auto& change : m_elementChangesToDispatch)
        change->invalidateStyleAndLayerComposition();

    m_elementChangesToDispatch.clear();

    if (updateStyle)
        m_frame.document()->updateStyleIfNeeded();
}

void CSSAnimationControllerPrivate::startUpdateStyleIfNeededDispatcher()
{
    if (!m_updateStyleIfNeededDispatcher.isActive())
        m_updateStyleIfNeededDispatcher.startOneShot(0);
}

void CSSAnimationControllerPrivate::addEventToDispatch(Element& element, const AtomicString& eventType, const String& name, double elapsedTime)
{
    m_eventsToDispatch.append({ element, eventType, name, elapsedTime });
    startUpdateStyleIfNeededDispatcher();
}

void CSSAnimationControllerPrivate::addElementChangeToDispatch(Element& element)
{
    m_elementChangesToDispatch.append(element);
    ASSERT(m_elementChangesToDispatch.last()->document().pageCacheState() == Document::NotInPageCache);
    startUpdateStyleIfNeededDispatcher();
}

void CSSAnimationControllerPrivate::animationFrameCallbackFired()
{
    double timeToNextService = updateAnimations(CallSetChanged);

    if (timeToNextService >= 0)
        m_frame.document()->view()->scheduleAnimation();
}

void CSSAnimationControllerPrivate::animationTimerFired()
{
    // We need to keep the frame alive, since it owns us.
    Ref<Frame> protector(m_frame);

    // Make sure animationUpdateTime is updated, so that it is current even if no
    // styleChange has happened (e.g. accelerated animations)
    AnimationPrivateUpdateBlock updateBlock(*this);

    // When the timer fires, all we do is call setChanged on all DOM nodes with running animations and then do an immediate
    // updateStyleIfNeeded. It will then call back to us with new information.
    updateAnimationTimer(CallSetChanged);

    // Fire events right away, to avoid a flash of unanimated style after an animation completes, and before
    // the 'end' event fires.
    fireEventsAndUpdateStyle();
}

bool CSSAnimationControllerPrivate::isRunningAnimationOnRenderer(RenderElement& renderer, CSSPropertyID property, AnimationBase::RunningState runningState) const
{
    ASSERT(renderer.isCSSAnimating());
    ASSERT(m_compositeAnimations.contains(&renderer));
    const CompositeAnimation& animation = *m_compositeAnimations.get(&renderer);
    return animation.isAnimatingProperty(property, false, runningState);
}

bool CSSAnimationControllerPrivate::isRunningAcceleratedAnimationOnRenderer(RenderElement& renderer, CSSPropertyID property, AnimationBase::RunningState runningState) const
{
    ASSERT(renderer.isCSSAnimating());
    ASSERT(m_compositeAnimations.contains(&renderer));
    const CompositeAnimation& animation = *m_compositeAnimations.get(&renderer);
    return animation.isAnimatingProperty(property, true, runningState);
}

void CSSAnimationControllerPrivate::updateThrottlingState()
{
    updateAnimationTimer();

    for (auto* childFrame = m_frame.tree().firstChild(); childFrame; childFrame = childFrame->tree().nextSibling())
        childFrame->animation().updateThrottlingState();
}

Seconds CSSAnimationControllerPrivate::animationInterval() const
{
    if (!m_animationTimer.isActive())
        return Seconds { INFINITY };

    return Seconds { m_animationTimer.repeatInterval() };
}

void CSSAnimationControllerPrivate::suspendAnimations()
{
    if (isSuspended())
        return;

    suspendAnimationsForDocument(m_frame.document());

    // Traverse subframes
    for (Frame* child = m_frame.tree().firstChild(); child; child = child->tree().nextSibling())
        child->animation().suspendAnimations();

    m_isSuspended = true;
}

void CSSAnimationControllerPrivate::resumeAnimations()
{
    if (!isSuspended())
        return;

    resumeAnimationsForDocument(m_frame.document());

    // Traverse subframes
    for (Frame* child = m_frame.tree().firstChild(); child; child = child->tree().nextSibling())
        child->animation().resumeAnimations();

    m_isSuspended = false;
}

bool CSSAnimationControllerPrivate::animationsAreSuspendedForDocument(Document* document)
{
    return isSuspended() || m_suspendedDocuments.contains(document);
}

void CSSAnimationControllerPrivate::detachFromDocument(Document* document)
{
    m_suspendedDocuments.remove(document);
}

void CSSAnimationControllerPrivate::suspendAnimationsForDocument(Document* document)
{
    if (animationsAreSuspendedForDocument(document))
        return;

    m_suspendedDocuments.add(document);

    AnimationPrivateUpdateBlock updateBlock(*this);

    for (auto& animation : m_compositeAnimations) {
        if (&animation.key->document() == document)
            animation.value->suspendAnimations();
    }

    updateAnimationTimer();
}

void CSSAnimationControllerPrivate::resumeAnimationsForDocument(Document* document)
{
    if (!animationsAreSuspendedForDocument(document))
        return;

    detachFromDocument(document);

    AnimationPrivateUpdateBlock updateBlock(*this);

    for (auto& animation : m_compositeAnimations) {
        if (&animation.key->document() == document)
            animation.value->resumeAnimations();
    }

    updateAnimationTimer();
}

void CSSAnimationControllerPrivate::startAnimationsIfNotSuspended(Document* document)
{
    if (!animationsAreSuspendedForDocument(document) || allowsNewAnimationsWhileSuspended())
        resumeAnimationsForDocument(document);
}

void CSSAnimationControllerPrivate::setAllowsNewAnimationsWhileSuspended(bool allowed)
{
    m_allowsNewAnimationsWhileSuspended = allowed;
}

bool CSSAnimationControllerPrivate::pauseAnimationAtTime(RenderElement* renderer, const AtomicString& name, double t)
{
    if (!renderer)
        return false;

    CompositeAnimation& compositeAnimation = ensureCompositeAnimation(*renderer);
    if (compositeAnimation.pauseAnimationAtTime(name, t)) {
        renderer->element()->invalidateStyleAndLayerComposition();
        startUpdateStyleIfNeededDispatcher();
        return true;
    }

    return false;
}

bool CSSAnimationControllerPrivate::pauseTransitionAtTime(RenderElement* renderer, const String& property, double t)
{
    if (!renderer)
        return false;

    CompositeAnimation& compositeAnimation = ensureCompositeAnimation(*renderer);
    if (compositeAnimation.pauseTransitionAtTime(cssPropertyID(property), t)) {
        renderer->element()->invalidateStyleAndLayerComposition();
        startUpdateStyleIfNeededDispatcher();
        return true;
    }

    return false;
}

double CSSAnimationControllerPrivate::beginAnimationUpdateTime()
{
    ASSERT(m_beginAnimationUpdateCount);
    if (!m_beginAnimationUpdateTime)
        m_beginAnimationUpdateTime = monotonicallyIncreasingTime();

    return m_beginAnimationUpdateTime.value();
}

void CSSAnimationControllerPrivate::beginAnimationUpdate()
{
    if (!m_beginAnimationUpdateCount)
        m_beginAnimationUpdateTime = std::nullopt;
    ++m_beginAnimationUpdateCount;
}

void CSSAnimationControllerPrivate::endAnimationUpdate()
{
    ASSERT(m_beginAnimationUpdateCount > 0);
    if (m_beginAnimationUpdateCount == 1) {
        styleAvailable();
        if (!m_waitingForAsyncStartNotification)
            startTimeResponse(beginAnimationUpdateTime());
    }
    --m_beginAnimationUpdateCount;
}

void CSSAnimationControllerPrivate::receivedStartTimeResponse(double time)
{
    LOG(Animations, "CSSAnimationControllerPrivate %p receivedStartTimeResponse %f", this, time);

    m_waitingForAsyncStartNotification = false;
    startTimeResponse(time);
}

std::unique_ptr<RenderStyle> CSSAnimationControllerPrivate::getAnimatedStyleForRenderer(RenderElement& renderer)
{
    AnimationPrivateUpdateBlock animationUpdateBlock(*this);

    ASSERT(renderer.isCSSAnimating());
    ASSERT(m_compositeAnimations.contains(&renderer));
    const CompositeAnimation& rendererAnimations = *m_compositeAnimations.get(&renderer);
    std::unique_ptr<RenderStyle> animatingStyle = rendererAnimations.getAnimatedStyle();
    if (!animatingStyle)
        animatingStyle = RenderStyle::clonePtr(renderer.style());
    
    return animatingStyle;
}

bool CSSAnimationControllerPrivate::computeExtentOfAnimation(RenderElement& renderer, LayoutRect& bounds) const
{
    ASSERT(renderer.isCSSAnimating());
    ASSERT(m_compositeAnimations.contains(&renderer));

    const CompositeAnimation& rendererAnimations = *m_compositeAnimations.get(&renderer);
    if (!rendererAnimations.isAnimatingProperty(CSSPropertyTransform, false, AnimationBase::Running | AnimationBase::Paused))
        return true;

    return rendererAnimations.computeExtentOfTransformAnimation(bounds);
}

unsigned CSSAnimationControllerPrivate::numberOfActiveAnimations(Document* document) const
{
    unsigned count = 0;
    
    for (auto& animation : m_compositeAnimations) {
        if (&animation.key->document() == document)
            count += animation.value->numberOfActiveAnimations();
    }

    return count;
}

void CSSAnimationControllerPrivate::addToAnimationsWaitingForStyle(AnimationBase* animation)
{
    // Make sure this animation is not in the start time waiters
    m_animationsWaitingForStartTimeResponse.remove(animation);

    m_animationsWaitingForStyle.add(animation);
}

void CSSAnimationControllerPrivate::removeFromAnimationsWaitingForStyle(AnimationBase* animationToRemove)
{
    m_animationsWaitingForStyle.remove(animationToRemove);
}

void CSSAnimationControllerPrivate::styleAvailable()
{
    // Go through list of waiters and send them on their way
    for (const auto& waitingAnimation : m_animationsWaitingForStyle)
        waitingAnimation->styleAvailable();

    m_animationsWaitingForStyle.clear();
}

void CSSAnimationControllerPrivate::addToAnimationsWaitingForStartTimeResponse(AnimationBase* animation, bool willGetResponse)
{
    // If willGetResponse is true, it means this animation is actually waiting for a response
    // (which will come in as a call to notifyAnimationStarted()).
    // In that case we don't need to add it to this list. We just set a waitingForAResponse flag 
    // which says we are waiting for the response. If willGetResponse is false, this animation 
    // is not waiting for a response for itself, but rather for a notifyXXXStarted() call for 
    // another animation to which it will sync.
    //
    // When endAnimationUpdate() is called we check to see if the waitingForAResponse flag is
    // true. If so, we just return and will do our work when the first notifyXXXStarted() call
    // comes in. If it is false, we will not be getting a notifyXXXStarted() call, so we will
    // do our work right away. In both cases we call the onAnimationStartResponse() method
    // on each animation. In the first case we send in the time we got from notifyXXXStarted().
    // In the second case, we just pass in the beginAnimationUpdateTime().
    //
    // This will synchronize all software and accelerated animations started in the same 
    // updateStyleIfNeeded cycle.
    //
    
    if (willGetResponse)
        m_waitingForAsyncStartNotification = true;

    m_animationsWaitingForStartTimeResponse.add(animation);
}

void CSSAnimationControllerPrivate::removeFromAnimationsWaitingForStartTimeResponse(AnimationBase* animationToRemove)
{
    m_animationsWaitingForStartTimeResponse.remove(animationToRemove);
    
    if (m_animationsWaitingForStartTimeResponse.isEmpty())
        m_waitingForAsyncStartNotification = false;
}

void CSSAnimationControllerPrivate::startTimeResponse(double time)
{
    // Go through list of waiters and send them on their way

    for (const auto& animation : m_animationsWaitingForStartTimeResponse)
        animation->onAnimationStartResponse(time);
    
    m_animationsWaitingForStartTimeResponse.clear();
    m_waitingForAsyncStartNotification = false;
}

void CSSAnimationControllerPrivate::animationWillBeRemoved(AnimationBase* animation)
{
    LOG(Animations, "CSSAnimationControllerPrivate %p animationWillBeRemoved: %p", this, animation);

    removeFromAnimationsWaitingForStyle(animation);
    removeFromAnimationsWaitingForStartTimeResponse(animation);
#if ENABLE(CSS_ANIMATIONS_LEVEL_2)
    removeFromAnimationsDependentOnScroll(animation);
#endif

    bool anyAnimationsWaitingForAsyncStart = false;
    for (auto& animation : m_animationsWaitingForStartTimeResponse) {
        if (animation->waitingForStartTime() && animation->isAccelerated()) {
            anyAnimationsWaitingForAsyncStart = true;
            break;
        }
    }

    if (!anyAnimationsWaitingForAsyncStart)
        m_waitingForAsyncStartNotification = false;
}

#if ENABLE(CSS_ANIMATIONS_LEVEL_2)
void CSSAnimationControllerPrivate::addToAnimationsDependentOnScroll(AnimationBase* animation)
{
    m_animationsDependentOnScroll.add(animation);
}

void CSSAnimationControllerPrivate::removeFromAnimationsDependentOnScroll(AnimationBase* animation)
{
    m_animationsDependentOnScroll.remove(animation);
}

void CSSAnimationControllerPrivate::scrollWasUpdated()
{
    auto* view = m_frame.view();
    if (!view || !wantsScrollUpdates())
        return;

    m_scrollPosition = view->scrollPositionForFixedPosition().y().toFloat();

    // FIXME: This is updating all the animations, rather than just the ones
    // that are dependent on scroll. We to go from our AnimationBase to its CompositeAnimation
    // so we can execute code similar to updateAnimations.
    // https://bugs.webkit.org/show_bug.cgi?id=144170
    updateAnimations(CallSetChanged);
}
#endif

CSSAnimationController::CSSAnimationController(Frame& frame)
    : m_data(std::make_unique<CSSAnimationControllerPrivate>(frame))
{
}

CSSAnimationController::~CSSAnimationController()
{
}

void CSSAnimationController::cancelAnimations(RenderElement& renderer)
{
    if (!renderer.isCSSAnimating())
        return;

    if (!m_data->clear(renderer))
        return;

    Element* element = renderer.element();
    if (!element || element->document().renderTreeBeingDestroyed())
        return;
    ASSERT(element->document().pageCacheState() == Document::NotInPageCache);
    element->invalidateStyleAndLayerComposition();
}

bool CSSAnimationController::updateAnimations(RenderElement& renderer, const RenderStyle& newStyle, std::unique_ptr<RenderStyle>& animatedStyle)
{
    auto* oldStyle = renderer.hasInitializedStyle() ? &renderer.style() : nullptr;
    if ((!oldStyle || (!oldStyle->animations() && !oldStyle->transitions())) && (!newStyle.animations() && !newStyle.transitions()))
        return false;

    if (renderer.document().pageCacheState() != Document::NotInPageCache)
        return false;

    // Don't run transitions when printing.
    if (renderer.view().printing())
        return false;

    // Fetch our current set of implicit animations from a hashtable. We then compare them
    // against the animations in the style and make sure we're in sync. If destination values
    // have changed, we reset the animation. We then do a blend to get new values and we return
    // a new style.

    // We don't support anonymous pseudo elements like :first-line or :first-letter.
    ASSERT(renderer.element());

    CompositeAnimation& rendererAnimations = m_data->ensureCompositeAnimation(renderer);
    bool animationStateChanged = rendererAnimations.animate(renderer, oldStyle, newStyle, animatedStyle);

    if (renderer.parent() || newStyle.animations() || (oldStyle && oldStyle->animations())) {
        m_data->updateAnimationTimerForRenderer(renderer);
        renderer.view().frameView().scheduleAnimation();
    }

    return animationStateChanged;
}

std::unique_ptr<RenderStyle> CSSAnimationController::getAnimatedStyleForRenderer(RenderElement& renderer)
{
    if (!renderer.isCSSAnimating())
        return RenderStyle::clonePtr(renderer.style());
    return m_data->getAnimatedStyleForRenderer(renderer);
}

bool CSSAnimationController::computeExtentOfAnimation(RenderElement& renderer, LayoutRect& bounds) const
{
    if (!renderer.isCSSAnimating())
        return true;

    return m_data->computeExtentOfAnimation(renderer, bounds);
}

void CSSAnimationController::notifyAnimationStarted(RenderElement& renderer, double startTime)
{
    LOG(Animations, "CSSAnimationController %p notifyAnimationStarted on renderer %p, time=%f", this, &renderer, startTime);
    UNUSED_PARAM(renderer);

    AnimationUpdateBlock animationUpdateBlock(this);
    m_data->receivedStartTimeResponse(startTime);
}

bool CSSAnimationController::pauseAnimationAtTime(RenderElement* renderer, const AtomicString& name, double t)
{
    AnimationUpdateBlock animationUpdateBlock(this);
    return m_data->pauseAnimationAtTime(renderer, name, t);
}

unsigned CSSAnimationController::numberOfActiveAnimations(Document* document) const
{
    return m_data->numberOfActiveAnimations(document);
}

bool CSSAnimationController::pauseTransitionAtTime(RenderElement* renderer, const String& property, double t)
{
    AnimationUpdateBlock animationUpdateBlock(this);
    return m_data->pauseTransitionAtTime(renderer, property, t);
}

bool CSSAnimationController::isRunningAnimationOnRenderer(RenderElement& renderer, CSSPropertyID property, AnimationBase::RunningState runningState) const
{
    return renderer.isCSSAnimating() && m_data->isRunningAnimationOnRenderer(renderer, property, runningState);
}

bool CSSAnimationController::isRunningAcceleratedAnimationOnRenderer(RenderElement& renderer, CSSPropertyID property, AnimationBase::RunningState runningState) const
{
    return renderer.isCSSAnimating() && m_data->isRunningAcceleratedAnimationOnRenderer(renderer, property, runningState);
}

bool CSSAnimationController::isSuspended() const
{
    return m_data->isSuspended();
}

void CSSAnimationController::suspendAnimations()
{
    LOG(Animations, "controller is suspending animations");
    m_data->suspendAnimations();
}

void CSSAnimationController::resumeAnimations()
{
    LOG(Animations, "controller is resuming animations");
    m_data->resumeAnimations();
}

bool CSSAnimationController::allowsNewAnimationsWhileSuspended() const
{
    return m_data->allowsNewAnimationsWhileSuspended();
}

void CSSAnimationController::setAllowsNewAnimationsWhileSuspended(bool allowed)
{
    m_data->setAllowsNewAnimationsWhileSuspended(allowed);
}

void CSSAnimationController::serviceAnimations()
{
    m_data->animationFrameCallbackFired();
}

void CSSAnimationController::updateThrottlingState()
{
    m_data->updateThrottlingState();
}

Seconds CSSAnimationController::animationInterval() const
{
    return m_data->animationInterval();
}

bool CSSAnimationController::animationsAreSuspendedForDocument(Document* document)
{
    return m_data->animationsAreSuspendedForDocument(document);
}

void CSSAnimationController::detachFromDocument(Document* document)
{
    return m_data->detachFromDocument(document);
}

void CSSAnimationController::suspendAnimationsForDocument(Document* document)
{
    LOG(Animations, "suspending animations for document %p", document);
    m_data->suspendAnimationsForDocument(document);
}

void CSSAnimationController::resumeAnimationsForDocument(Document* document)
{
    LOG(Animations, "resuming animations for document %p", document);
    AnimationUpdateBlock animationUpdateBlock(this);
    m_data->resumeAnimationsForDocument(document);
}

void CSSAnimationController::startAnimationsIfNotSuspended(Document* document)
{
    LOG(Animations, "animations may start for document %p", document);

    AnimationUpdateBlock animationUpdateBlock(this);
    m_data->startAnimationsIfNotSuspended(document);
}

void CSSAnimationController::beginAnimationUpdate()
{
    m_data->beginAnimationUpdate();
}

void CSSAnimationController::endAnimationUpdate()
{
    m_data->endAnimationUpdate();
}

bool CSSAnimationController::supportsAcceleratedAnimationOfProperty(CSSPropertyID property)
{
    return CSSPropertyAnimation::animationOfPropertyIsAccelerated(property);
}

#if ENABLE(CSS_ANIMATIONS_LEVEL_2)
bool CSSAnimationController::wantsScrollUpdates() const
{
    return m_data->wantsScrollUpdates();
}

void CSSAnimationController::scrollWasUpdated()
{
    m_data->scrollWasUpdated();
}
#endif

bool CSSAnimationController::hasAnimations() const
{
    return m_data->hasAnimations();
}

} // namespace WebCore
