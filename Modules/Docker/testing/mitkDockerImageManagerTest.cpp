/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <mitkDockerImageManager.h>
#include <mitkCoreServices.h>
#include <mitkIPreferences.h>
#include <mitkIPreferencesService.h>
#include <mitkTestFixture.h>
#include <mitkTestingMacros.h>

class mitkDockerImageManagerTestSuite : public mitk::TestFixture
{
  CPPUNIT_TEST_SUITE(mitkDockerImageManagerTestSuite);
  
  // Basic functionality tests
  MITK_TEST(TestConstruction);
  MITK_TEST(TestAddImage);
  MITK_TEST(TestAddInvalidImage);
  MITK_TEST(TestAddDuplicateImage);
  MITK_TEST(TestRemoveImage);
  MITK_TEST(TestRemoveNonexistentImage);
  MITK_TEST(TestUpdateImageTag);
  MITK_TEST(TestUpdateNonexistentImageTag);
  MITK_TEST(TestGetImage);
  MITK_TEST(TestGetNonexistentImage);
  MITK_TEST(TestHasImage);
  MITK_TEST(TestGetImages);
  MITK_TEST(TestCount);
  MITK_TEST(TestClear);
  
  // Persistence tests
  MITK_TEST(TestPersistence);
  MITK_TEST(TestLoadFromPreferences);
  MITK_TEST(TestSaveToPreferences);
  
  // DockerImage struct tests
  MITK_TEST(TestDockerImageConstruction);
  MITK_TEST(TestDockerImageValidation);
  MITK_TEST(TestDockerImageFullName);
  
  CPPUNIT_TEST_SUITE_END();

private:
  mitk::DockerImageManager *m_Manager;
  
public:
  void setUp() override
  {
    // Create a fresh manager for each test
    m_Manager = new mitk::DockerImageManager();
    m_Manager->Clear(); // Ensure clean state
  }

  void tearDown() override
  {
    // Clean up
    m_Manager->Clear();
    delete m_Manager;
    m_Manager = nullptr;
  }

  // ============================================================================
  // Basic Functionality Tests
  // ============================================================================
  
  void TestConstruction()
  {
    mitk::DockerImageManager manager;
    CPPUNIT_ASSERT_EQUAL(0, manager.Count());
  }

  void TestAddImage()
  {
    mitk::DockerImageManager::DockerImage image("ghcr.io/m2aia/umap", "latest");
    bool result = m_Manager->AddImage(image);
    
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT_EQUAL(1, m_Manager->Count());
    CPPUNIT_ASSERT(m_Manager->HasImage("ghcr.io/m2aia/umap"));
  }

  void TestAddInvalidImage()
  {
    mitk::DockerImageManager::DockerImage invalidImage("", "latest");
    bool result = m_Manager->AddImage(invalidImage);
    
    CPPUNIT_ASSERT(!result);
    CPPUNIT_ASSERT_EQUAL(0, m_Manager->Count());
  }

  void TestAddDuplicateImage()
  {
    mitk::DockerImageManager::DockerImage image1("ghcr.io/m2aia/umap", "latest");
    mitk::DockerImageManager::DockerImage image2("ghcr.io/m2aia/umap", "v2.0");
    
    bool result1 = m_Manager->AddImage(image1);
    bool result2 = m_Manager->AddImage(image2);
    
    CPPUNIT_ASSERT(result1);
    CPPUNIT_ASSERT(!result2); // Should fail - duplicate image name
    CPPUNIT_ASSERT_EQUAL(1, m_Manager->Count());
  }

  void TestRemoveImage()
  {
    mitk::DockerImageManager::DockerImage image("ghcr.io/m2aia/umap", "latest");
    m_Manager->AddImage(image);
    
    bool result = m_Manager->RemoveImage("ghcr.io/m2aia/umap");
    
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT_EQUAL(0, m_Manager->Count());
    CPPUNIT_ASSERT(!m_Manager->HasImage("ghcr.io/m2aia/umap"));
  }

