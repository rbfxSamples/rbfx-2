//
// Copyright (c) 2008-2020 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

/// \file

#pragma once

#include "../Core/Assert.h"
#include "../Math/MathDefs.h"
#include "../Math/Quaternion.h"
#include "../Replica/NetworkTime.h"

#include <EASTL/optional.h>
#include <EASTL/span.h>
#include <EASTL/vector.h>

namespace Urho3D
{

namespace Detail
{

/// Return squared distance between points.
/// @{
inline float GetDistanceSquared(float lhs, float rhs) { return (lhs - rhs) * (lhs - rhs); }
inline float GetDistanceSquared(const Vector2& lhs, const Vector2& rhs) { return (lhs - rhs).LengthSquared(); }
inline float GetDistanceSquared(const Vector3& lhs, const Vector3& rhs) { return (lhs - rhs).LengthSquared(); }
inline float GetDistanceSquared(const Quaternion& lhs, const Quaternion& rhs) { return 1.0f - Abs(lhs.DotProduct(rhs)); }
/// @}

}

/// Value with derivative, can be extrapolated.
template <class T>
struct ValueWithDerivative
{
    T value_{};
    T derivative_{};
};

template <class T> inline bool operator==(const ValueWithDerivative<T>& lhs, const T& rhs) { return lhs.value_ == rhs; }
template <class T> inline bool operator==(const T& lhs, const ValueWithDerivative<T>& rhs) { return lhs == rhs.value_; }

/// Derivative of a quaternion is angular velocity vector.
template <>
struct ValueWithDerivative<Quaternion>
{
    Quaternion value_{};
    Vector3 derivative_{};
};

/// Helper class to manipulate values stored in NetworkValue.
template <class T>
struct NetworkValueTraits
{
    using InternalType = T;
    using ReturnType = T;

    static InternalType Interpolate(
        const InternalType& lhs, const InternalType& rhs, float blendFactor, float snapThreshold)
    {
        if (Detail::GetDistanceSquared(lhs, rhs) >= snapThreshold * snapThreshold)
            return blendFactor < 0.5f ? lhs : rhs;
        return Lerp(lhs, rhs, blendFactor);
    }

    static ReturnType Extract(const InternalType& value) { return value; }

    static ReturnType Extrapolate(const InternalType& value, float extrapolationFactor) { return value; }

    static void UpdateCorrection(
        ReturnType& inverseCorrection, const ReturnType& correctValue, const ReturnType& oldValue)
    {
        inverseCorrection -= correctValue - oldValue;
    }

    static void SmoothCorrection(ReturnType& inverseCorrection, float blendFactor)
    {
        inverseCorrection = Lerp(inverseCorrection, ReturnType{}, blendFactor);
    }

    static void ApplyCorrection(const ReturnType& inverseCorrection, ReturnType& value)
    {
        value += inverseCorrection;
    }
};

template <>
struct NetworkValueTraits<Quaternion>
{
    using InternalType = Quaternion;
    using ReturnType = Quaternion;

    static Quaternion Extract(const Quaternion& value) { return value; }

    static Quaternion Interpolate(const Quaternion& lhs, const Quaternion& rhs, float blendFactor, float snapThreshold)
    {
        return lhs.Slerp(rhs, blendFactor);
    }

    static Quaternion Extrapolate(const Quaternion& value, float extrapolationFactor) { return value; }

    static void UpdateCorrection(Quaternion& inverseCorrection, const Quaternion& correctValue, const Quaternion& oldValue)
    {
        inverseCorrection = oldValue * correctValue.Inverse() * inverseCorrection;
    }

    static void SmoothCorrection(Quaternion& inverseCorrection, float blendFactor)
    {
        inverseCorrection = inverseCorrection.Slerp(Quaternion::IDENTITY, blendFactor);
    }

    static void ApplyCorrection(const Quaternion& inverseCorrection, Quaternion& value)
    {
        value = inverseCorrection * value;
    }
};

template <class T>
struct NetworkValueTraits<ValueWithDerivative<T>>
{
    using InternalType = ValueWithDerivative<T>;
    using ReturnType = T;

