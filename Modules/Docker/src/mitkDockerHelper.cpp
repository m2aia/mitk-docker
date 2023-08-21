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

#include <iostream>

#include <boost/format.hpp>


bool mitk::DockerHelper::CheckDocker()
{
  std::string command = "docker ps";

  Poco::Process p;
  Poco::Pipe pipe;
  auto handle = p.launch("docker", {"ps"}, 0, &pipe, 0);
  auto code = handle.wait();

  if (code == 0)
    return true;
  else
  {
    MITK_INFO << "Docker is not installed on this system." << std::endl;
    return false;
  }
}

std::string mitk::DockerHelper::GetFilePath(std::string path)
{
  return (m_WorkingDirectory / path).string();
}

void mitk::DockerHelper::AddApplicationArgument(std::string argumentName,
                                                std::string what)
{
  m_AdditionalApplicationArguments.push_back(argumentName);
  if (!what.empty())
    m_AdditionalApplicationArguments.push_back(what);
}

void mitk::DockerHelper::AddRunArgument(std::string targetArgument,
                                        std::string what)
{
  m_AdditionalRunArguments.push_back(targetArgument);
  if (!what.empty())
    m_AdditionalRunArguments.push_back(what);
}

mitk::DockerHelper::SaveDataInfo *
mitk::DockerHelper::AddAutoSaveData(mitk::BaseData::Pointer data,
                                    std::string targetArgument,
                                    std::string name,
                                    std::string extension)
{
  auto res = m_SaveDataInfo.try_emplace(targetArgument, name, extension, std::vector<mitk::BaseData::Pointer>{data}, AUTOSAVE, SINGLE_FILE);
  if (res.second)
  {
    return &(res.first->second);
  }
  else
  { // !res.second
    mitkThrow()
        << "Warning! Overriding an already inserted argument is not allowed!";
  }
}

mitk::DockerHelper::SaveDataInfo *
mitk::DockerHelper::AddAutoSaveData(std::vector<mitk::BaseData::Pointer> data,
                                    std::string targetArgument,
                                    std::string name,
                                    std::string extension)
{
  auto res = m_SaveDataInfo.try_emplace(targetArgument, name, extension, data, AUTOSAVE, !SINGLE_FILE);
  if (res.second)
  {
    return &(res.first->second);
  }
  else
  { // !res.second
    mitkThrow()
        << "Warning! Overriding an already inserted argument is not allowed!";
  }
}

mitk::DockerHelper::SaveDataInfo *
mitk::DockerHelper::AddSaveLaterData(mitk::BaseData::Pointer data,
                                     std::string targetArgument,
                                     std::string name,
                                     std::string extension)
{
  auto res = m_SaveDataInfo.try_emplace(targetArgument, name, extension, std::vector<mitk::BaseData::Pointer>{data}, !AUTOSAVE, SINGLE_FILE);
  if (res.second)
  {
    return &(res.first->second);
  }
  else
  { // !res.second
    mitkThrow()
        << "Warning! Overriding an already inserted argument is not allowed!";
  }
}

mitk::DockerHelper::LoadDataInfo *
mitk::DockerHelper::AddAutoLoadOutput(std::string targetArgument,
                                      std::string nameWithExtension,
                                      bool isFlagOnly)
{
  m_LoadDataInfo.emplace_back(targetArgument, nameWithExtension, AUTOLOAD,
                              isFlagOnly);
  return &(m_LoadDataInfo.back());
}

mitk::DockerHelper::LoadDataInfo *
mitk::DockerHelper::AddLoadLaterOutput(std::string targetArgument,
                                       std::string nameWithExtension,
                                       bool isFlagOnly)
{
  m_LoadDataInfo.emplace_back(targetArgument, nameWithExtension, !AUTOLOAD,
                              isFlagOnly);
  return &(m_LoadDataInfo.back());
}

mitk::DockerHelper::LoadDataInfo *mitk::DockerHelper::AddAutoLoadOutputFolder(
    std::string targetArgument, std::string directory,
    std::vector<std::string> expectedFilenames)
{
  m_LoadDataInfo.emplace_back(targetArgument, directory, AUTOLOAD,
                              !FLAG_ONLY, DIRECTORY, expectedFilenames);
  return &(m_LoadDataInfo.back());
}

void mitk::DockerHelper::AddAutoLoadFileFormWorkingDirectory(
    std::string expectedFilename)
{
  m_AutoLoadFilenamesFromWorkingDirectory.push_back(expectedFilename);
}

void mitk::DockerHelper::EnableGPUs(bool value)
{
  m_UseGPUs = value;
}

void mitk::DockerHelper::EnableAutoRemoveImage(bool value)
{
  m_AutoRemoveImage = value;
}

