//convert from relative path into absolute path
#pragma once
#include <filesystem>
#include <string>
#include <iostream>

using namespace std;
using namespace std::filesystem;

static string getInputFilePath(const char* fileName) 
{
    path projectPath = current_path();

    while (projectPath.filename() != "Graphics")
    {
        projectPath = projectPath.parent_path();
    }

    path inputPath = projectPath / "input" / fileName;

    if (exists(inputPath)) 
    {
        return inputPath.string();
    }
    else 
    {
        return "<file does not exist>";
    }
}

static string getOutputFilePath(const char* fileName)
{
    path projectPath = current_path();

    while (projectPath.filename() != "Graphics")
    {
        projectPath = projectPath.parent_path();
    }

    path outputPath = projectPath / "output" / fileName;

    return outputPath.string();
}

static string getTexturePath(const char* fileName)
{
    path projectPath = current_path();

    while (projectPath.filename() != "Graphics")
    {
        projectPath = projectPath.parent_path();
    }

    path inputPath = projectPath / "texture" / fileName;

    if (exists(inputPath))
    {
        return inputPath.string();
    }
    else
    {
        return "<file does not exist>";
    }
}

static string getTriangleMeshPath(const char* fileName)
{
    path projectPath = current_path();

    while (projectPath.filename() != "Graphics")
    {
        projectPath = projectPath.parent_path();
    }

    path inputPath = projectPath / "mesh" / fileName;

    if (exists(inputPath))
    {
        return inputPath.string();
    }
    else
    {
        return "<file does not exist>";
    }
}