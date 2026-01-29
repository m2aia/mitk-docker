/*===================================================================
MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#pragma once

#include <MitkDockerExports.h>
#include <string>
#include <vector>

namespace mitk
{
  class IPreferences;

  /**
   * @brief Manages Docker images with persistence using JSON preferences
   * 
   * This class handles:
   * - Storing and retrieving Docker image configurations
   * - Serializing/deserializing to JSON for preferences storage
   * - Managing image metadata (name, tags, etc.)
   */
  class MITKDOCKER_EXPORT DockerImageManager
  {
  public:
    struct DockerImage
    {
      std::string imageName;
      std::string tag;
      std::string repository;
      std::string notes;

      DockerImage() : tag("latest") {}
      DockerImage(const std::string &name, const std::string &imageTag = "latest",
                  const std::string &repo = "", const std::string &imageNotes = "")
          : imageName(name), tag(imageTag), repository(repo), notes(imageNotes)
      {
      }

      bool IsValid() const { return !imageName.empty(); }
      std::string FullImageName() const { return imageName + ":" + tag; }
    };

    DockerImageManager();
    ~DockerImageManager();

    /**
     * @brief Add a new Docker image to the managed list
     * @param image The Docker image to add
     * @param isPluginImage If true, saves to plugin storage; otherwise to user storage
     * @return true if added successfully, false if it already exists
     */
    bool AddImage(const DockerImage &image, bool isPluginImage = false);

    /**
     * @brief Remove a Docker image from the managed list
     * @param imageName The name of the image to remove
     * @param isPluginImage If true, removes from plugin storage; otherwise from user storage
     * @return true if removed successfully, false if not found
     */
    bool RemoveImage(const std::string &imageName, bool isPluginImage = false);

    /**
     * @brief Update the tag for an existing image
     * @param imageName The name of the image
     * @param newTag The new tag to set
     * @return true if updated successfully, false if not found
     */
    bool UpdateImageTag(const std::string &imageName, const std::string &newTag);

    /**
     * @brief Get all managed Docker images (both user and plugin)
     * @return Vector of all Docker images
     */
    std::vector<DockerImage> GetImages() const;

    /**
     * @brief Get user-defined Docker images only
     * @return Vector of user Docker images
     */
    std::vector<DockerImage> GetUserImages() const;

    /**
     * @brief Get plugin-registered Docker images only
     * @return Vector of plugin Docker images
     */
    std::vector<DockerImage> GetPluginImages() const;

    /**
     * @brief Get a specific image by name
     * @param imageName The name of the image to find
     * @return The Docker image if found, invalid image otherwise
     */
    DockerImage GetImage(const std::string &imageName) const;

    /**
     * @brief Check if an image exists in the managed list
     * @param imageName The name of the image to check
     * @return true if the image exists
     */
    bool HasImage(const std::string &imageName) const;

    /**
     * @brief Load images from preferences
     */
    void LoadFromPreferences();

    /**
     * @brief Save images to preferences
     */
    void SaveToPreferences();

    /**
     * @brief Clear all managed images
     */
    void Clear();

    /**
     * @brief Get the number of managed images
     * @return The count of images
     */
    int Count() const;

    /**
     * @brief Serialize images to JSON string
     * @param isPluginStorage If true, serializes plugin images; otherwise user images
     * @return JSON string representation
     */
    std::string ToJson(bool isPluginStorage = false) const;

    /**
     * @brief Deserialize images from JSON string
     * @param jsonStr JSON string to parse
     * @param isPluginStorage If true, loads into plugin storage; otherwise user storage
     * @return true if parsing was successful
     */
    bool FromJson(const std::string &jsonStr, bool isPluginStorage = false);

  private:

    /**
     * @brief Get preferences node for storage
     * @return Preferences node or nullptr if not available
     */
    IPreferences *GetPreferences();

    std::vector<DockerImage> m_userImages;
    std::vector<DockerImage> m_pluginImages;
    static const std::string USER_PREFERENCE_KEY;
    static const std::string PLUGIN_PREFERENCE_KEY;
  };

} // namespace mitk
