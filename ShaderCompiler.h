#pragma once

#include <string>
#include <filesystem>
#include <iostream>
#include <cstdlib>

namespace ShaderCompiler {

    // Check if a file exists
    inline bool fileExists(const std::string& path) {
        return std::filesystem::exists(path);
    }

    // Get file modification time
    inline std::filesystem::file_time_type getModTime(const std::string& path) {
        if (fileExists(path)) {
            return std::filesystem::last_write_time(path);
        }
        return std::filesystem::file_time_type::min();
    }

    // Compile a single shader if needed
    inline bool compileShaderIfNeeded(const std::string& sourcePath, const std::string& outputPath) {
        bool needsCompile = false;

        if (!fileExists(outputPath)) {
            std::cout << "[Shader] " << outputPath << " not found, compiling..." << std::endl;
            needsCompile = true;
        }
        else if (fileExists(sourcePath)) {
            auto sourceTime = getModTime(sourcePath);
            auto outputTime = getModTime(outputPath);
            if (sourceTime > outputTime) {
                std::cout << "[Shader] " << sourcePath << " changed, recompiling..." << std::endl;
                needsCompile = true;
            }
        }

        if (needsCompile) {
            if (!fileExists(sourcePath)) {
                std::cerr << "[Shader] ERROR: Source file not found: " << sourcePath << std::endl;
                return false;
            }

            // Make sure output directory exists
            std::filesystem::path outPath(outputPath);
            std::filesystem::create_directories(outPath.parent_path());

            // Build glslc command
            std::string command = "glslc \"" + sourcePath + "\" -o \"" + outputPath + "\"";

            std::cout << "[Shader] Running: " << command << std::endl;
            int result = std::system(command.c_str());

            if (result != 0) {
                std::cerr << "[Shader] ERROR: Failed to compile " << sourcePath << std::endl;
                return false;
            }

            std::cout << "[Shader] Compiled: " << outputPath << std::endl;
        }

        return true;
    }

    // Compile all required shaders - call this at app startup
    inline bool compileAllShaders() {
        std::cout << "[Shader] Checking shaders..." << std::endl;

        bool success = true;

        // Mesh shaders
        success &= compileShaderIfNeeded("shaders/workbench.vert", "shaders/compiled/workbench.vert.spv");
        success &= compileShaderIfNeeded("shaders/workbench.frag", "shaders/compiled/workbench.frag.spv");

        // Grid shaders
        success &= compileShaderIfNeeded("shaders/grid.vert", "shaders/compiled/grid.vert.spv");
        success &= compileShaderIfNeeded("shaders/grid.frag", "shaders/compiled/grid.frag.spv");

        // UI shaders
        success &= compileShaderIfNeeded("shaders/ui.vert", "shaders/compiled/ui.vert.spv");
        success &= compileShaderIfNeeded("shaders/ui.frag", "shaders/compiled/ui.frag.spv");

        if (success) {
            std::cout << "[Shader] All shaders ready" << std::endl;
        }
        else {
            std::cerr << "[Shader] ERROR: Some shaders failed to compile!" << std::endl;
        }

        return success;
    }

} // namespace ShaderCompiler