    static InternalType Interpolate(
        const InternalType& lhs, const InternalType& rhs, float blendFactor, float snapThreshold)
    {
        if (Detail::GetDistanceSquared(lhs.value_, rhs.value_) >= snapThreshold * snapThreshold)
            return blendFactor < 0.5f ? lhs : rhs;

        const auto interpolatedValue = Lerp(lhs.value_, rhs.value_, blendFactor);
        const auto interpolatedDerivative = Lerp(lhs.derivative_, rhs.derivative_, blendFactor);
        return InternalType{interpolatedValue, interpolatedDerivative};
    }

    static ReturnType Extract(const InternalType& value) { return value.value_; }

    static ReturnType Extrapolate(const InternalType& value, float extrapolationFactor)
    {
        return value.value_ + value.derivative_ * extrapolationFactor;
    }

    static void UpdateCorrection(
        ReturnType& inverseCorrection, const ReturnType& correctValue, const ReturnType& oldValue)
    {
        NetworkValueTraits<ReturnType>::UpdateCorrection(inverseCorrection, correctValue, oldValue);
    }

    static void SmoothCorrection(ReturnType& inverseCorrection, float blendFactor)
    {
        NetworkValueTraits<ReturnType>::SmoothCorrection(inverseCorrection, blendFactor);
    }

    static void ApplyCorrection(const ReturnType& inverseCorrection, ReturnType& value)
    {
        NetworkValueTraits<ReturnType>::ApplyCorrection(inverseCorrection, value);
    }
};

template <>
struct NetworkValueTraits<ValueWithDerivative<Quaternion>>
{
    using InternalType = ValueWithDerivative<Quaternion>;
    using ReturnType = Quaternion;

    static InternalType Interpolate(
        const InternalType& lhs, const InternalType& rhs, float blendFactor, float snapThreshold)
    {
        const auto interpolatedValue = lhs.value_.Slerp(rhs.value_, blendFactor);
        const auto interpolatedDerivative = Lerp(lhs.derivative_, rhs.derivative_, blendFactor);
        return InternalType{interpolatedValue, interpolatedDerivative};
    }

    static ReturnType Extract(const InternalType& value) { return value.value_; }

    static ReturnType Extrapolate(const InternalType& value, float extrapolationFactor)
    {
        return Quaternion::FromAngularVelocity(value.derivative_ * extrapolationFactor) * value.value_;
    }

    static void UpdateCorrection(
        ReturnType& inverseCorrection, const ReturnType& correctValue, const ReturnType& oldValue)
    {
        NetworkValueTraits<ReturnType>::UpdateCorrection(inverseCorrection, correctValue, oldValue);
    }

    static void SmoothCorrection(ReturnType& inverseCorrection, float blendFactor)
    {
        NetworkValueTraits<ReturnType>::SmoothCorrection(inverseCorrection, blendFactor);
    }

    static void ApplyCorrection(const ReturnType& inverseCorrection, ReturnType& value)
    {
        NetworkValueTraits<ReturnType>::ApplyCorrection(inverseCorrection, value);
    }
};

/// Base class for NetworkValue and NetworkValueVector.
class NetworkValueBase
{
public:
    struct InterpolationBase
    {
        unsigned firstFrame_{};
        unsigned firstIndex_{};

        unsigned secondFrame_{};
        unsigned secondIndex_{};

        float blendFactor_{};
    };

    NetworkValueBase() = default;

    bool IsInitialized() const { return initialized_; }
    unsigned GetCapacity() const { return hasFrameByIndex_.size(); }
    unsigned GetFirstFrame() const { return lastFrame_ - GetCapacity() + 1; }
    unsigned GetLastFrame() const { return lastFrame_; }

    /// Intransitive frame comparison
    /// @{
    static int CompareFrames(unsigned lhs, unsigned rhs) { return Sign(static_cast<int>(lhs - rhs)); }
    static bool IsFrameGreaterThan(unsigned lhs, unsigned rhs) { return CompareFrames(lhs, rhs) > 0; }
    static bool IsFrameLessThan(unsigned lhs, unsigned rhs) { return CompareFrames(lhs, rhs) < 0; }
    static unsigned MaxFrame(unsigned lhs, unsigned rhs) { return IsFrameGreaterThan(lhs, rhs) ? lhs : rhs; }
    static unsigned MinFrame(unsigned lhs, unsigned rhs) { return IsFrameLessThan(lhs, rhs) ? lhs : rhs; }
    /// @}

    void Resize(unsigned capacity)
    {
        URHO3D_ASSERT(capacity > 0);

        hasFrameByIndex_.clear();
        hasFrameByIndex_.resize(capacity);
    }

