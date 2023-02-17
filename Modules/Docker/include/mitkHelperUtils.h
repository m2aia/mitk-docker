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

#include <itksys/System.h>
#include <itksys/SystemTools.hxx>

#include <itkImageRegionIterator.h>

#include <mitkIOUtil.h>
#include <mitkImageCast.h>

namespace mitk
{
  namespace HelperUtils
  {
    std::string TempDirPath() {
      auto path = mitk::IOUtil::CreateTemporaryDirectory("m2_XXXXXX");
      return itksys::SystemTools::ConvertToOutputPath(path);
    }

    std::string FilePath(std::string path, std::string ext) {
      return mitk::IOUtil::CreateTemporaryFile("XXXXXX", path) + ext;
    }
    
    /**
     * @brief CreatePath can be used to normalize and join path arguments
     * Example call:
     * CreatPath({"this/is/a/directory/path/with/or/without/a/slash/","/","test"});
     * @param args is a vector of strings
     * @return std::string
     */
    std::string JoinPath(std::vector<std::string> &&args)
    {
      return itksys::SystemTools::ConvertToOutputPath(itksys::SystemTools::JoinPath(args));
    }


    mitk::Image::Pointer GetVectorImage3D(std::array<unsigned int,3> dimensions, unsigned int components ){
      auto vectorImage = itk::VectorImage<double, 3>::New();
      vectorImage->SetVectorLength(components);

      itk::Image<double, 3>::IndexType start;
      start[0] = 0;
      start[1] = 0;
      start[2] = 0;

      itk::Image<double, 3>::SizeType size;
      size[0] = dimensions[0];
      size[1] = dimensions[1];
      size[2] = dimensions[2];
      itk::Image<double, 3>::RegionType region;
      region.SetSize(size);
      region.SetIndex(start);
      vectorImage->SetRegions(region);

      vectorImage->Allocate();

      itk::VariableLengthVector<double> initialPixelValues;
      initialPixelValues.SetSize(components);
      initialPixelValues.Fill(0.0);
      itk::ImageRegionIterator<itk::VectorImage<double, 3>> imageIterator(vectorImage,
                                                                            vectorImage->GetRequestedRegion());

      while (!imageIterator.IsAtEnd())
      {
        imageIterator.Set(initialPixelValues);
        ++imageIterator;
      }
      mitk::Image::Pointer result;
      mitk::CastToMitkImage(vectorImage, result);
      return result;
    }




  } // namespace HelperUtils

} // namespace m2