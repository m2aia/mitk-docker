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
#include <boost/filesystem.hpp>


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
    const bool AUTOSAVE = true;
    const bool SINGLE_FILE = true;
    const bool DIRECTORY = true;
    const bool FLAG_ONLY = true;

    virtual ~DockerHelper(){}
    DockerHelper(std::string image):m_ImageName(image){
      m_WorkingDirectory = boost::filesystem::path(mitk::HelperUtils::TempDirPath());
    }

    struct SaveDataInfo{
      SaveDataInfo(const std::string & name, const std::string & extension, const std::vector<mitk::BaseData::Pointer> & data, bool useAutoSave = false, bool isSingleFile = true)
      : name(name), extension(extension), data{data}, useAutoSave(useAutoSave), isSingleFile(isSingleFile)
      {
        if(name.find(".") != std::string::npos)
          mitkThrow() << "Do not use dots in file names";
        
        if(extension.find(".") == std::string::npos)
          mitkThrow() << "Add a dot so that extension follows the pattern '.<extensionname>'";
        
        if(isSingleFile && name.find("%") != std::string::npos)
          mitkThrow() << "boost::format strings not allowed for single file objects";

        if(!isSingleFile && name.find("/") == std::string::npos)
          mitkThrow() << "name requires to have a folder name (i.e. <foldername>/<filename_pattern>)";

        


      };
      // name of the file
      // or boost format pattern (i.e. dirname/filename_%1%) if used for multiple data objects 
      // use only one placeholder for enumeration
      std::string name;

      // target extension with dot (i.e. ".nrrd")
      std::string extension;
      
      // set or single object
      std::vector<mitk::BaseData::Pointer> data;
      
      // Files are written with mitk::IOUtil if an appropriate writer exist
      bool useAutoSave;
      
      // if autosave is false, this path will be provided for later usage
      boost::filesystem::path manualSavePath;

      // if file is part of a set of objects (isSingleFile==false)
      // a directory is created and files are saved according to name pattern
      bool isSingleFile;
    };

    
    

    struct LoadDataInfo{
      LoadDataInfo(std::string arg, std::string path, bool autoLoad = false, bool isFlagOnly = false, bool isDirectory = false, std::vector<std::string> directoryFileNames = {})
      : 
      arg(arg),
      path(path), 
      useAutoLoad(autoLoad),
      isFlagOnly(isFlagOnly),
      isDirectory(isDirectory),
      directoryFileNames(directoryFileNames)
      {};

      // target argument
      std::string arg;

      // the directory will be created relative to the working directory
      std::string path;

      // Files are read with mitk::IOUtil if an appropriate reader exist
      bool useAutoLoad;
      

      // path is stored but not used/passed as argument-value for this argument
      // e.g. for the argument flag "--preview", the LoadDataInfo("preview.png", ...)
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
    
    static bool CanRunDocker();
    std::string GetFilePath(std::string path);

    // void SetData(mitk::BaseData::Pointer data, std::string targetArgument, std::string extension);
    SaveDataInfo* AddAutoSaveData(mitk::BaseData::Pointer data, std::string targetArgument, std::string name, std::string extension);
    SaveDataInfo* AddAutoSaveData(std::vector<mitk::BaseData::Pointer> data, std::string targetArgument, std::string name, std::string extension);
    SaveDataInfo* AddSaveLaterData(mitk::BaseData::Pointer data, std::string targetArgument, std::string name, std::string extension);
    
    LoadDataInfo* AddAutoLoadOutput(std::string targetArgument, std::string nameWithExtension,  bool isFlagOnly=false);
    LoadDataInfo* AddAutoLoadOutputFolder(std::string targetArgument, std::string directory, std::vector<std::string> expectedFilenames);
    LoadDataInfo* AddLoadLaterOutput(std::string targetArgument, std::string nameWithExtension, bool isFlagOnly=false);
    
    void AddAutoLoadFileFormWorkingDirectory(std::string expectedFilename);

    std::vector<mitk::BaseData::Pointer> GetResults();
    void EnableAutoRemoveImage(bool value);
    void EnableGPUs(bool value);
    void EnableAutoRemoveContainer(bool value);
    boost::filesystem::path GetWorkingDirectory() const;
    
    
    void AddApplicationArgument(std::string targetParameter, std::string what = "");
    std::vector<std::string> & GetAdditionalApplicationArguments(){return m_AdditionalApplicationArguments; }
    void AddRunArgument(std::string targetArgument, std::string what = "");
    
    static std::string Replace(std::string f, char a, char b){
      std::replace(f.begin(), f.end(), a, b);
      return f;
    }

  protected:
    

    std::string m_ImageName;
    boost::filesystem::path m_WorkingDirectory;
    bool m_AutoRemoveImage = false;
    bool m_AutoRemoveContainer = false;
    bool m_UseGPUs = false;
    
    
    mutable std::map<std::string, SaveDataInfo> m_SaveDataInfo;
    std::vector<LoadDataInfo> m_LoadDataInfo;
    
    // extra parameters for the embedded application
    std::vector<std::string> m_AdditionalApplicationArguments;
    
    // extra parameters for the docker run command
    std::vector<std::string> m_AdditionalRunArguments;

    std::vector<mitk::BaseData::Pointer> m_OutputData;
    std::vector<std::string> m_AutoLoadFilenamesFromWorkingDirectory;

    std::vector<std::string> m_DockerArguments;
    std::vector<std::string> m_ProgramArguments;

    void ExecuteDockerCommand(std::string command, const std::vector<std::string> & args);
    void GenerateRunData();
    void Run(const std::vector<std::string> &cmdArgs, const std::vector<std::string> &entryPointArgs);
    void RemoveImage(std::vector<std::string> args = {});
    void GenerateSaveDataInfoAndSaveData();
    void GenerateLoadDataInfo();
    void LoadData();



 

  };

} // namespace m2