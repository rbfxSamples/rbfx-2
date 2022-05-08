//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../Project/EditorTab.h"

#include <Urho3D/Core/Object.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/XMLFile.h>

namespace Urho3D
{

/// Request to open resource in Editor.
struct OpenResourceRequest
{
    ea::string fileName_;
    ea::string resourceName_;

    SharedPtr<File> file_;
    SharedPtr<XMLFile> xmlFile_;
    SharedPtr<JSONFile> jsonFile_;

    bool IsValid() const { return !fileName_.empty(); }
    explicit operator bool() const { return IsValid(); }

    static OpenResourceRequest FromResourceName(Context* context, const ea::string& resourceName);
};

/// Helper class to keep and restore state of ResourceCache.
class ResourceCacheGuard
{
public:
    explicit ResourceCacheGuard(Context* context);
    ~ResourceCacheGuard();

    const ea::string& GetCoreData() const { return oldCoreData_; }
    const ea::string& GetEditorData() const { return oldEditorData_; }

private:
    Context* context_{};
    StringVector oldResourceDirs_;
    ea::string oldCoreData_;
    ea::string oldEditorData_;
};

/// Main class for all Editor logic related to the project folder.
class ProjectEditor : public Object
{
    URHO3D_OBJECT(ProjectEditor, Object);

public:
    Signal<void()> OnInitialized;

    ProjectEditor(Context* context, const ea::string& projectPath);
    ~ProjectEditor() override;

    /// Plugin API
    /// @{
    void AddTab(SharedPtr<EditorTab> tab);
    template <class T> T* FindTab() const;
    void OpenResource(const OpenResourceRequest& request);
    /// @}

    void UpdateAndRender();
    void Save();

    void ReadIniSettings(const char* entry, const char* line);
    void WriteIniSettings(ImGuiTextBuffer* output);

    /// Return global properties.
    /// @{
    const ea::string& GetCoreDataPath() const { return coreDataPath_; }
    const ea::string& GetDataPath() const { return dataPath_; }
    const ea::string& GetCachePath() const { return cachePath_; }
    /// @}

private:
    void EnsureDirectoryInitialized();
    void InitializeResourceCache();
    void ResetLayout();
    void ApplyPlugins();

    /// Project properties
    /// @{
    const ea::string projectPath_;

    const ea::string coreDataPath_;
    const ea::string cachePath_;
    const ea::string projectJsonPath_;
    const ea::string uiIniPath_;
    ea::string dataPath_;

    const ResourceCacheGuard oldCacheState_;
    /// @}

    bool initialized_{};
    ea::vector<SharedPtr<EditorTab>> tabs_;

    /// UI state
    /// @{
    bool pendingResetLayout_{};
    ImGuiID dockspaceId_{};
    /// @}
};

template <class T>
T* ProjectEditor::FindTab() const
{
    for (EditorTab* tab : tabs_)
    {
        if (auto derivedTab = dynamic_cast<T*>(tab))
            return derivedTab;
    }
    return nullptr;
}

}