void mitk::DockerHelper::EnableAutoRemoveContainer(bool value)
{
  m_AutoRemoveContainer = value;
}

void mitk::DockerHelper::ExecuteDockerCommand(
    std::string command, const std::vector<std::string> &args)
{
  Poco::Process::Args processArgs;
  processArgs.push_back(command);
  for (auto a : args)
  {
    MITK_INFO << a;
    processArgs.push_back(a);
  }

  // launch the process
  Poco::Process p;
  int code;
  auto handle = p.launch("docker", processArgs);
  code = handle.wait();

  if (code)
  {
    mitkThrow() << "docker " << command << " failed with exit code [" << code
                << "]";
  }
}

void mitk::DockerHelper::Run(const std::vector<std::string> &cmdArgs,
                             const std::vector<std::string> &entryPointArgs)
{

  std::vector<std::string> args;
  args.insert(args.end(), cmdArgs.begin(), cmdArgs.end());

  if (m_AutoRemoveContainer &&
      std::find(args.begin(), args.end(), "--rm") == args.end())
    args.push_back("--rm");

  if (m_UseGPUs &&
      std::find(args.begin(), args.end(), "--gpus") == args.end())
  {
    args.push_back("--gpus");
    args.push_back("all");
  }

  args.push_back(m_ImageName);
  args.insert(args.end(), entryPointArgs.begin(), entryPointArgs.end());

  ExecuteDockerCommand("run", args);
}

void mitk::DockerHelper::RemoveImage(std::vector<std::string> args)
{
  if (std::find(args.begin(), args.end(), "-f") == args.end())
    args.insert(args.begin(), "-f");
  ExecuteDockerCommand("rmi", args);
}

mitk::DockerHelper::MappingsAndArguments
mitk::DockerHelper::DataToDockerRunArguments() const
{
  using namespace itksys;
  using namespace std;
  MappingsAndArguments ma;

  // working directory name of the host system is used as mounting point name in
  // the container this folder is used as persistent communication bridge
  // between container and host and vice versa.
  const auto dirPathContainer = m_WorkingDirectory.filename();
  // add this as not "read only" mapping
  ma.docker.push_back("-v");
  ma.docker.push_back(m_WorkingDirectory.string() + ":/" + dirPathContainer.string());

  // add custom run arguments
  ma.docker.insert(end(ma.docker), begin(m_AdditionalRunArguments),
                   end(m_AdditionalRunArguments));

  ma.application.insert(end(ma.application),
                        begin(m_AdditionalApplicationArguments),
                        end(m_AdditionalApplicationArguments));

  // Add input data
  for (auto &kv : m_SaveDataInfo)
  {
    std::string targetArgument = kv.first;
    SaveDataInfo &dataInfo = kv.second;
    const auto dataVector = dataInfo.data;

    // save multiple objects to a folder given by dataInfo.nameWithExtension
    if (!dataInfo.isSingleFile)
    {
     
      { // create data folder
        const auto splitPos = dataInfo.name.find("/");
        const auto folderName = dataInfo.name.substr(0, splitPos);
        const auto dirPathHost = m_WorkingDirectory / folderName;
        boost::filesystem::create_directory(dirPathHost);
      }
      
      int i = 0;
      for(auto data: dataVector){
        const auto fileRelativeFilePath = boost::filesystem::path((boost::format(dataInfo.name) % i).str() + dataInfo.extension);

        const auto filePathHost = m_WorkingDirectory / fileRelativeFilePath;
        MITK_INFO << "Write " << filePathHost.string();
        mitk::IOUtil::Save(data, filePathHost.string());
        ++i;
      }

      // the target folder is passed as command line argument instead of a file name
      // this has to be handled of the target script
      const auto splitPos = dataInfo.name.find("/");
      const auto folderName = dataInfo.name.substr(0, splitPos);
      ma.application.push_back(targetArgument);
      ma.application.push_back("/" + (dirPathContainer / folderName).string());


    }
    // save/link a single data object
    else 
    {
      auto data = dataVector.front();

      std::string filePath = "";
      data->GetPropertyList()->GetStringProperty("MITK.IO.reader.inputlocation",
                                                 filePath);
      const bool hasSameExtension =
          dataInfo.extension == SystemTools::GetFilenameExtension(filePath);

      auto filePathHost = m_WorkingDirectory / (dataInfo.name + dataInfo.extension);
      dataInfo.manualSavePath = filePathHost;

      if (filePath.empty() || !hasSameExtension)
      { // file not on disk or different extension
        // MITK_INFO << filePathHost.string() << " " << data;
        mitk::IOUtil::Save(data, filePathHost.string());
        const auto filePathContainer = dirPathContainer / (dataInfo.name + dataInfo.extension);
        ma.application.push_back(targetArgument);
        ma.application.push_back("/" + filePathContainer.string());
      }
      else
      {
        // Actual this dir is never used on the host system, but the name is
        // involved on in the container as mounting point
        const auto phantomDirPathHost = boost::filesystem::path(mitk::HelperUtils::TempDirPath());
        // Extract the directory name
        const auto dirPathContainer = phantomDirPathHost.filename();
        // so delete it on the host again
        boost::filesystem::remove(phantomDirPathHost);

        // find the files location (parent dir) on the host system
        const auto dirPathHost = boost::filesystem::path(filePath).remove_filename();

        // mount this parent dir as read only into the container
        ma.docker.push_back("-v");
        ma.docker.push_back(dirPathHost.string() + ":/" + dirPathContainer.string() + ":ro");

        auto fileName = boost::filesystem::path(filePath).filename();

        const auto filePathContainer = dirPathContainer / fileName.replace_extension(dataInfo.extension);
        ma.application.push_back(targetArgument);
        ma.application.push_back("/" + filePathContainer.string());
      }
    }
  }

  for (const auto &outputInfo : m_LoadDataInfo)
  {
    const auto argumentName = outputInfo.arg;

    // no directory
    if (!outputInfo.isDirectory)
    {
      const auto filePathContainer = dirPathContainer / outputInfo.path;

      ma.application.push_back(argumentName);
      if (!outputInfo.isFlagOnly)
        ma.application.push_back("/" + filePathContainer.string());
    }
    else
    { // directory

      // find within container
      const auto folderPathContainer = dirPathContainer / outputInfo.path;

      ma.application.push_back(argumentName);
      if (!outputInfo.isFlagOnly)
        ma.application.push_back("/" + folderPathContainer.string());

      if (SystemTools::GetFilenameExtension(outputInfo.path) != "")
      {
        MITK_WARN << "Directory path [" << outputInfo.path
                  << "] contains a dot";
      }

      // create directory on host
      const auto folderPathHost = m_WorkingDirectory / outputInfo.path;
      boost::filesystem::create_directories(folderPathHost);
    }
  }

  return ma;
}

