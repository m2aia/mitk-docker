/*===================================================================
MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include "mitkDockerImageManager.h"

#include <mitkCoreServices.h>
#include <mitkIPreferences.h>
#include <mitkIPreferencesService.h>
#include <mitkLogMacros.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace mitk
{
  const std::string DockerImageManager::USER_PREFERENCE_KEY = "docker.container.manager.user.images";
  const std::string DockerImageManager::PLUGIN_PREFERENCE_KEY = "docker.container.manager.plugin.images";

  DockerImageManager::DockerImageManager()
  {
    LoadFromPreferences();
  }

  DockerImageManager::~DockerImageManager()
  {
    SaveToPreferences();
  }

  bool DockerImageManager::AddImage(const DockerImage &image, bool isPluginImage)
  {
    if (!image.IsValid())
    {
      MITK_WARN << "Attempted to add invalid Docker image";
      return false;
    }

    auto &targetImages = isPluginImage ? m_pluginImages : m_userImages;
    
    // Check if image already exists in target storage
    for (const auto &existingImage : targetImages)
    {
      if (existingImage.imageName == image.imageName)
      {
        MITK_INFO << "Image already exists: " << image.imageName;
        return false;
      }
    }

    targetImages.push_back(image);
    SaveToPreferences();
    MITK_INFO << "Added Docker image to " << (isPluginImage ? "plugin" : "user") << " storage: " << image.FullImageName();
    return true;
  }

  bool DockerImageManager::RemoveImage(const std::string &imageName, bool isPluginImage)
  {
    auto &targetImages = isPluginImage ? m_pluginImages : m_userImages;
    
    for (auto it = targetImages.begin(); it != targetImages.end(); ++it)
    {
      if (it->imageName == imageName)
      {
        targetImages.erase(it);
        SaveToPreferences();
        MITK_INFO << "Removed Docker image from " << (isPluginImage ? "plugin" : "user") << " storage: " << imageName;
        return true;
      }
    }

    MITK_WARN << "Image not found for removal: " << imageName;
    return false;
  }

  bool DockerImageManager::UpdateImageTag(const std::string &imageName, const std::string &newTag)
  {
    // Try user images first
    for (auto &image : m_userImages)
    {
      if (image.imageName == imageName)
      {
        image.tag = newTag.empty() ? "latest" : newTag;
        SaveToPreferences();
        MITK_INFO << "Updated tag for " << imageName << " to " << image.tag;
        return true;
      }
    }
    
    // Try plugin images
    for (auto &image : m_pluginImages)
    {
      if (image.imageName == imageName)
      {
        image.tag = newTag.empty() ? "latest" : newTag;
        SaveToPreferences();
        MITK_INFO << "Updated tag for " << imageName << " to " << image.tag;
        return true;
      }
    }

    MITK_WARN << "Image not found for tag update: " << imageName;
    return false;
  }

  std::vector<DockerImageManager::DockerImage> DockerImageManager::GetImages() const
  {
    std::vector<DockerImage> allImages;
    allImages.insert(allImages.end(), m_pluginImages.begin(), m_pluginImages.end());
    allImages.insert(allImages.end(), m_userImages.begin(), m_userImages.end());
    return allImages;
  }

  std::vector<DockerImageManager::DockerImage> DockerImageManager::GetUserImages() const
  {
    return m_userImages;
  }

  std::vector<DockerImageManager::DockerImage> DockerImageManager::GetPluginImages() const
  {
    return m_pluginImages;
  }

  DockerImageManager::DockerImage DockerImageManager::GetImage(const std::string &imageName) const
  {
    // Check plugin images first
    for (const auto &image : m_pluginImages)
    {
      if (image.imageName == imageName)
      {
        return image;
      }
    }
    // Then check user images
    for (const auto &image : m_userImages)
    {
      if (image.imageName == imageName)
      {
        return image;
      }
    }
    return DockerImage(); // Return invalid image
  }

  bool DockerImageManager::HasImage(const std::string &imageName) const
  {
    for (const auto &image : m_pluginImages)
    {
      if (image.imageName == imageName)
      {
        return true;
      }
    }
    for (const auto &image : m_userImages)
    {
      if (image.imageName == imageName)
      {
        return true;
      }
    }
    return false;
  }

  void DockerImageManager::LoadFromPreferences()
  {
    auto *preferences = GetPreferences();
    if (!preferences)
    {
      MITK_WARN << "No preferences service available for loading Docker images";
      return;
    }

    // Load user images
    std::string userJsonStr = preferences->Get(USER_PREFERENCE_KEY, "");
    if (!userJsonStr.empty())
    {
      if (FromJson(userJsonStr, false))
      {
        MITK_INFO << "Loaded " << m_userImages.size() << " user Docker image(s) from preferences";
      }
      else
      {
        MITK_ERROR << "Failed to parse user Docker images from preferences";
        m_userImages.clear();
      }
    }
    else
    {
      MITK_INFO << "No persisted user Docker images found in preferences";
      m_userImages.clear();
    }

    // Load plugin images
    std::string pluginJsonStr = preferences->Get(PLUGIN_PREFERENCE_KEY, "");
    if (!pluginJsonStr.empty())
    {
      if (FromJson(pluginJsonStr, true))
      {
        MITK_INFO << "Loaded " << m_pluginImages.size() << " plugin Docker image(s) from preferences";
      }
      else
      {
        MITK_ERROR << "Failed to parse plugin Docker images from preferences";
        m_pluginImages.clear();
      }
    }
    else
    {
      MITK_INFO << "No persisted plugin Docker images found in preferences";
      m_pluginImages.clear();
    }
  }

  void DockerImageManager::SaveToPreferences()
  {
    auto *preferences = GetPreferences();
    if (!preferences)
    {
      MITK_WARN << "No preferences service available for saving Docker images";
      return;
    }

    // Save user images
    std::string userJsonStr = ToJson(false);
    preferences->Put(USER_PREFERENCE_KEY, userJsonStr);
    
    // Save plugin images
    std::string pluginJsonStr = ToJson(true);
    preferences->Put(PLUGIN_PREFERENCE_KEY, pluginJsonStr);
    
    preferences->Flush();

    MITK_INFO << "Saved " << m_userImages.size() << " user and " << m_pluginImages.size() << " plugin Docker image(s) to preferences";
  }

  void DockerImageManager::Clear()
  {
    m_userImages.clear();
    m_pluginImages.clear();
    SaveToPreferences();
    MITK_INFO << "Cleared all Docker images";
  }

  int DockerImageManager::Count() const
  {
    return static_cast<int>(m_userImages.size() + m_pluginImages.size());
  }

  std::string DockerImageManager::ToJson(bool isPluginStorage) const
  {
    json jsonArray = json::array();
    
    const auto &images = isPluginStorage ? m_pluginImages : m_userImages;

    for (const auto &image : images)
    {
      json imageObj;
      imageObj["imageName"] = image.imageName;
      imageObj["tag"] = image.tag;
      imageObj["repository"] = image.repository;
      imageObj["notes"] = image.notes;
      jsonArray.push_back(imageObj);
    }

    return jsonArray.dump();
  }

  bool DockerImageManager::FromJson(const std::string &jsonStr, bool isPluginStorage)
  {
    try
    {
      json doc = json::parse(jsonStr);

      if (!doc.is_array())
      {
        MITK_ERROR << "Invalid JSON format for Docker images";
        return false;
      }

      auto &targetImages = isPluginStorage ? m_pluginImages : m_userImages;
      targetImages.clear();

      for (const auto &value : doc)
      {
        if (!value.is_object())
        {
          MITK_WARN << "Skipping invalid image entry in JSON";
          continue;
        }

        DockerImage image;
        if (value.contains("imageName") && value["imageName"].is_string())
        {
          image.imageName = value["imageName"].get<std::string>();
        }
        if (value.contains("tag") && value["tag"].is_string())
        {
          image.tag = value["tag"].get<std::string>();
        }
        if (value.contains("repository") && value["repository"].is_string())
        {
          image.repository = value["repository"].get<std::string>();
        }
        if (value.contains("notes") && value["notes"].is_string())
        {
          image.notes = value["notes"].get<std::string>();
        }
        else
        {
          image.tag = "latest";
        }

        if (image.IsValid())
        {
          targetImages.push_back(image);
        }
      }

      return true;
    }
    catch (const json::exception &e)
    {
      MITK_ERROR << "JSON parsing error: " << e.what();
      return false;
    }
  }

  IPreferences *DockerImageManager::GetPreferences()
  {
    auto *preferencesService = CoreServices::GetPreferencesService();
    if (!preferencesService)
    {
      return nullptr;
    }

    auto *systemPreferences = preferencesService->GetSystemPreferences();
    if (!systemPreferences)
    {
      return nullptr;
    }

    return systemPreferences->Node("org.mitk.views.docker.containermanager");
  }

} // namespace mitk