  void TestRemoveNonexistentImage()
  {
    bool result = m_Manager->RemoveImage("nonexistent/image");
    
    CPPUNIT_ASSERT(!result);
    CPPUNIT_ASSERT_EQUAL(0, m_Manager->Count());
  }

  void TestUpdateImageTag()
  {
    mitk::DockerImageManager::DockerImage image("ghcr.io/m2aia/umap", "latest");
    m_Manager->AddImage(image);
    
    bool result = m_Manager->UpdateImageTag("ghcr.io/m2aia/umap", "v2.0");
    
    CPPUNIT_ASSERT(result);
    
    auto retrievedImage = m_Manager->GetImage("ghcr.io/m2aia/umap");
    CPPUNIT_ASSERT_EQUAL(std::string("v2.0"), retrievedImage.tag);
  }

  void TestUpdateNonexistentImageTag()
  {
    bool result = m_Manager->UpdateImageTag("nonexistent/image", "v1.0");
    
    CPPUNIT_ASSERT(!result);
  }

  void TestGetImage()
  {
    mitk::DockerImageManager::DockerImage image("ghcr.io/m2aia/umap", "v1.5");
    m_Manager->AddImage(image);
    
    auto retrievedImage = m_Manager->GetImage("ghcr.io/m2aia/umap");
    
    CPPUNIT_ASSERT(retrievedImage.IsValid());
    CPPUNIT_ASSERT_EQUAL(std::string("ghcr.io/m2aia/umap"), retrievedImage.imageName);
    CPPUNIT_ASSERT_EQUAL(std::string("v1.5"), retrievedImage.tag);
  }

  void TestGetNonexistentImage()
  {
    auto retrievedImage = m_Manager->GetImage("nonexistent/image");
    
    CPPUNIT_ASSERT(!retrievedImage.IsValid());
    CPPUNIT_ASSERT(retrievedImage.imageName.empty());
  }

  void TestHasImage()
  {
    mitk::DockerImageManager::DockerImage image("ghcr.io/m2aia/umap", "latest");
    m_Manager->AddImage(image);
    
    CPPUNIT_ASSERT(m_Manager->HasImage("ghcr.io/m2aia/umap"));
    CPPUNIT_ASSERT(!m_Manager->HasImage("nonexistent/image"));
  }