    ea::optional<unsigned> FrameToIndex(unsigned frame) const
    {
        const auto capacity = GetCapacity();
        const auto behind = static_cast<int>(lastFrame_ - frame);
        if (behind >= 0 && behind < capacity)
            return (lastIndex_ + capacity - behind) % capacity;
        return ea::nullopt;
    }

    unsigned FrameToIndexUnchecked(unsigned frame) const
    {
        const auto index = FrameToIndex(frame);
        URHO3D_ASSERT(index);
        return *index;
    }

    ea::optional<unsigned> AllocatedFrameToIndex(unsigned frame) const
    {
        if (auto index = FrameToIndex(frame))
        {
            if (hasFrameByIndex_[*index])
                return *index;
        }
        return ea::nullopt;
    }

    bool AllocateFrame(unsigned frame)
    {
        URHO3D_ASSERT(!hasFrameByIndex_.empty());

        // Initialize first frame if not intialized
        if (!initialized_)
        {
            initialized_ = true;
            lastFrame_ = frame;
            lastIndex_ = 0;

            hasFrameByIndex_[lastIndex_] = true;
            return true;
        }

        // Roll ring buffer forward if frame is greater
        if (IsFrameGreaterThan(frame, lastFrame_))
        {
            const int offset = static_cast<int>(frame - lastFrame_);
            lastFrame_ += offset;
            lastIndex_ = (lastIndex_ + offset) % GetCapacity();

            // Reset skipped frames
            const unsigned firstSkippedFrame = MaxFrame(lastFrame_ - offset + 1, GetFirstFrame());
            for (unsigned skippedFrame = firstSkippedFrame; skippedFrame != lastFrame_; ++skippedFrame)
                hasFrameByIndex_[FrameToIndexUnchecked(skippedFrame)] = false;

            hasFrameByIndex_[lastIndex_] = true;
            return true;
        }

        // Set past value if within buffer
        if (auto index = FrameToIndex(frame))
        {
            // Set singular past value, overwrite is optional
            hasFrameByIndex_[*index] = true;
            return true;
        }

        return false;
    }

    bool HasFrame(unsigned frame) const { return AllocatedFrameToIndex(frame).has_value(); }

    ea::optional<unsigned> FindClosestAllocatedFrame(unsigned frame, bool searchPast, bool searchFuture) const
    {
        if (HasFrame(frame))
            return frame;

        const unsigned firstFrame = GetFirstFrame();

        // Search past values if any
        if (searchPast && IsFrameGreaterThan(frame, firstFrame))
        {
            const unsigned lastCheckedFrame = MinFrame(lastFrame_, frame - 1);
            for (unsigned pastFrame = lastCheckedFrame; pastFrame != firstFrame - 1; --pastFrame)
            {
                if (HasFrame(pastFrame))
                    return pastFrame;
            }
        }

        // Search future values if any
        if (searchFuture && IsFrameLessThan(frame, lastFrame_))
        {
            const unsigned firstCheckedFrame = MaxFrame(firstFrame, frame + 1);
            for (unsigned futureFrame = firstCheckedFrame; futureFrame != lastFrame_ + 1; ++futureFrame)
            {
                if (HasFrame(futureFrame))
                    return futureFrame;
            }
        }

        return ea::nullopt;
    }

    unsigned GetClosestAllocatedFrame(unsigned frame) const
    {
        URHO3D_ASSERT(initialized_);
        if (const auto closestFrame = FindClosestAllocatedFrame(frame, true, true))
            return *closestFrame;
        return lastFrame_;
    }

