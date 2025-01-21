/*
  ==============================================================================

    notation.h
    Created: 17 Jan 2025 11:41:28pm
    Author:  a931e

  ==============================================================================
*/

#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <array>
#include <numeric>
#include <mutex>

const bool SHOW_PLOTS = true;

// Pitch range for mapping vertical position to MIDI notes
const std::array<double, 2> PITCH_RANGE = {-1.0, 1.0};  // Normalized range

// Convert pitch range to frequencies
const std::array<double, 2> FREQUENCY_RANGE = {
    440.0 * std::pow(2.0, (PITCH_RANGE[0] - 69.0) / 12.0),
    440.0 * std::pow(2.0, (PITCH_RANGE[1] - 69.0) / 12.0)
};

const int MAX_ANALYSIS_HEIGHT = 200;
const int BINARY_THRESHOLD = 240;

// Function declarations
std::vector<std::vector<cv::Point>> findStrokeContours(const cv::Mat& grayscaleImage);

std::pair<std::vector<double>, std::vector<double>> findStrokeBoundaries(
    const cv::Mat& imageSlice, 
    int sliceWidth, 
    int yOffset, 
    int height
);

struct StrokeParameters {
    std::vector<std::vector<double>> strokeIntensities;
    std::vector<std::vector<double>> strokePitches;
    std::vector<std::vector<double>> strokeDensities;
    std::vector<std::vector<double>> strokeHues;
    std::vector<std::vector<double>> strokeSaturations;
    std::vector<std::vector<double>> strokeValues;
    std::vector<std::vector<double>> xPositions;
};
struct NotationResult {
    StrokeParameters params;
    cv::Mat originalImage;
    std::vector<std::vector<cv::Point>> strokes;
};

NotationResult notationToParameters(const std::string& imagePath);

void testNotationToParameters2();