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

#pragma once

#include "Sample.h"
#include <RmlUi/Core.h>

namespace Urho3D { class RmlUI; }

/// A helper class used to manage one window.
class DemoWindow : public Object
{
    URHO3D_OBJECT(DemoWindow, Object);
public:
    /// Construct.
    explicit DemoWindow(RmlUI* ui);
    /// Reload window rml and rcss from disk/cache.
    void Reload();
    /// Callback function invoked from rml template.
    void CountClicks(Rml::DataModelHandle modelHandle, Rml::Event& ev, const Rml::VariantList& arguments);
    /// Update model and animate progressbars.
    void OnUpdate(StringHash, VariantMap&);
    /// Process 'CloseWindow' event.
    void OnCloseWindow(StringHash, VariantMap& args);

public:
    /// UI instance which renders this window.
    WeakPtr<RmlUI> ui_;
    /// Sample rml document path.
    ea::string documentPath_{"UI/HelloRmlUI.rml"};
    /// Rml document instance.
    Rml::ElementDocument* document_;
    /// Value of UI slider.
    int sliderValue_ = 0;
    /// Value of button click counter.
    int counter_ = 0;
    /// Value of progressbar progress.
    float progress_ = 0;
    /// Handle of our data model.
    Rml::DataModelHandle model_;
};

/// A RmlUI demonstration.
class HelloRmlUI : public Sample
{
    URHO3D_OBJECT(HelloRmlUI, Sample);
public:
    /// Construct.
    explicit HelloRmlUI(Context* context);
    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Initialize 3D scene.
    void InitScene();
    /// Initialize UI subsystems, backbuffer and cube windows.
    void InitWindow();
    /// Animate cube, handle keys.
    void OnUpdate(StringHash, VariantMap&);

    /// Window which will be rendered into backbuffer.
    SharedPtr<DemoWindow> window_;
    /// Window which will be rendered onto a side of a cube.
    SharedPtr<DemoWindow> window3D_;
};