    InterpolationBase GetValidFrameInterpolation(const NetworkTime& time) const
    {
        const unsigned frame = time.GetFrame();
        const auto thisOrPastFrame = FindClosestAllocatedFrame(frame, true, false);

        // Optimize for exact queries
        if (thisOrPastFrame == frame && time.GetSubFrame() < M_LARGE_EPSILON)
        {
            const unsigned index = FrameToIndexUnchecked(frame);
            return InterpolationBase{frame, index, frame, index, 0.0f};
        }

        const auto nextOrFutureFrame = FindClosestAllocatedFrame(frame + 1, false, true);
        if (thisOrPastFrame && nextOrFutureFrame)
        {
            const unsigned firstIndex = FrameToIndexUnchecked(*thisOrPastFrame);
            const unsigned secondIndex = FrameToIndexUnchecked(*nextOrFutureFrame);
            const auto extraPastFrames = static_cast<int>(frame - *thisOrPastFrame);
            const auto extraFutureFrames = static_cast<int>(*nextOrFutureFrame - frame - 1);
            const float adjustedFactor =
                (extraPastFrames + time.GetSubFrame()) / (extraPastFrames + extraFutureFrames + 1);
            return InterpolationBase{*thisOrPastFrame, firstIndex, *nextOrFutureFrame, secondIndex, adjustedFactor};
        }

        const unsigned closestFrame = thisOrPastFrame.value_or(nextOrFutureFrame.value_or(lastFrame_));
        const unsigned index = FrameToIndexUnchecked(closestFrame);
        return InterpolationBase{closestFrame, index, closestFrame, index, 0.0f};
    }

    void CollectAllocatedFrames(unsigned firstFrame, unsigned lastFrame, ea::vector<unsigned>& frames) const
    {
        frames.clear();
        for (unsigned frame = firstFrame; frame != lastFrame + 1; ++frame)
        {
            const unsigned index = FrameToIndexUnchecked(frame);
            if (hasFrameByIndex_[index])
                frames.push_back(frame);
        }
    }

    static float GetFrameInterpolationFactor(unsigned lhs, unsigned rhs, unsigned value)
    {
        const auto valueOffset = static_cast<int>(value - lhs);
        const auto maxOffset = static_cast<int>(rhs - lhs);
        return maxOffset >= 0 ? Clamp(static_cast<float>(valueOffset) / maxOffset, 0.0f, 1.0f) : 0.0f;
    }

    static float GetFrameExtrapolationFactor(unsigned baseFrame, unsigned extrapolatedFrame, unsigned maxExtrapolation)
    {
        return static_cast<float>(ea::min(extrapolatedFrame - baseFrame, maxExtrapolation));
    }

private:
    bool initialized_{};
    unsigned lastFrame_{};
    unsigned lastIndex_{};
    ea::vector<bool> hasFrameByIndex_;
};

/// Value stored at multiple points of time in ring buffer.
/// If value was set at least once, it will have at least one valid value forever.
/// On Server, and values are treated as reliable and piecewise-continuous.
/// On Client, values may be extrapolated if frames are missing.
template <class T, class Traits = NetworkValueTraits<T>>
class NetworkValue : private NetworkValueBase
{
public:
    using InternalType = typename Traits::InternalType;
    using ReturnType = typename Traits::ReturnType;

    using NetworkValueBase::IsInitialized;
    using NetworkValueBase::GetLastFrame;

    NetworkValue() = default;
    explicit NetworkValue(const Traits& traits)
        : Traits(traits)
    {
    }

    void Resize(unsigned capacity)
    {
        NetworkValueBase::Resize(capacity);

        values_.clear();
        values_.resize(capacity);
    }

    /// Set value for given frame if possible.
    void Set(unsigned frame, const InternalType& value)
    {
        if (AllocateFrame(frame))
        {
            const unsigned index = FrameToIndexUnchecked(frame);
            values_[index] = value;
        }
    }

    /// Return raw value at given frame.
    ea::optional<InternalType> GetRaw(unsigned frame) const
    {
        if (const auto index = AllocatedFrameToIndex(frame))
            return values_[*index];
        return ea::nullopt;
    }

    /// Return closest valid raw value. Prior values take precedence.
    InternalType GetClosestRaw(unsigned frame) const
    {
        const unsigned closestFrame = GetClosestAllocatedFrame(frame);
        return values_[FrameToIndexUnchecked(closestFrame)];
    }

    /// Interpolate between two frames or return value of the closest valid frame.
    InternalType SampleValid(const NetworkTime& time, float snapThreshold = M_LARGE_VALUE) const
    {
        return CalculateInterpolatedValue(time, snapThreshold).first;
    }

