#ifndef FILE_UTILITY_H
#define FILE_UTILITY_H

#pragma once

#include <windows.h>
#include <stdio.h>
#include <imagehlp.h>
#pragma comment(lib, "imagehlp.lib")

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#include <QString>

namespace FileUtility
{
	/**
	 * @breif
	 * @param path
	 * @return 
	 */
	static std::string findFolderPath(const std::string &path)
	{
		size_t pos1;
	 
		pos1 = path.rfind('\\');
		if(pos1 != std::string::npos){
			return path.substr(0, pos1+1);
	        
		}
	 
		pos1 = path.rfind('/');
		if(pos1 != std::string::npos){
			return path.substr(0, pos1+1);
		}
	 
		return "";
	}

	/**
	 * @breif 
	 * @param folderPath
	 * @return
	 */
	static bool mkdir(std::string folderPath)
	{
		PathAddBackslashA(&folderPath[0]);
		std::replace(folderPath.begin(), folderPath.end(), '/', '\\');
		BOOL success = MakeSureDirectoryPathExists(folderPath.c_str());
		return success == TRUE;
	}

	/**
	 * @breif
	 * @param filePath(出力するファイルパス)
	 */
	static bool makeDirIfNotExist(QString filePath)
	{
		QFileInfo fileInfo(filePath);
		QString dirpath = fileInfo.absolutePath();
		std::string filePathStr(dirpath.toLocal8Bit());
		return mkdir(filePathStr);
	}

	/**
	 * @breif
	 * @param filePath
	 * @param debugInfo
	 * @return
	 */
	static bool outputDebugInfo(std::wstring filePath, std::string debugInfo)
	{
		std::ofstream ofs;
		ofs.open(filePath.c_str(), std::ios::out);
		ofs << debugInfo.c_str() << std::endl;
		ofs.close();

		return true;
	}
}

#endif