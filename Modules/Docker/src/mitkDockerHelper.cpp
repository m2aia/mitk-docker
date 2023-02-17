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

#include <Poco/Pipe.h>
#include <Poco/Process.h>

#include <mitkDockerHelper.h>
#include <mitkHelperUtils.h>
#include <mitkIOUtil.h>

#include <dirent.h>
#include <iostream>

bool mitk::DockerHelper::CheckDocker() {
  std::string command = "docker ps";

  Poco::Process p;
  Poco::Pipe pipe;
  auto handle = p.launch("docker", {"ps"}, 0, &pipe, 0);
  auto code = handle.wait();

  if (code == 0)
    return true;
  else {
    MITK_INFO << "Docker is not installed on this system." << std::endl;
    return false;
  }
}

std::string mitk::DockerHelper::GetFilePath(std::string path) {
  return itksys::SystemTools::ConvertToOutputPath(m_WorkingDirectory + "/" + path);
}

void mitk::DockerHelper::AddApplicationArgument(std::string argumentName,
                                                std::string what) {
  m_AdditionalApplicationArguments.push_back(argumentName);
  if (!what.empty())
    m_AdditionalApplicationArguments.push_back(what);
}

void mitk::DockerHelper::AddRunArgument(std::string targetArgument,
                                        std::string what) {
  m_AdditionalRunArguments.push_back(targetArgument);
  if (!what.empty())
    m_AdditionalRunArguments.push_back(what);
}

mitk::DockerHelper::DataInfo *
mitk::DockerHelper::AddData(mitk::BaseData::Pointer data,
                            std::string targetArgument,
                            std::string nameWithExtension) {
  auto res = m_Data.try_emplace(data, targetArgument, nameWithExtension);
  if (res.second) {
    return &(res.first->second);
  } else { // !res.second
    mitkThrow()
        << "Warning! Overriding an already inserted argument is not allowed!";
  }
}

mitk::DockerHelper::OutputInfo * 
mitk::DockerHelper::AddAutoLoadOutput(std::string targetArgument,
                                      std::string nameWithExtension,
                                      bool isFlagOnly) {
  auto res = m_Outputs.try_emplace(targetArgument, nameWithExtension, AUTOLOAD, isFlagOnly);
  if (res.second) {
    return &(res.first->second);
  } else { // !res.second
    mitkThrow()
        << "Warning! Overriding an already inserted argument is not allowed!";
  }
}

mitk::DockerHelper::OutputInfo * 
mitk::DockerHelper::AddLoadLaterOutput(std::string targetArgument,
                                       std::string nameWithExtension,
                                       bool isFlagOnly) {
  auto res = m_Outputs.try_emplace(targetArgument, nameWithExtension, !AUTOLOAD, isFlagOnly);
  if (res.second) {
    return &(res.first->second);
  } else { // !res.second
    mitkThrow()
        << "Warning! Overriding an already inserted argument is not allowed!";
  }
}

mitk::DockerHelper::OutputInfo *
mitk::DockerHelper::AddAutoLoadOutputFolder(
    std::string targetArgument, std::string directory,
    std::vector<std::string> expectedFilenames) {
  auto res = m_Outputs.try_emplace(targetArgument, directory, AUTOLOAD, !FLAG_ONLY, DIRECTORY,
                                   expectedFilenames);
  if (res.second) {
    return &(res.first->second);
  } else { // !res.second
    mitkThrow()
        << "Warning! Overriding an already inserted argument is not allowed!";
  }
}


void mitk::DockerHelper::AddAutoLoadFileFormWorkingDirectory(std::string expectedFilename) {
  m_AutoLoadFilenamesFromWorkingDirectory.push_back(expectedFilename);
}

void mitk::DockerHelper::EnableAutoRemoveImage(bool value) {
  m_AutoRemoveImage = value;
}

void mitk::DockerHelper::EnableAutoRemoveContainer(bool value) {
  m_AutoRemoveContainer = value;
}