    /// Interpolate between two valid frames if possible.
    ea::optional<InternalType> SamplePrecise(const NetworkTime& time, float snapThreshold = M_LARGE_VALUE) const
    {
        const auto [value, isPrecise] = CalculateInterpolatedValue(time, snapThreshold);
        if (!isPrecise)
            return ea::nullopt;

        return value;
    }

private:
    /// Calculate exact, interpolated or nearest valid value. Return whether the result is precise.
    ea::pair<InternalType, bool> CalculateInterpolatedValue(const NetworkTime& time, float snapThreshold) const
    {
        const InterpolationBase interpolation = GetValidFrameInterpolation(time);

        const InternalType value = interpolation.firstIndex_ == interpolation.secondIndex_
            ? values_[interpolation.firstIndex_]
            : Traits::Interpolate(values_[interpolation.firstIndex_], values_[interpolation.secondIndex_],
                interpolation.blendFactor_, snapThreshold);

        const unsigned frame = time.GetFrame();
        // Consider too old frames "precise" because we are not going to get any new data for them anyway.
        const bool isPrecise = /*interpolation.firstFrame_ <= frame &&*/ frame <= interpolation.secondFrame_;

        return {value, isPrecise};
    }

    ea::vector<InternalType> values_;
};

/// Helper class that manages continuous sampling of NetworkValue on the client side.
template <class T, class Traits = NetworkValueTraits<T>>
class NetworkValueSampler
{
public:
    using NetworkValueType = NetworkValue<T, Traits>;
    using InternalType = typename Traits::InternalType;
    using ReturnType = typename Traits::ReturnType;

    /// Update sampler settings.
    void Setup(unsigned maxExtrapolation, float smoothingConstant, float snapThreshold)
    {
        maxExtrapolation_ = maxExtrapolation;
        smoothingConstant_ = smoothingConstant;
        snapThreshold_ = snapThreshold;
    }

    /// Update sampler state for new time and return current value.
    ea::optional<ReturnType> UpdateAndSample(
        const NetworkValueType& value, const NetworkTime& time, float timeStep)
    {
        if (!value.IsInitialized())
            return ea::nullopt;

        UpdateCorrection(value, timeStep);
        UpdateCache(value, time.GetFrame());

        ReturnType sampledValue = CalculateValueFromCache(value, time);
        previousValue_ = TimeAndValue{time, sampledValue};

        Traits::ApplyCorrection(valueCorrection_, sampledValue);
        return sampledValue;
    }

private:
    float GetExtrapolationFactor(const NetworkTime& time, unsigned baseFrame, unsigned maxExtrapolation) const
    {
        const float factor = (time.GetFrame() - baseFrame) + time.GetSubFrame();
        return ea::min(factor, static_cast<float>(maxExtrapolation));
    }

    void UpdateCorrection(const NetworkValueType& value, float timeStep)
    {
        if (!previousValue_)
            return;

        Traits::SmoothCorrection(valueCorrection_, ExpSmoothing(smoothingConstant_, timeStep));

        UpdateCache(value, previousValue_->time_.GetFrame());
        const ReturnType newPreviousValue = CalculateValueFromCache(value, previousValue_->time_);
        Traits::UpdateCorrection(valueCorrection_, newPreviousValue, previousValue_->value_);
    }

    void UpdateCache(const NetworkValueType& value, unsigned frame)
    {
        // Nothing to do if cache is valid
        if (interpolationCache_ && interpolationCache_->baseFrame_ == frame)
            return;

        if (const auto nextValue = value.SamplePrecise(NetworkTime{frame + 1}, snapThreshold_))
        {
            // Update interpolation if has enough data for it.
            // Get base value from cache if possible, or just take previous frame.
            const InternalType baseValue = (interpolationCache_ && interpolationCache_->baseFrame_ + 1 == frame)
                ? interpolationCache_->nextValue_
                : value.SampleValid(NetworkTime{frame}, snapThreshold_);

            interpolationCache_ = InterpolationCache{frame, baseValue, *nextValue};
            extrapolationFrame_ = frame + 1;
        }
        else
        {
            // Update frame used for extrapolation.
            extrapolationFrame_ = value.GetLastFrame();
            URHO3D_ASSERT(extrapolationFrame_ && *extrapolationFrame_ < frame + 1);
        }
    }

    ReturnType CalculateValueFromCache(const NetworkValueType& value, const NetworkTime& time)
    {
        if (interpolationCache_ && interpolationCache_->baseFrame_ == time.GetFrame())
        {
            const InternalType value = Traits::Interpolate(
                interpolationCache_->baseValue_, interpolationCache_->nextValue_, time.GetSubFrame(), snapThreshold_);
            return Traits::Extract(value);
        }

        URHO3D_ASSERT(extrapolationFrame_);

        const InternalType baseValue = *value.GetRaw(*extrapolationFrame_);
        const float factor = GetExtrapolationFactor(time, *extrapolationFrame_, maxExtrapolation_);
        return Traits::Extrapolate(baseValue, factor);
    }