  void TestGetImages()
  {
    mitk::DockerImageManager::DockerImage image1("ghcr.io/m2aia/umap", "latest");
    mitk::DockerImageManager::DockerImage image2("ghcr.io/m2aia/molecular", "v2.0");
    mitk::DockerImageManager::DockerImage image3("tensorflow/tensorflow", "2.15.0");
    
    m_Manager->AddImage(image1);
    m_Manager->AddImage(image2);
    m_Manager->AddImage(image3);
    
    auto images = m_Manager->GetImages();
    
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(3), images.size());
  }

  void TestCount()
  {
    CPPUNIT_ASSERT_EQUAL(0, m_Manager->Count());
    
    mitk::DockerImageManager::DockerImage image1("image1", "latest");
    mitk::DockerImageManager::DockerImage image2("image2", "latest");
    
    m_Manager->AddImage(image1);
    CPPUNIT_ASSERT_EQUAL(1, m_Manager->Count());
    
    m_Manager->AddImage(image2);
    CPPUNIT_ASSERT_EQUAL(2, m_Manager->Count());
    
    m_Manager->RemoveImage("image1");
    CPPUNIT_ASSERT_EQUAL(1, m_Manager->Count());
  }

  void TestClear()
  {
    mitk::DockerImageManager::DockerImage image1("image1", "latest");
    mitk::DockerImageManager::DockerImage image2("image2", "latest");
    
    m_Manager->AddImage(image1);
    m_Manager->AddImage(image2);
    CPPUNIT_ASSERT_EQUAL(2, m_Manager->Count());
    
    m_Manager->Clear();
    CPPUNIT_ASSERT_EQUAL(0, m_Manager->Count());
  }

  // ============================================================================
  // Persistence Tests
  // ============================================================================
  
  void TestPersistence()
  {
    // Add some images
    mitk::DockerImageManager::DockerImage image1("ghcr.io/m2aia/umap", "v1.0");
    mitk::DockerImageManager::DockerImage image2("ghcr.io/m2aia/molecular", "latest");
    
    m_Manager->AddImage(image1);
    m_Manager->AddImage(image2);
    m_Manager->SaveToPreferences();
    
    // Create a new manager and load from preferences
    mitk::DockerImageManager newManager;
    newManager.LoadFromPreferences();
    
    // Verify the loaded images
    CPPUNIT_ASSERT_EQUAL(2, newManager.Count());
    CPPUNIT_ASSERT(newManager.HasImage("ghcr.io/m2aia/umap"));
    CPPUNIT_ASSERT(newManager.HasImage("ghcr.io/m2aia/molecular"));
    
    auto loadedImage1 = newManager.GetImage("ghcr.io/m2aia/umap");
    CPPUNIT_ASSERT_EQUAL(std::string("v1.0"), loadedImage1.tag);
    
    auto loadedImage2 = newManager.GetImage("ghcr.io/m2aia/molecular");
    CPPUNIT_ASSERT_EQUAL(std::string("latest"), loadedImage2.tag);
    
    // Clean up
    newManager.Clear();
  }

  void TestLoadFromPreferences()
  {
    // Add and save images
    mitk::DockerImageManager::DockerImage image("test/image", "v1.0");
    m_Manager->AddImage(image);
    m_Manager->SaveToPreferences();
    
    // Clear and reload
    m_Manager->Clear();
    CPPUNIT_ASSERT_EQUAL(0, m_Manager->Count());
    
    m_Manager->LoadFromPreferences();
    CPPUNIT_ASSERT_EQUAL(1, m_Manager->Count());
    CPPUNIT_ASSERT(m_Manager->HasImage("test/image"));
  }

  void TestSaveToPreferences()
  {
    // This is implicitly tested in TestPersistence, but we can add explicit test
    mitk::DockerImageManager::DockerImage image("save/test", "v2.0");
    m_Manager->AddImage(image);
    
    // SaveToPreferences is called automatically by AddImage
    // Verify by creating new manager
    mitk::DockerImageManager newManager;
    newManager.LoadFromPreferences();
    
    CPPUNIT_ASSERT(newManager.HasImage("save/test"));
    
    // Clean up
    newManager.Clear();
  }

  // ============================================================================
  // DockerImage Struct Tests
  // ============================================================================
  
  void TestDockerImageConstruction()
  {
    // Default constructor
    mitk::DockerImageManager::DockerImage image1;
    CPPUNIT_ASSERT(image1.imageName.empty());
    CPPUNIT_ASSERT_EQUAL(std::string("latest"), image1.tag);
    
    // Constructor with name only
    mitk::DockerImageManager::DockerImage image2("test/image");
    CPPUNIT_ASSERT_EQUAL(std::string("test/image"), image2.imageName);
    CPPUNIT_ASSERT_EQUAL(std::string("latest"), image2.tag);
    
    // Constructor with name and tag
    mitk::DockerImageManager::DockerImage image3("test/image", "v1.0");
    CPPUNIT_ASSERT_EQUAL(std::string("test/image"), image3.imageName);
    CPPUNIT_ASSERT_EQUAL(std::string("v1.0"), image3.tag);
  }

  void TestDockerImageValidation()
  {
    mitk::DockerImageManager::DockerImage validImage("valid/image", "latest");
    CPPUNIT_ASSERT(validImage.IsValid());
    
    mitk::DockerImageManager::DockerImage invalidImage("", "latest");
    CPPUNIT_ASSERT(!invalidImage.IsValid());
    
    mitk::DockerImageManager::DockerImage defaultImage;
    CPPUNIT_ASSERT(!defaultImage.IsValid());
  }

  void TestDockerImageFullName()
  {
    mitk::DockerImageManager::DockerImage image("ghcr.io/m2aia/umap", "v2.0");
    std::string fullName = image.FullImageName();
    
    CPPUNIT_ASSERT_EQUAL(std::string("ghcr.io/m2aia/umap:v2.0"), fullName);
  }
};

MITK_TEST_SUITE_REGISTRATION(mitkDockerImageManager)
