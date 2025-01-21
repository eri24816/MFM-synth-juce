/*
  ==============================================================================

    Notation.cpp
    Created: 17 Jan 2025 11:41:55pm
    Author:  a931e

  ==============================================================================
*/

#include "Notation.h"
#include <cmath>
#include <algorithm>  // For std::max_element, std::min_element

std::vector<std::vector<cv::Point>> findStrokeContours(const cv::Mat& grayscaleImage) {
    cv::Mat binaryImage = cv::Mat();
    cv::threshold(grayscaleImage, binaryImage, BINARY_THRESHOLD, 255, cv::THRESH_BINARY_INV);

    // Smooth image to improve contour detection
    cv::Mat blurredImage;
    cv::GaussianBlur(binaryImage, blurredImage, cv::Size(7, 7), 0);

    // Dilate to connect nearby regions
    cv::Mat dilatedImage;
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
    cv::dilate(blurredImage, dilatedImage, kernel);

    // Close gaps in strokes
    cv::Mat closedImage;
    cv::morphologyEx(dilatedImage, closedImage, cv::MORPH_CLOSE, kernel);

    // Find contours
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(closedImage, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);


    return contours;
}

std::pair<std::vector<double>, std::vector<double>> findStrokeBoundaries(
    const cv::Mat& imageSlice,
    int sliceWidth,
    int yOffset,
    int height
) {
    // Find edges using Laplacian
    cv::Mat edges;
    cv::Laplacian(imageSlice, edges, CV_64F);
    cv::convertScaleAbs(edges, edges);

    std::vector<double> topBoundaries(sliceWidth, 0.0);
    std::vector<double> bottomBoundaries(sliceWidth, height);

    // Find edge positions
    for (int x = 0; x < sliceWidth; ++x) {
        for (int y = 0; y < edges.rows; ++y) {
            // uchar (unsigned char) is used here because the Laplacian edge detection 
            // outputs an 8-bit grayscale image where each pixel is 0-255
            if (edges.at<uchar>(y, x) > 0) {
                topBoundaries[x] = std::max(static_cast<double>(y), topBoundaries[x]);
                bottomBoundaries[x] = std::min(static_cast<double>(y), bottomBoundaries[x]);
            }
        }
    }

    // Fill in gaps
    double maxTop = *std::max_element(topBoundaries.begin(), topBoundaries.end());
    double minBottom = *std::min_element(bottomBoundaries.begin(), bottomBoundaries.end());

    for (int i = 0; i < sliceWidth; ++i) {
        if (topBoundaries[i] == 0) topBoundaries[i] = maxTop;
        if (bottomBoundaries[i] == height) bottomBoundaries[i] = minBottom;

        // Add y offset
        topBoundaries[i] += yOffset;
        bottomBoundaries[i] += yOffset;
    }

    return { topBoundaries, bottomBoundaries };
}