void mitk::DockerHelper::LoadResults()
{

  using namespace itksys;

  // load files from working directory
  for (auto filename : m_AutoLoadFilenamesFromWorkingDirectory)
  {
    const auto fileInFolderPathHost = m_WorkingDirectory / filename;
    if (boost::filesystem::exists(fileInFolderPathHost))
    {
      auto data = mitk::IOUtil::Load(fileInFolderPathHost.string());
      m_OutputData.insert(m_OutputData.end(), data.begin(), data.end());
      MITK_INFO << "Loaded [Working Directory]: " << fileInFolderPathHost;
    }
  }

  for (const auto &outputInfo : m_LoadDataInfo)
  {
    const auto argumentName = outputInfo.arg;
    if (outputInfo.useAutoLoad)
    {

      // load a file
      if (!outputInfo.isDirectory)
      {
        const auto filePathHost = m_WorkingDirectory / outputInfo.path;
        if (boost::filesystem::exists(filePathHost))
        {
          auto data = mitk::IOUtil::Load(filePathHost.string());
          m_OutputData.insert(m_OutputData.end(), data.begin(), data.end());
          MITK_INFO << "Loaded [File]: " << filePathHost << " for argument "
                    << argumentName;
        }
        else
        {
          MITK_WARN << "FAILD: Loaded [File]: " << filePathHost
                    << " for argument " << argumentName;
        }
      }
      else
      { // load all files in directory

        for (auto filename : outputInfo.directoryFileNames)
        {
          const auto fileInFolderPathHost = m_WorkingDirectory / outputInfo.path / filename;
          if (boost::filesystem::exists(fileInFolderPathHost))
          {
            auto data = mitk::IOUtil::Load(fileInFolderPathHost.string());
            m_OutputData.insert(m_OutputData.end(), data.begin(), data.end());
            MITK_INFO << "Loaded [Directory]: " << fileInFolderPathHost
                      << " for argument " << argumentName;
          }
        }
      }
    }
  }
}

boost::filesystem::path mitk::DockerHelper::GetWorkingDirectory() const
{
  return m_WorkingDirectory;
}

std::vector<mitk::BaseData::Pointer> mitk::DockerHelper::GetResults()
{
  if (!CheckDocker())
  {
    mitkThrow() << "No Docker instance found!";
  }

  auto runArgs = DataToDockerRunArguments();

  Run(runArgs.docker, runArgs.application);
  LoadResults();

  MITK_INFO << "Size of the results vector " << m_OutputData.size();

  // autoremove image
  if (m_AutoRemoveImage)
  {
    RemoveImage({m_ImageName});
  }

  return m_OutputData;
}