void mitk::DockerHelper::ExecuteDockerCommand(
    std::string command, const std::vector<std::string> &args) {
  Poco::Process::Args processArgs;
  processArgs.push_back(command);
  for (auto a : args) {
    processArgs.push_back(a);
  }

  // launch the process
  Poco::Process p;
  int code;
  auto handle = p.launch("docker", processArgs);
  code = handle.wait();

  if (code) {
    mitkThrow() << "docker " << command << " failed with exit code [" << code
                << "]";
  }
}

void mitk::DockerHelper::Run(const std::vector<std::string> &cmdArgs,
                             const std::vector<std::string> &entryPointArgs) {

  std::vector<std::string> args;
  args.insert(args.end(), cmdArgs.begin(), cmdArgs.end());

  if (m_AutoRemoveContainer &&
      std::find(args.begin(), args.end(), "--rm") == args.end())
    args.push_back("--rm");

  args.push_back(m_ImageName);
  args.insert(args.end(), entryPointArgs.begin(), entryPointArgs.end());

  ExecuteDockerCommand("run", args);
}

void mitk::DockerHelper::RemoveImage(std::vector<std::string> args) {
  if (std::find(args.begin(), args.end(), "-f") == args.end())
    args.insert(args.begin(), "-f");
  ExecuteDockerCommand("rmi", args);
}

mitk::DockerHelper::MappingsAndArguments
mitk::DockerHelper::DataToDockerRunArguments() const {
  using namespace itksys;
  using namespace std;
  MappingsAndArguments ma;

  // working directory name of the host system is used as mounting point name in
  // the container this folder is used as persistent communication bridge
  // between container and host and vice versa.
  const auto dirPathContainer =
      m_WorkingDirectory.substr(m_WorkingDirectory.rfind('/'));
  // add this as not "read only" mapping
  ma.docker.push_back("-v");
  ma.docker.push_back(m_WorkingDirectory + ":" + dirPathContainer);

  // add custom run arguments
  ma.docker.insert(end(ma.docker), begin(m_AdditionalRunArguments),
                   end(m_AdditionalRunArguments));

  ma.application.insert(end(ma.application),
                        begin(m_AdditionalApplicationArguments),
                        end(m_AdditionalApplicationArguments));

  // Add input data
  for (const auto kv : m_Data) {
    const auto data = kv.first;
    const auto dataInfo = kv.second;
    std::string filePath;
    data->GetPropertyList()->GetStringProperty("MITK.IO.reader.inputlocation",
                                               filePath);
    const bool hasSameExtension =
        SystemTools::GetFilenameExtension(dataInfo.nameWithExtension) ==
        SystemTools::GetFilenameExtension(filePath);

    if (filePath.empty() || !hasSameExtension) {
      const auto filePathHost = SystemTools::ConvertToOutputPath(
          m_WorkingDirectory + "/" + dataInfo.nameWithExtension);
      mitk::IOUtil::Save(data, filePathHost);
      const auto filePathContainer = SystemTools::ConvertToOutputPath(
          dirPathContainer + "/" + dataInfo.nameWithExtension);
      ma.application.push_back(dataInfo.targetArgument);
      ma.application.push_back(filePathContainer);
    } else {
      // Actual this dir is never used on the host system, but the name is
      // involved on in the container as mounting point
      const auto phantomDirPathHost = mitk::HelperUtils::TempDirPath();
      // Extract the directory name
      const auto dirPathContainer =
          phantomDirPathHost.substr(phantomDirPathHost.rfind('/'));
      // so delete it on the host again
      SystemTools::RemoveADirectory(phantomDirPathHost);

      // find the files location (parent dir) on the host system
      const auto dirPathHost = SystemTools::GetParentDirectory(filePath);

      // mount this parent dir as read only into the container
      ma.docker.push_back("-v");
      ma.docker.push_back(dirPathHost + ":" + dirPathContainer + ":ro");

      auto fileName = SystemTools::GetFilenameWithoutExtension(filePath);
      auto proposedExtension =
          SystemTools::GetFilenameExtension(dataInfo.nameWithExtension);

      const auto filePathContainer = SystemTools::ConvertToOutputPath(
          dirPathContainer + "/" + fileName + proposedExtension);
      ma.application.push_back(dataInfo.targetArgument);
      ma.application.push_back(filePathContainer);
    }
  }

  for (const auto kv : m_Outputs) {
    const auto argumentName = kv.first;
    const auto outputInfo = kv.second;

    // no directory
    if (!outputInfo.isDirectory) {

      const auto filePathContainer = SystemTools::ConvertToOutputPath(
          dirPathContainer + "/" + outputInfo.path);

      ma.application.push_back(argumentName);
      if(!outputInfo.isFlagOnly)
        ma.application.push_back(filePathContainer);

    } else { // directory

      // find within container
      const auto folderPathContainer = SystemTools::ConvertToOutputPath(
          dirPathContainer + "/" + outputInfo.path);

      ma.application.push_back(argumentName);
      if(!outputInfo.isFlagOnly)
        ma.application.push_back(folderPathContainer);

      if (SystemTools::GetFilenameExtension(outputInfo.path) != "") {
        MITK_WARN << "Directory path [" << outputInfo.path << "] contains a dot";
      }

      // create directory on host
      const auto folderPathHost = SystemTools::ConvertToOutputPath(
          m_WorkingDirectory + "/" + outputInfo.path);
      SystemTools::MakeDirectory(folderPathHost);
    }
  }

  return ma;
}

