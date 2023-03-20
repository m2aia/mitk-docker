/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <mitkDockerHelper.h>
#include <cppunit/TestFixture.h>
#include <mitkImageCast.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkTestFixture.h>
#include <mitkTestingMacros.h>

//#include <boost/algorithm/string.hpp>

class DockerTestSuite : public mitk::TestFixture
{
  CPPUNIT_TEST_SUITE(DockerTestSuite);
  MITK_TEST(FindDocker);
  MITK_TEST(RunHelloWorldContainer_NoThrow);
  MITK_TEST(RunSparsePCA_NoThrow);
  CPPUNIT_TEST_SUITE_END();

public:
  void FindDocker(){
    CPPUNIT_ASSERT(mitk::DockerHelper::CheckDocker());
  }

  void RunHelloWorldContainer_NoThrow(){
    mitk::DockerHelper helper("hello-world");
    // helper.EnableAutoRemoveImage(true);
    CPPUNIT_ASSERT_NO_THROW(helper.GetResults());
  }

  void RunSparsePCA_NoThrow(){

    
    auto data = mitk::Image::New();
    const char * filePath ="/home/jtfc/HS/M2aia/Sources/m2Extensions/sparse_pca/testData.imzML";
    data->GetPropertyList()->SetStringProperty("MITK.IO.reader.inputlocation", filePath);

    mitk::DockerHelper helper("sparse_pca");
    // helper.AddAutoSaveData(data.front(), "--test", "unique_name_if_needed.nrrd");
    helper.AddAutoSaveData(data, "--imzml", "default.imzML");
    helper.AddLoadLaterOutput("--csv", "pca_data.csv");
    helper.AddAutoLoadOutput("--image", "pca_data.nrrd");
    helper.GetResults();
  }
  
};

MITK_TEST_SUITE_REGISTRATION(Docker)
