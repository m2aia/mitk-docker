/*===================================================================
MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

// Additional Attributions: Lorenz Schwab

#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>

#include <MitkDockerExports.h>
#include <mitkBaseData.h>

#include <mitkPointSet.h>
#include <mitkHelperUtils.h>


namespace mitk
{
  /**
   * @brief This class manages docker container
   *
   */
  class MITKDOCKER_EXPORT DockerHelper
  {

   public:

    const bool AUTOLOAD = true;
    const bool DIRECTORY = true;
    const bool FLAG_ONLY = true;

    virtual ~DockerHelper(){}
    DockerHelper(std::string image):m_ImageName(image){
      m_WorkingDirectory = mitk::HelperUtils::TempDirPath();
    }

    struct DataInfo{
      DataInfo(std::string targetArgument,  std::string nameWithExtension)
      : targetArgument(targetArgument),nameWithExtension(nameWithExtension)
      {};
      std::string targetArgument;
      std::string nameWithExtension;
    };

    
    

    struct OutputInfo{
      OutputInfo(std::string path, bool autoLoad = false, bool isFlagOnly = false, bool isDirectory = false, std::vector<std::string> directoryFileNames = {})
      : 
      path(path), 
      useAutoLoad(autoLoad),
      isFlagOnly(isFlagOnly),
      isDirectory(isDirectory),
      directoryFileNames(directoryFileNames)
      {};
      // the directory will be created relative to the working directory
      std::string path;

      // Files are read with mitk::IOUtil if an appropriate reader exist
      bool useAutoLoad;
      

      // path is stored but not used/passed as argument-value for this argument
      // e.g. for the argument flag "--preview", the OutputInfo("preview.png", ...)
      // if isFlagOnly is false, the path is added
      //    "<cmd> --preview preview.png"
      // if isFlagOnly is true, the path is dropped
      //    "<cmd> --preview"
      bool isFlagOnly;

      // path is a directory
      bool isDirectory;
      // list of files expected in the directory
      std::vector<std::string> directoryFileNames;
      // TODO: auto search folder for all loadable data items
      // if the directoryFileNames vector is empty


    };
    
    static bool CheckDocker();
    std::string GetFilePath(std::string path);

    // void SetData(mitk::BaseData::Pointer data, std::string targetArgument, std::string extension);
    DataInfo* AddData(mitk::BaseData::Pointer data, std::string targetArgument, std::string nameWithExtension);
    OutputInfo* AddAutoLoadOutput(std::string targetArgument, std::string nameWithExtension,  bool isFlagOnly=false);
    OutputInfo* AddAutoLoadOutputFolder(std::string targetArgument, std::string directory, std::vector<std::string> expectedFilenames);
    OutputInfo* AddLoadLaterOutput(std::string targetArgument, std::string nameWithExtension, bool isFlagOnly=false);
    void AddAutoLoadFileFormWorkingDirectory(std::string expectedFilename);

    std::vector<mitk::BaseData::Pointer> GetResults();
    void EnableAutoRemoveImage(bool value);
    void EnableAutoRemoveContainer(bool value);
    std::string GetWorkingDirectory() const;
    
    
    void AddApplicationArgument(std::string targetParameter, std::string what = "");
    void AddRunArgument(std::string targetArgument, std::string what = "");
    

  protected:
    

    struct MappingsAndArguments{
      std::vector<std::string> docker;
      std::vector<std::string> application;
    };

    std::string m_ImageName, m_WorkingDirectory;
    bool m_AutoRemoveImage = false;
    bool m_AutoRemoveContainer = false;
    
    

    

    std::map<mitk::BaseData::Pointer, DataInfo> m_Data;
    std::map<std::string, OutputInfo> m_Outputs;
    
    // extra parameters for the embedded application
    std::vector<std::string> m_AdditionalApplicationArguments;
    
    // extra parameters for the docker run command
    std::vector<std::string> m_AdditionalRunArguments;

    std::vector<mitk::BaseData::Pointer> m_OutputData;
    std::vector<std::string> m_AutoLoadFilenamesFromWorkingDirectory;

    // std::map<DiskPath, ContainerPath> m_ReadableData;

    static std::string CreateWorkingDirectory();
    static void RemoveWorkingDirectory(std::string);



    void ExecuteDockerCommand(std::string command, const std::vector<std::string> & args);
    void Run(const std::vector<std::string> &cmdArgs, const std::vector<std::string> &entryPointArgs);
    void RemoveImage(std::vector<std::string> args = {});
    void LoadResults();
    

    MappingsAndArguments DataToDockerRunArguments() const;

 

  };

} // namespace m2