    unsigned maxExtrapolation_{};
    float smoothingConstant_{};
    float snapThreshold_{M_LARGE_VALUE};

    struct InterpolationCache
    {
        unsigned baseFrame_{};
        InternalType baseValue_{};
        InternalType nextValue_{};
    };

    struct TimeAndValue
    {
        NetworkTime time_;
        ReturnType value_{};
    };

    ea::optional<InterpolationCache> interpolationCache_;
    ea::optional<TimeAndValue> previousValue_;
    ea::optional<unsigned> extrapolationFrame_;

    ReturnType valueCorrection_{};
};

/// Helper class to interpolate value spans.
template <class T, class Traits = NetworkValueTraits<T>>
class InterpolatedConstSpan
{
public:
    explicit InterpolatedConstSpan(ea::span<const T> valueSpan)
        : first_(valueSpan)
        , second_(valueSpan)
        , snapThreshold_(M_LARGE_VALUE)
    {
    }

    InterpolatedConstSpan(ea::span<const T> firstSpan, ea::span<const T> secondSpan, float blendFactor, float snapThreshold)
        : first_(firstSpan)
        , second_(secondSpan)
        , blendFactor_(blendFactor)
        , snapThreshold_(snapThreshold)
    {
    }

    T operator[](unsigned index) const { return Traits::Interpolate(first_[index], second_[index], blendFactor_, snapThreshold_); }

    unsigned Size() const { return first_.size(); }

private:
    ea::span<const T> first_;
    ea::span<const T> second_;
    float blendFactor_{};
    float snapThreshold_{};
};

/// Similar to NetworkValue, except each frame contains an array of elements.
/// Does not support client-side reconstruction.
template <class T, class Traits = NetworkValueTraits<T>>
class NetworkValueVector : private NetworkValueBase
{
public:
    using ValueSpan = ea::span<const T>;
    using InterpolatedValueSpan = InterpolatedConstSpan<T, Traits>;

    NetworkValueVector() = default;

    void Resize(unsigned size, unsigned capacity)
    {
        NetworkValueBase::Resize(capacity);

        size_ = ea::max(1u, size);
        values_.clear();
        values_.resize(size_ * capacity);
    }

    /// Set value for given frame if possible.
    void Set(unsigned frame, ValueSpan value)
    {
        if (AllocateFrame(frame))
        {
            const unsigned index = FrameToIndexUnchecked(frame);
            const unsigned count = ea::min<unsigned>(value.size(), size_);
            ea::copy_n(value.begin(), count, &values_[index * size_]);
        }
    }

    /// Return raw value at given frame.
    ea::optional<ValueSpan> GetRaw(unsigned frame) const
    {
        if (const auto index = AllocatedFrameToIndex(frame))
            return GetSpanForIndex(*index);
        return ea::nullopt;
    }

    /// Return closest valid raw value, if possible. Prior values take precedence.
    ValueSpan GetClosestRaw(unsigned frame) const
    {
        const unsigned closestFrame = GetClosestAllocatedFrame(frame);
        return GetSpanForIndex(FrameToIndexUnchecked(closestFrame));
    }

    /// Server-side sampling: interpolate between consequent frames
    /// or return value of the closest valid frame.
    InterpolatedValueSpan SampleValid(const NetworkTime& time, float snapThreshold = M_LARGE_VALUE) const
    {
        const InterpolationBase interpolation = GetValidFrameInterpolation(time);

        if (interpolation.firstIndex_ == interpolation.secondIndex_)
            return InterpolatedValueSpan{GetSpanForIndex(interpolation.firstIndex_)};

        return InterpolatedValueSpan{GetSpanForIndex(interpolation.firstIndex_),
            GetSpanForIndex(interpolation.secondIndex_), interpolation.blendFactor_, snapThreshold};
    }

    //InterpolatedValueSpan SampleValid(unsigned frame, float snapThreshold = M_LARGE_VALUE) const { return SampleValid(NetworkTime{frame}); }

private:
    ValueSpan GetSpanForIndex(unsigned index) const
    {
        return ValueSpan(values_).subspan(index * size_, size_);
    }

    unsigned size_{};
    ea::vector<T> values_;
};

}
