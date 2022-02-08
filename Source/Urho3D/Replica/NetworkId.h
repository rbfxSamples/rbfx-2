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

#include "../Scene/TrackedComponent.h"

namespace Urho3D
{

/// ID used to identify unique NetworkObject within Scene.
using NetworkId = ComponentReference;
static constexpr NetworkId InvalidNetworkId = NetworkId{};

/// Relevance of the NetworkObject.
/// If 0, NetworkObject is irrelevant for the client.
/// If not 0, indicates how often the NetworkObject receives unreliable updates:
/// - 1: send every update;
/// - 2: send every second update;
/// - 3: send every third update;
/// - etc
enum class NetworkObjectRelevance : unsigned char
{
    Irrelevant = 0,
    Normal = 1,
};

}