void mitk::DockerHelper::LoadResults() {

  using namespace itksys;
  // load all files in working directory  
  if(!m_AutoLoadFilenamesFromWorkingDirectory.empty()){
    for (auto filename : m_AutoLoadFilenamesFromWorkingDirectory) {
      const auto fileInFolderPathHost = SystemTools::ConvertToOutputPath(
          m_WorkingDirectory + "/" + filename);
      if(SystemTools::FileExists(fileInFolderPathHost)){
        auto data = mitk::IOUtil::Load(fileInFolderPathHost);
        m_OutputData.insert(m_OutputData.end(), data.begin(), data.end());
        MITK_INFO << "Loaded [Working Directory]: " << fileInFolderPathHost;
      }
    }
  }


  for (const auto kv : m_Outputs) {
    const auto argumentName = kv.first;
    const auto outputInfo = kv.second;
    if (outputInfo.useAutoLoad) {

      // load a file
      if (!outputInfo.isDirectory) {
        const auto filePathHost = SystemTools::ConvertToOutputPath(
            m_WorkingDirectory + "/" + outputInfo.path);
        if(SystemTools::FileExists(filePathHost)){
          auto data = mitk::IOUtil::Load(filePathHost);
          m_OutputData.insert(m_OutputData.end(), data.begin(), data.end());
          MITK_INFO << "Loaded [File]: " << filePathHost << " for argument " << argumentName;
        }else{
          MITK_WARN << "FAILD: Loaded [File]: " << filePathHost << " for argument " << argumentName;
        }
      } else { // load all files in directory

        for (auto filename : outputInfo.directoryFileNames) {
          const auto fileInFolderPathHost = SystemTools::ConvertToOutputPath(
              m_WorkingDirectory + "/" + outputInfo.path + "/" + filename);
          if(SystemTools::FileExists(fileInFolderPathHost)){
            auto data = mitk::IOUtil::Load(fileInFolderPathHost);
            m_OutputData.insert(m_OutputData.end(), data.begin(), data.end());
            MITK_INFO << "Loaded [Directory]: " << fileInFolderPathHost << " for argument " << argumentName;
          }
        }
      }
  

    }
  }
}

std::string mitk::DockerHelper::GetWorkingDirectory() const {
  return m_WorkingDirectory;
}

std::vector<mitk::BaseData::Pointer> mitk::DockerHelper::GetResults() {
  if (!CheckDocker()) {
    mitkThrow() << "No Docker instance found!";
  }

  auto runArgs = DataToDockerRunArguments();

  Run(runArgs.docker, runArgs.application);
  LoadResults();

  MITK_INFO << "Size of the results vector " << m_OutputData.size();

  // autoremove image
  if (m_AutoRemoveImage) {
    RemoveImage({m_ImageName});
  }

  return m_OutputData;
}