void showResults(const cv::Mat& originalImage, const std::vector<std::vector<cv::Point>>& strokes,
    const StrokeParameters& params) {
    // Create a copy for visualization
    cv::Mat visualImage = originalImage.clone();

    // Draw contours
    cv::drawContours(visualImage, strokes, -1, cv::Scalar(0, 255, 0), 2);

    // Show the original image with detected strokes
    cv::namedWindow("Detected Strokes", cv::WINDOW_AUTOSIZE);
    cv::imshow("Detected Strokes", visualImage);

    // Create plots for parameters
    const int plotHeight = 200;
    const int plotWidth = 600;
    cv::Mat plotImage(plotHeight * 3, plotWidth * 2, CV_8UC3, cv::Scalar(255, 255, 255));

    // Colors for different strokes
    std::vector<cv::Scalar> colors = {
        cv::Scalar(0, 0, 255),    // Red
        cv::Scalar(0, 255, 0),    // Green
        cv::Scalar(255, 0, 0),    // Blue
        cv::Scalar(255, 0, 255),  // Magenta
        cv::Scalar(0, 255, 255)   // Yellow
    };

    // Plot parameters
    for (size_t i = 0; i < params.strokePitches.size(); ++i) {
        cv::Scalar color = colors[i % colors.size()];
        const auto& xPos = params.xPositions[i];

        // Plot pitch
        for (size_t j = 1; j < params.strokePitches[i].size(); ++j) {
            cv::line(plotImage,
                cv::Point(xPos[j - 1] * plotWidth, (0.5 - 10 * params.strokePitches[i][j - 1]) * plotHeight),
                cv::Point(xPos[j] * plotWidth, (0.5 - 10 * params.strokePitches[i][j]) * plotHeight),
                color, 2);
        }

        // Plot intensity
        for (size_t j = 1; j < params.strokeIntensities[i].size(); ++j) {
            cv::line(plotImage,
                cv::Point(xPos[j - 1] * plotWidth, plotHeight + (1 - 10 * params.strokeIntensities[i][j - 1]) * plotHeight),
                cv::Point(xPos[j] * plotWidth, plotHeight + (1 - 10 * params.strokeIntensities[i][j]) * plotHeight),
                color, 2);
        }

        // Plot density
        for (size_t j = 1; j < params.strokeDensities[i].size(); ++j) {
            cv::line(plotImage,
                cv::Point(xPos[j - 1] * plotWidth, 2 * plotHeight + (1 - params.strokeDensities[i][j - 1]) * plotHeight),
                cv::Point(xPos[j] * plotWidth, 2 * plotHeight + (1 - params.strokeDensities[i][j]) * plotHeight),
                color, 2);
        }

        // Plot HSV values
        for (size_t j = 1; j < params.strokeHues[i].size(); ++j) {
            // Plot Hue
            cv::line(plotImage,
                cv::Point(plotWidth + xPos[j - 1] * plotWidth, (1 - params.strokeHues[i][j - 1] / 180.0) * plotHeight),
                cv::Point(plotWidth + xPos[j] * plotWidth, (1 - params.strokeHues[i][j] / 180.0) * plotHeight),
                color, 2);

            // Plot Saturation
            cv::line(plotImage,
                cv::Point(plotWidth + xPos[j - 1] * plotWidth, plotHeight + (1 - params.strokeSaturations[i][j - 1]) * plotHeight),
                cv::Point(plotWidth + xPos[j] * plotWidth, plotHeight + (1 - params.strokeSaturations[i][j]) * plotHeight),
                color, 2);

            // Plot Value
            cv::line(plotImage,
                cv::Point(plotWidth + xPos[j - 1] * plotWidth, 2 * plotHeight + (1 - params.strokeValues[i][j - 1]) * plotHeight),
                cv::Point(plotWidth + xPos[j] * plotWidth, 2 * plotHeight + (1 - params.strokeValues[i][j]) * plotHeight),
                color, 2);
        }
    }

    // Update labels
    cv::putText(plotImage, "Pitch", cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
    cv::putText(plotImage, "Intensity", cv::Point(10, plotHeight + 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
    cv::putText(plotImage, "Density", cv::Point(10, 2 * plotHeight + 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
    cv::putText(plotImage, "Hue", cv::Point(plotWidth + 10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
    cv::putText(plotImage, "Saturation", cv::Point(plotWidth + 10, plotHeight + 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
    cv::putText(plotImage, "Value", cv::Point(plotWidth + 10, 2 * plotHeight + 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);

    // Show plots
    cv::namedWindow("Parameter Plots", cv::WINDOW_AUTOSIZE);
    cv::imshow("Parameter Plots", plotImage);

    // Wait for key press
    cv::waitKey(0);
    cv::destroyAllWindows();
}


NotationResult notationToParameters(const std::string& imagePath) {
    // Load and preprocess image
    cv::Mat image = cv::imread(imagePath);
    cv::resize(image, image, cv::Size(1600, 100));
    int maxAnalysisHeight = 100;
    int borderSize = static_cast<int>(round(image.rows / 2.0));

    // Add white borders
    cv::copyMakeBorder(image, image, borderSize, borderSize, 0, 0,
        cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));

    // Convert to different color spaces
    cv::Mat grayscaleImage, hsvImage;
    cv::cvtColor(image, grayscaleImage, cv::COLOR_BGR2GRAY);
    cv::cvtColor(image, hsvImage, cv::COLOR_BGR2HSV);

    int imageHeight = image.rows;
    int imageWidth = image.cols;

    // Find strokes
    auto strokes = findStrokeContours(grayscaleImage);

    // Analysis parameters
    const int analysisStep = 10;
    const int analysisWidth = analysisStep * 2;

    StrokeParameters params;

    // Process each stroke
    for (const auto& stroke : strokes) {
        cv::Rect boundingRect = cv::boundingRect(stroke);
        std::vector<double> intensityMeasurements;
        std::vector<double> pitchMeasurements;
        std::vector<double> densityMeasurements;
        std::vector<double> hueMeasurements;
        std::vector<double> valueMeasurements;
        std::vector<double> saturationMeasurements;
        std::vector<double> xPos;
        std::vector<double> strokeAngles;

        // Analyze stroke from left to right
        for (int currentX = boundingRect.x;
            currentX < boundingRect.x + boundingRect.width - analysisWidth;
            currentX += analysisStep) {

            int sliceWidth = std::min(analysisWidth,
                boundingRect.x + boundingRect.width - currentX);

            cv::Mat imageSlice = grayscaleImage(
                cv::Range(boundingRect.y, boundingRect.y + boundingRect.height),
                cv::Range(currentX, currentX + sliceWidth)
            );

            // Find stroke boundaries
            auto [topEdges, bottomEdges] = findStrokeBoundaries(
                imageSlice, sliceWidth, boundingRect.y, imageHeight);

            // Calculate midline
            std::vector<double> midline(sliceWidth);
            for (int i = 0; i < sliceWidth; ++i) {
                midline[i] = (topEdges[i] + bottomEdges[i]) / 2.0;
            }

            int sliceHeight = std::min(MAX_ANALYSIS_HEIGHT,
                static_cast<int>(round(*std::max_element(topEdges.begin(), topEdges.end()) -
                    *std::min_element(bottomEdges.begin(), bottomEdges.end()))));

            // Calculate stroke angle and center
            double centerX = currentX + sliceWidth / 2.0;
            double centerY = cv::mean(midline)[0];

            // Calculate stroke angle using central difference
            double strokeAngle = 0.0;
            if (midline.size() > 1) {
                strokeAngle = std::atan2(1.0,
                    (midline.back() - midline.front()) / static_cast<double>(sliceWidth));
            }
            strokeAngles.push_back(strokeAngle);

            // Rotate analysis window
            cv::Mat rotationMatrix = cv::getRotationMatrix2D(
                cv::Point2f(centerX, centerY),
                -cv::fastAtan2(1.0f, strokeAngle) + 90,
                1.0
            );

            cv::Mat rotatedImage;
            cv::warpAffine(hsvImage, rotatedImage, rotationMatrix,
                cv::Size(imageWidth, imageHeight));

            // Extract rotated slice
            cv::Mat sliceHSV;
            cv::getRectSubPix(rotatedImage,
                cv::Size(sliceWidth, sliceHeight),
                cv::Point2f(centerX, centerY),
                sliceHSV);

            // Convert back to BGR and grayscale for analysis
            cv::Mat sliceBGR, sliceGray;
            cv::cvtColor(sliceHSV, sliceBGR, cv::COLOR_HSV2BGR);
            cv::cvtColor(sliceBGR, sliceGray, cv::COLOR_BGR2GRAY);

            // Get rotated boundaries
            auto [rotatedTop, rotatedBottom] = findStrokeBoundaries(
                sliceGray, sliceWidth, 0, sliceHeight);

            // Calculate stroke width (intensity)
            double strokeWidth = 0.0;
            for (size_t i = 0; i < rotatedTop.size(); ++i) {
                strokeWidth += rotatedTop[i] - rotatedBottom[i];
            }
            strokeWidth /= rotatedTop.size();
            intensityMeasurements.push_back(strokeWidth / MAX_ANALYSIS_HEIGHT);

            // Calculate pitch from vertical position
            double verticalPosition = centerY;
            double pitch = (1.0 - verticalPosition / imageHeight) *
                (PITCH_RANGE[1] - PITCH_RANGE[0]) + PITCH_RANGE[0];
            pitchMeasurements.push_back(pitch);

            // Extract pixels for color analysis
            std::vector<cv::Vec3b> hsvPixels;
            std::vector<uchar> grayPixels;
            int totalPixels = 0;

            for (int x = 0; x < sliceWidth; ++x) {
                for (int y = static_cast<int>(rotatedBottom[x]);
                    y < static_cast<int>(rotatedTop[x]);
                    ++y) {
                    if (y >= 0 && y < sliceGray.rows && x >= 0 && x < sliceGray.cols) {
                        if (sliceGray.at<uchar>(y, x) <= 223) {
                            hsvPixels.push_back(sliceHSV.at<cv::Vec3b>(y, x));
                            grayPixels.push_back(sliceGray.at<uchar>(y, x));
                        }
                        totalPixels++;
                    }
                }
            }

            // Calculate density and color attributes
            if (!hsvPixels.empty()) {
                double density = static_cast<double>(hsvPixels.size()) / totalPixels;

                // Calculate mean HSV values
                cv::Scalar meanHSV = cv::mean(hsvPixels);

                densityMeasurements.push_back(density);
                hueMeasurements.push_back(meanHSV[0]);
                saturationMeasurements.push_back(meanHSV[1] / 256.0);
                valueMeasurements.push_back(meanHSV[2] / 256.0);
            }
            else {
                densityMeasurements.push_back(0.0);
                hueMeasurements.push_back(0.0);
                saturationMeasurements.push_back(0.0);
                valueMeasurements.push_back(0.0);
            }

            xPos.push_back(currentX);
        }

        // Store measurements for this stroke
        params.strokeIntensities.push_back(intensityMeasurements);
        params.strokePitches.push_back(pitchMeasurements);
        params.strokeDensities.push_back(densityMeasurements);
        params.strokeHues.push_back(hueMeasurements);
        params.strokeSaturations.push_back(saturationMeasurements);
        params.strokeValues.push_back(valueMeasurements);
        params.xPositions.push_back(xPos);
    }

    // Normalize x positions
    for (auto& positions : params.xPositions) {
        for (auto& pos : positions) {
            pos /= imageWidth;
        }
    }

    NotationResult result;
    result.params = params;
    result.originalImage = image;
    result.strokes = strokes;

    return result;
}

void testNotationToParameters2() {
    auto result = notationToParameters("W:\\mfm\\MFM_Synthsizer\\data\\notation\\06_A4.png");
    showResults(result.originalImage, result.strokes, result.params);
}

// the opencv is at C:\Users\a931e\opencv\build\include
